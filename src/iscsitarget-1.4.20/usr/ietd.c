/*
 * Copyright (C) 2002-2003 Ardis Technolgies <roman@ardistech.com>
 *
 * Released under the terms of the GNU GPL v2.0.
 */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <netdb.h>
#include <signal.h>

#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include "iscsid.h"
#include "ietadm.h"

static char* server_address;
uint16_t server_port = ISCSI_LISTEN_PORT;

/* poll数组，数组大小为45 */
struct pollfd poll_array[POLL_MAX];
/* 连接数组，最多保存32个连接 */
static struct connection *incoming[INCOMING_MAX];
/* 保存当前的连接总数 */
static int incoming_cnt;
/* 设备/dev/ietctl的文件描述符合 */
/* 本地网络的文件描述符合 */
/* netlink的文件描述符合 */
int ctrl_fd, ipc_fd, nl_fd;

static char program_name[] = "iscsid";

static struct option const long_options[] =
{
	{"config", required_argument, 0, 'c'},
	{"foreground", no_argument, 0, 'f'},
	{"debug", required_argument, 0, 'd'},
	{"uid", required_argument, 0, 'u'},
	{"gid", required_argument, 0, 'g'},
	{"address", required_argument, 0, 'a'},
	{"port", required_argument, 0, 'p'},
	{"version", no_argument, 0, 'v'},
	{"help", no_argument, 0, 'h'},
	{0, 0, 0, 0},
};

/* This will be configurable by command line options */
extern struct config_operations plain_ops;
struct config_operations *cops = &plain_ops;

static void usage(int status)
{
	if (status != 0)
		fprintf(stderr, "Try `%s --help' for more information.\n", program_name);
	else {
		printf("Usage: %s [OPTION]\n", program_name);
		printf("\
iSCSI target daemon.\n\
  -c, --config=[path]     Execute in the config file.\n");
		printf("\
  -f, --foreground        make the program run in the foreground\n\
  -d, --debug debuglevel  print debugging information\n\
  -u, --uid=uid           run as uid, default is current user\n\
  -g, --gid=gid           run as gid, default is current user group\n\
  -a, --address=address   listen on specified local address instead of all\n\
  -p, --port=port         listen on specified port instead of 3260\n\
  -h, --help              display this help and exit\n\
");
	}
	exit(1);
}

/* 比较用户态和核态版本 */
static int check_version(void)
{
	struct module_info info;
	int err;

	/* 获取核态版本 */
	err = ki->module_info(&info);
	if (err)
		return 0;

	/* 比较版本 */
	return !strncmp(info.version, IET_VERSION_STRING, sizeof(info.version));
}

/* 非阻塞 */
static void set_non_blocking(int fd)
{
	int res = fcntl(fd, F_GETFL);

	if (res != -1) {
		res = fcntl(fd, F_SETFL, res | O_NONBLOCK);
		if (res)
			log_warning("unable to set fd flags (%s)!", strerror(errno));
	} else
		log_warning("unable to get fd flags (%s)!", strerror(errno));
}

/* 创建监听socket */
static void create_listen_socket(struct pollfd *array)
{
	struct addrinfo hints, *res, *res0;
	char servname[64];
	int i, sock, opt;

	memset(servname, 0, sizeof(servname));
	snprintf(servname, sizeof(servname), "%d", server_port);

	memset(&hints, 0, sizeof(hints));
	/* 流 */
	hints.ai_socktype = SOCK_STREAM;
    /* bind */
	hints.ai_flags = AI_PASSIVE;

	/* 获取地址信息 ? */
	if (getaddrinfo(server_address, servname, &hints, &res0)) {
		log_error("unable to get address info (%s)!",
			(errno == EAI_SYSTEM) ? strerror(errno) :
						gai_strerror(errno));
		exit(1);
	}

	for (i = 0, res = res0; res && i < LISTEN_MAX; i++, res = res->ai_next) {
		sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (sock < 0) {
			log_error("unable to create server socket (%s) %d %d %d!",
				  strerror(errno), res->ai_family,
				  res->ai_socktype, res->ai_protocol);
			continue;
		}

		opt = 1;
		if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt)))
			log_warning("unable to set SO_KEEPALIVE on server socket (%s)!",
				    strerror(errno));

		opt = 1;
		if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
			log_warning("unable to set SO_REUSEADDR on server socket (%s)!",
				    strerror(errno));

		opt = 1;
		if (res->ai_family == AF_INET6 &&
		    setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt)))
			continue;

		if (bind(sock, res->ai_addr, res->ai_addrlen)) {
			log_error("unable to bind server socket (%s)!", strerror(errno));
			continue;
		}

		if (listen(sock, INCOMING_MAX)) {
			log_error("unable to listen to server socket (%s)!", strerror(errno));
			continue;
		}

		set_non_blocking(sock);

		array[i].fd = sock;
		/* 关心数据可读事件 */
		array[i].events = POLLIN;
	}

	freeaddrinfo(res0);
}

/* 接受连接请求 */
static void accept_connection(int listen)
{
	struct sockaddr_storage from;
	socklen_t namesize;
	struct pollfd *pollfd;
	struct connection *conn;
	int fd, i;

	namesize = sizeof(from);
	if ((fd = accept(listen, (struct sockaddr *) &from, &namesize)) < 0) {
		if (errno != EINTR && errno != EAGAIN) {
			perror("accept(incoming_socket)");
			exit(1);
		}
		return;
	}

	/* 获取空闲位置，用来保存连接信息 */
	for (i = 0; i < INCOMING_MAX; i++) {
		if (!incoming[i])
			break;
	}
	if (i >= INCOMING_MAX) {
		log_error("unable to find incoming slot? %d\n", i);
		exit(1);
	}

	/* 为连接申请内存 */
	if (!(conn = conn_alloc())) {
		log_error("fail to allocate %s", "conn\n");
		exit(1);
	}
	conn->fd = fd;
	/* 连接信息存入incoming数组 */
	incoming[i] = conn;
	/* 初始化读连接 */
	conn_read_pdu(conn);

	set_non_blocking(fd);
	/* 监听的fd转给连接的fd */
	pollfd = &poll_array[POLL_INCOMING + i];
	pollfd->fd = fd;
	/* 初始化连接事件信息 */
	pollfd->events = POLLIN;
	pollfd->revents = 0;

	incoming_cnt++;
	if (incoming_cnt >= INCOMING_MAX)
		poll_array[POLL_LISTEN].events = 0;
}

static void __set_fd(int idx, int fd)
{
	poll_array[idx].fd = fd;
	poll_array[idx].events = fd ? POLLIN : 0;
}

void isns_set_fd(int isns, int scn_listen, int scn)
{
	/* 初始化ISNS、SCN_LISTEN、SCN事件，关心数据可读事件 */
	__set_fd(POLL_ISNS, isns);
	__set_fd(POLL_SCN_LISTEN, scn_listen);
	__set_fd(POLL_SCN, scn);
}

void event_loop(int timeout)
{
	int res, i, opt;
	struct connection *conn;
	struct pollfd *pollfd;

	/* 创建监听socket */ 
	create_listen_socket(poll_array + POLL_LISTEN);

	/* 初始化IPC和netlink事件信息,同样关心有数据可读事件 */
	poll_array[POLL_IPC].fd = ipc_fd;
	poll_array[POLL_IPC].events = POLLIN;
	poll_array[POLL_NL].fd = nl_fd;
	poll_array[POLL_NL].events = POLLIN;

	/* 初始化 */
	for (i = 0; i < INCOMING_MAX; i++) {
		poll_array[POLL_INCOMING + i].fd = -1;
		poll_array[POLL_INCOMING + i].events = 0;
		incoming[i] = NULL;
	}

	while (1) {
		/* poll事件查询,
		 * POLLIN:    有数据从3260端口进来 
		 * POLL_NL:   有数据从netlink进来
		 * POLL_IPC:  有数据从AF_LOCAL进来
		 * POLL_ISNS:
		 * POLL_SCN:
		 */
		res = poll(poll_array, POLL_MAX, timeout);
		if (res == 0) {
			/* 超时处理 */
			isns_handle(1, &timeout);
			continue;
		} else if (res < 0) {
			if (res < 0 && errno != EINTR) {
				perror("poll()");
				exit(1);
			}
			continue;
		}

		/* 监听事件触发后，接受连接，注意在调用 accept_connection 后，还需要
		 * poll等待连接事件触发才会进入后面的连接处理 */
		for (i = 0; i < LISTEN_MAX; i++) {
			if (poll_array[POLL_LISTEN + i].revents
			    && incoming_cnt < INCOMING_MAX)
				accept_connection(poll_array[POLL_LISTEN + i].fd);
		}

		/* 处理事件 netlink */
		if (poll_array[POLL_NL].revents)
			handle_iscsi_events(nl_fd);

		/* 处理事件 IPC */
		if (poll_array[POLL_IPC].revents)
			ietadm_request_handle(ipc_fd);

		/* 处理事件 ISNS */
		if (poll_array[POLL_ISNS].revents)
			isns_handle(0, &timeout);

		/* 处理事件 SCN监听, 用于接受 SCN 连接 */
		if (poll_array[POLL_SCN_LISTEN].revents)
			isns_scn_handle(1);

		/* 处理事件 SCN, 处理数据 */
		if (poll_array[POLL_SCN].revents)
			isns_scn_handle(0);

		/* 循环处理所有连接数据 */
		for (i = 0; i < INCOMING_MAX; i++) {
			conn = incoming[i];
			pollfd = &poll_array[POLL_INCOMING + i];
			/* 如果连接事件没有触发, 循环下一个 */
			if (!conn || !pollfd->revents)
				continue;

			/* 重置连接事件 */
			pollfd->revents = 0;

			/* 判断IO类型,连接初始阶段为IOSTATE_READ_BHS */
			switch (conn->iostate) {
			/* io状态类型为读 */
			case IOSTATE_READ_BHS:
			case IOSTATE_READ_AHS_DATA:
			read_again:
				/* 读取数据 */
				/* rwsize初始值为48,BHS基本头段的固定大小 */
				res = read(pollfd->fd, conn->buffer, conn->rwsize);
				if (res <= 0) {
					if (res == 0 || (errno != EINTR && errno != EAGAIN))
						conn->state = STATE_CLOSE;
					else if (errno == EINTR)
						goto read_again;
					break;
				}
				conn->rwsize -= res;
				conn->buffer += res;
				/* 正常情况下，这里rwsize的值应该为0 */
				if (conn->rwsize)
					break;

				/* 判断读取状态 */
				switch (conn->iostate) {
				/* 处于读取BHS状态, 用于读取initiator端数据 */
				case IOSTATE_READ_BHS:
					/* 修改读取状态为AHS_DATA附加头段数据 */
					conn->iostate = IOSTATE_READ_AHS_DATA;
					/* 为什么要乘上4? */
					conn->req.ahssize = conn->req.bhs.ahslength * 4;
					/* 高低位相反,获取BHS段大小 */
					conn->req.datasize = ((conn->req.bhs.datalength[0] << 16) +
							      (conn->req.bhs.datalength[1] << 8) +
							      conn->req.bhs.datalength[2]);
					conn->rwsize = (conn->req.ahssize + conn->req.datasize + 3) & -4;
					/* 数据报大小不能超过8k */
					if (conn->rwsize > INCOMING_BUFSIZE) {
						log_warning("Recv PDU with "
							    "invalid size %d "
							    "(max: %d)",
							    conn->rwsize,
							    INCOMING_BUFSIZE);
						conn->state = STATE_CLOSE;
						goto conn_close;
					}
					if (conn->rwsize) {
						if (!conn->req_buffer) {
							conn->req_buffer = malloc(INCOMING_BUFSIZE);
							if (!conn->req_buffer) {
								log_error("Failed to alloc recv buffer");
								conn->state = STATE_CLOSE;
								goto conn_close;
							}
						}
						conn->buffer = conn->req_buffer;
						conn->req.ahs = conn->buffer;
						conn->req.data = conn->buffer + conn->req.ahssize;
						goto read_again;
					}

				/* 读取附件段数据 */
				case IOSTATE_READ_AHS_DATA:
					/* io状态由读转为写，初始化连接信息中的响应buffer */
					conn_write_pdu(conn);
					/* 修改poll事件 POLLOUT,这样poll机制才能触发 */
					pollfd->events = POLLOUT;

					/* 打印PDU */
					log_pdu(2, &conn->req);
					/* 执行命令 */
					if (!cmnd_execute(conn))
						conn->state = STATE_CLOSE;
					break;
				}
				break;

			/* io状态类型为写, 用于发送响应数据 */
			case IOSTATE_WRITE_BHS:
			case IOSTATE_WRITE_AHS:
			case IOSTATE_WRITE_DATA:
			write_again:
				opt = 1;
				setsockopt(pollfd->fd, SOL_TCP, TCP_CORK, &opt, sizeof(opt));
				res = write(pollfd->fd, conn->buffer, conn->rwsize);
				if (res < 0) {
					if (errno != EINTR && errno != EAGAIN)
						conn->state = STATE_CLOSE;
					else if (errno == EINTR)
						goto write_again;
					break;
				}

				conn->rwsize -= res;
				conn->buffer += res;
				if (conn->rwsize)
					goto write_again;

				switch (conn->iostate) {
				case IOSTATE_WRITE_BHS:
					if (conn->rsp.ahssize) {
						conn->iostate = IOSTATE_WRITE_AHS;
						conn->buffer = conn->rsp.ahs;
						conn->rwsize = conn->rsp.ahssize;
						goto write_again;
					}
				case IOSTATE_WRITE_AHS:
					if (conn->rsp.datasize) {
						int o;

						conn->iostate = IOSTATE_WRITE_DATA;
						conn->buffer = conn->rsp.data;
						conn->rwsize = conn->rsp.datasize;
						o = conn->rwsize & 3;
						if (o) {
							for (o = 4 - o; o; o--)
								*((u8 *)conn->buffer + conn->rwsize++) = 0;
						}
						goto write_again;
					}
				case IOSTATE_WRITE_DATA:
					opt = 0;
					setsockopt(pollfd->fd, SOL_TCP, TCP_CORK, &opt, sizeof(opt));
					cmnd_finish(conn);

					switch (conn->state) {
					case STATE_KERNEL:
						/* 连接转入内核 */
						conn_take_fd(conn, pollfd->fd);
						conn->state = STATE_CLOSE;
						break;
					case STATE_EXIT:
					case STATE_CLOSE:
						break;
					default:
						conn_read_pdu(conn);
						pollfd->events = POLLIN;
						break;
					}
					break;
				}

				break;
			default:
				log_error("illegal iostate %d for port %d!\n", conn->iostate, i);
				exit(1);
			}

		conn_close:
			if (conn->state == STATE_CLOSE) {
				/* 连接关闭后，删除连接；如果会话中的连接数为0，删除会话；*/
				/* 并且释放相关内存 */
				struct session *session = conn->session;
				log_debug(1, "connection closed");
				conn_free_pdu(conn);
				conn_free(conn);
				close(pollfd->fd);
				pollfd->fd = -1;
				incoming[i] = NULL;
				incoming_cnt--;
				if (session && !session->conn_cnt)
					session_remove(session);
			}
		}
	}
}

/* 入口函数 */
int main(int argc, char **argv)
{
	int ch, longindex, timeout = -1;
	char *config = NULL;
	uid_t uid = 0;
	gid_t gid = 0;
	char *isns = NULL;
	int isns_ac = 0;

	/* otherwise we would die in some later write() during the event_loop
	 * instead of getting EPIPE! */
	signal(SIGPIPE, SIG_IGN);

	while ((ch = getopt_long(argc, argv, "c:fd:s:u:g:a:p:vh", long_options, &longindex)) >= 0) {
		switch (ch) {
		case 'c':
			config = optarg;
			break;
		case 'f':
			log_daemon = 0;
			break;
		case 'd':
			log_level = atoi(optarg);
			break;
		case 'u':
			uid = strtoul(optarg, NULL, 10);
			break;
		case 'g':
			gid = strtoul(optarg, NULL, 10);
			break;
		case 'a':
			server_address = strdup(optarg);
			break;
		case 'p':
			server_port = (uint16_t)strtoul(optarg, NULL, 10);
			break;
		case 'v':
			printf("%s version %s\n", program_name, IET_VERSION_STRING);
			exit(0);
			break;
		case 'h':
			usage(0);
			break;
		default:
			usage(1);
			break;
		}
	}

	/* 日志初始化 */
	log_init();

	/* 打开netlink网络连接 */
	if ((nl_fd = nl_open()) < 0) {
		log_error("unable to open netlink fd: %m");
		exit(-1);
	};

	/* 打开设备/dev/ietctl, 用于跟内核态的iscsi通信 */
	if ((ctrl_fd = ki->ctldev_open()) < 0) {
		log_error("unable to open ctldev fd: %m");
		exit(-1);
	}

	/* 监听本地网络AF_LOCAL,用于处理来自ietadm的请求 */
	if ((ipc_fd = ietadm_request_listen()) < 0) {
		log_error("unable to open ipc fd: %m");
		exit(-1);
	}

	/* 检查版本信息，如果用户态跟内核态版本不一致，直接退出 */
	if (!check_version()) {
		log_error("kernel module version mismatch!");
		exit(-1);
	}

	/* daemon 方式运行，如果用-f参数不会进入 */
	if (log_daemon) {
		char buf[64];
		pid_t pid;
		int fd;

		fd = open("/var/run/iscsi_trgt.pid", O_WRONLY|O_CREAT, 0644);
		if (fd < 0) {
			log_error("unable to create pid file: %m");
			exit(1);
		}
		pid = fork();
		if (pid < 0) {
			log_error("error starting daemon: %m");
			exit(1);
		} else if (pid)
			exit(0);

		if (chdir("/") < 0) {
			log_error("failed to set working dir to /: %m");
			exit(1);
		}

		if (lockf(fd, F_TLOCK, 0) < 0) {
			log_error("unable to lock pid file: %m");
			exit(1);
		}
		if (ftruncate(fd, 0) < 0) {
			log_error("failed to ftruncate the PID file: %m");
			exit(1);
		}

		sprintf(buf, "%d\n", getpid());
		if (write(fd, buf, strlen(buf)) < strlen(buf)) {
			log_error("failed to write PID to PID file: %m");
			exit(1);
		}

		close(0);
		open("/dev/null", O_RDWR);
		dup2(0, 1);
		dup2(0, 2);
		setsid();
	}

	/* 读取配置文件初始化/etc/iet/ietd.conf和/etc/ietd.conf */
	cops->init(config, &isns, &isns_ac);
	if (isns)
		timeout = isns_init(isns, isns_ac);

	if (gid && setgid(gid) < 0)
		log_error("unable to setgid: %m");

	if (uid && setuid(uid) < 0)
		log_error("unable to setuid: %m");

	/* 事件循环 */
	event_loop(timeout);

	return 0;
}

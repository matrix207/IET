###ietd
------------------

    main

    getopt_long

    nl_open: [event.c] 打开网络连接netlink, PF_NETLINK

    ctldev_open: [ctldev.c] 打开设备控制/dev/ietctl

    ietadm_request_listen: [message.c] 监听本地网络AF_LOCAL,来自ietadm的请求

    plain_init: [plain.c] 读取配置文件初始化/etc/iet/ietd.conf和/etc/ietd.conf
       plain_portal_init : 检查SNS配置
       plain_target_init : 检查taget配置,创建Target和Lun, 设置target和session参数
       plain_account_init: 检查帐号配置, 创建帐号

    isns_init: [SNS.c] 初始化SNS服务
        scn_init: scn?

    event_loop

      create_listen_socket 

      while:
        poll
        accept_connection
        if (poll_array[POLL_NL].revents)
            handle_iscsi_events(nl_fd);

        if (poll_array[POLL_IPC].revents)
            ietadm_request_handle(ipc_fd);

        if (poll_array[POLL_ISNS].revents)
            isns_handle(0, &timeout);

        if (poll_array[POLL_SCN_LISTEN].revents)
            isns_scn_handle(1);

        if (poll_array[POLL_SCN].revents)
            isns_scn_handle(0);

        ...

    #define LISTEN_MAX 8
    #define INCOMING_MAX 32
    enum {
        POLL_LISTEN,                             /* val:0  监听事件 8个 */
        POLL_IPC = POLL_LISTEN + LISTEN_MAX,     /* val:8  IPC          */
        POLL_NL,                                 /* val:9  Netlink      */
        POLL_ISNS,                               /* val:10 SNS protocol */
        POLL_SCN_LISTEN,                         /* val:11 监听SCN?     */
        POLL_SCN,                                /* val:12 SCN?         */
        POLL_INCOMING,                           /* val:13 处理数据     */
        POLL_MAX = POLL_INCOMING + INCOMING_MAX, /* val:45 最多32路     */
    };

------------------

####question:

1. getaddrinfo 用于监听绑定? 而且还可以用于获取域名的IP地址.?


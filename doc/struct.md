-------------------------------------------------------------
iscsid.h

    struct buf_segment {
        struct __qelem entry;

        unsigned int len;
        char data[0];
    };

    struct PDU {
        struct iscsi_hdr bhs;
        void *ahs;
        unsigned int ahssize;
        void *data;
        unsigned int datasize;
    };

    struct session {
        struct __qelem slist;

        char *initiator;
        struct target *target;
        union iscsi_sid sid;

        int conn_cnt;
    };

    struct connection {
        int state;
        int iostate;
        int fd;

        struct session *session;

        u32 tid;
        struct iscsi_param session_param[session_key_last];

        char *initiator;
        union iscsi_sid sid;
        u16 cid;

        int session_type;
        int auth_method;

        u32 stat_sn;
        u32 exp_stat_sn;

        u32 cmd_sn;
        u32 exp_cmd_sn;
        u32 max_cmd_sn;
        u32 ttt;

        struct PDU req;
        void *req_buffer;
        struct PDU rsp;
        struct __qelem rsp_buf_list;

        unsigned char *buffer;
        int rwsize;

        int auth_state;
        union {
            struct {
                int digest_alg;
                int id;
                int challenge_size;
                unsigned char *challenge;
            } chap;
        } auth;
    };

    struct target {
        struct __qelem tlist;

        struct __qelem sessions_list;

        u32 tid;
        char name[ISCSI_NAME_LEN];
        char *alias;

        struct redirect_addr {
            char addr[NI_MAXHOST + 1];
            char port[NI_MAXSERV + 1];
            u8 type;
        } redirect;

        int max_nr_sessions;
        int nr_sessions;

        struct __qelem isns_head;
    };

    struct iscsi_kernel_interface {
        int (*ctldev_open) (void);
        int (*module_info) (struct module_info *);
        int (*lunit_create) (u32 tid, u32 lun, char *args);
        int (*lunit_destroy) (u32 tid, u32 lun);
        int (*param_get) (u32, u64, int, struct iscsi_param *);
        int (*param_set) (u32, u64, int, u32, struct iscsi_param *);
        int (*target_create) (u32 *, char *);
        int (*target_destroy) (u32);
        int (*session_create) (u32, u64, u32, u32, char *);
        int (*session_destroy) (u32, u64);
        int (*session_info) (struct session_info *);
        int (*conn_create) (u32, u64, u32, u32, u32, int, u32, u32);
        int (*conn_destroy) (u32 tid, u64 sid, u32 cid);
    };


-------------------------------------------------------------
poll.h

    struct pollfd
    {
        int fd;			    /* File descriptor to poll.  */
        short int events;	/* Types of events poller cares about.  */
        short int revents;	/* Types of events that actually occurred.  */
    };

---------------------------------------------------------------
config.h

    struct config_operations {
        void (*init) (char *, char **, int *);
        int (*target_add) (u32 *, char *);
        int (*target_stop) (u32);
        int (*target_del) (u32);
        int (*lunit_add) (u32, u32, char *);
        int (*lunit_stop) (u32, u32);
        int (*lunit_del) (u32, u32);
        int (*param_set) (u32, u64, int, u32, struct iscsi_param *);
        int (*account_add) (u32, int, char *, char *);
        int (*account_del) (u32, int, char *);
        int (*account_query) (u32, int, char *, char *);
        int (*account_list) (u32, int, u32 *, u32 *, char *, size_t);
        int (*initiator_allow) (u32, int, char *);
        int (*target_allow) (u32, struct sockaddr *);
        int (*target_redirect) (u32, char *, u8);
    };

---------------------------------------------------------------
ietadm.h

    struct msg_trgt {
        char name[ISCSI_NAME_LEN];
        char alias[ISCSI_NAME_LEN];

        u32 type;
        u32 session_partial;
        u32 target_partial;
        struct iscsi_param session_param[session_key_last];
        struct iscsi_param target_param[target_key_last];
    };

    struct msg_acnt {
        u32 auth_dir;
        union {
            struct {
                char name[ISCSI_NAME_LEN];
                char pass[ISCSI_NAME_LEN];
            } user;
            struct {
                u32 alloc_len;
                u32 count;
                u32 overflow;
            } list;
        } u;
    };

    struct msg_lunit {
        char args[ISCSI_ARGS_LEN];
    };

    struct msg_redir {
        char dest[NI_MAXHOST + NI_MAXSERV + 4];
    };

    enum ietadm_cmnd {
        C_TRGT_NEW,
        C_TRGT_DEL,
        C_TRGT_UPDATE,
        C_TRGT_SHOW,
        C_TRGT_REDIRECT,

        C_SESS_NEW,
        C_SESS_DEL,
        C_SESS_UPDATE,
        C_SESS_SHOW,

        C_CONN_NEW,
        C_CONN_DEL,
        C_CONN_UPDATE,
        C_CONN_SHOW,

        C_LUNIT_NEW,
        C_LUNIT_DEL,
        C_LUNIT_UPDATE,
        C_LUNIT_SHOW,

        C_ACCT_NEW,
        C_ACCT_DEL,
        C_ACCT_UPDATE,
        C_ACCT_SHOW,

        C_SYS_NEW,
        C_SYS_DEL,
        C_SYS_UPDATE,
        C_SYS_SHOW,

        C_ACCT_LIST,
    };

    struct ietadm_req {
        enum ietadm_cmnd rcmnd;

        u32 tid;
        u64 sid;
        u32 cid;
        u32 lun;

        union {
            struct msg_trgt trgt;
            struct msg_acnt acnt;
            struct msg_lunit lunit;
            struct msg_redir redir;
        } u;
    };

    struct ietadm_rsp {
        int err;
    };

-------------------------------------------------------
md5.h

    struct md5_ctx {
        u32 block[MD5_BLOCK_WORDS];
        u32 digest[MD5_DIGEST_WORDS];
        u64 count;
    };

-------------------------------------------------------
param.h

    struct iscsi_param {
        int state;
        unsigned int val;
    };

    struct iscsi_key_ops {
        int (*val_to_str)(unsigned int, char *);
        int (*str_to_val)(char *, unsigned int *);
        int (*check_val)(struct iscsi_key *, unsigned int *);
        int (*set_val)(struct iscsi_param *, int, unsigned int *);
    };

    struct iscsi_key {
        char *name;
        unsigned int def;
        unsigned int min;
        unsigned int max;
        struct iscsi_key_ops *ops;
    };


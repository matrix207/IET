-------------------------------------------------------------
iet_u.h

    struct module_info {
        char version[128];
    };

    struct target_info {
        u32 tid;
        char name[ISCSI_NAME_LEN];
    };

    struct volume_info {
        u32 tid;
        u32 lun;
        aligned_u64 args_ptr;
        u32 args_len;
    };

    struct session_info {
        u32 tid;

        aligned_u64 sid;
        char initiator_name[ISCSI_NAME_LEN];
        u32 exp_cmd_sn;
        u32 max_cmd_sn;
    };

    struct conn_info {
        u32 tid;
        aligned_u64 sid;

        u32 cid;
        u32 stat_sn;
        u32 exp_stat_sn;
        int header_digest;
        int data_digest;
        int fd;
    };

    enum {
        key_initial_r2t,
        key_immediate_data,
        key_max_connections,
        key_max_recv_data_length,
        key_max_xmit_data_length,
        key_max_burst_length,
        key_first_burst_length,
        key_default_wait_time,
        key_default_retain_time,
        key_max_outstanding_r2t,
        key_data_pdu_inorder,
        key_data_sequence_inorder,
        key_error_recovery_level,
        key_header_digest,
        key_data_digest,
        key_ofmarker,
        key_ifmarker,
        key_ofmarkint,
        key_ifmarkint,
        session_key_last,
    };

    enum {
        key_wthreads,
        key_target_type,
        key_queued_cmnds,
        key_nop_interval,
        key_nop_timeout,
        target_key_last,
    };

    enum {
        key_session,
        key_target,
    };

    struct iscsi_param_info {
        u32 tid;
        aligned_u64 sid;

        u32 param_type;
        u32 partial;

        u32 session_param[session_key_last];
        u32 target_param[target_key_last];
    };

    enum iet_event_state {
        E_CONN_CLOSE,
    };

    struct iet_event {
        u32 tid;
        aligned_u64 sid;
        u32 cid;
        u32 state;
    };

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

-------------------------------------------------------------
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

-------------------------------------------------------------
md5.h

    struct md5_ctx {
        u32 block[MD5_BLOCK_WORDS];
        u32 digest[MD5_DIGEST_WORDS];
        u64 count;
    };

-------------------------------------------------------------
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

-------------------------------------------------------------
iscsi_hdr.h

    struct iscsi_hdr {
        u8  opcode;			/* 0 */
        u8  flags;
        u8  spec1[2];
        u8  ahslength;			/* 4 */
        u8  datalength[3];
        u16 lun[4];			/* 8 */
        u32 itt;			/* 16 */
        u32 ttt;			/* 20 */
        u32 sn;				/* 24 */
        u32 exp_sn;			/* 28 */
        u32 max_sn;			/* 32 */
        u32 spec3[3];			/* 36 */
    } __packed;				/* 48 */

    struct iscsi_ahs_hdr {
        u16 ahslength;
        u8 ahstype;
    } __packed;

    union iscsi_sid {
        struct {
            u8 isid[6];		/* Initiator Session ID */
            u16 tsih;		/* Target Session ID */
        } id;
        u64 id64;
    } __packed;

    struct iscsi_text_req_hdr {
        u8  opcode;
        u8  flags;
        u16 rsvd1;
        u8  ahslength;
        u8  datalength[3];
        u32 rsvd2[2];
        u32 itt;
        u32 ttt;
        u32 cmd_sn;
        u32 exp_stat_sn;
        u32 rsvd3[4];
    } __packed;

    struct iscsi_text_rsp_hdr {
        u8  opcode;
        u8  flags;
        u16 rsvd1;
        u8  ahslength;
        u8  datalength[3];
        u32 rsvd2[2];
        u32 itt;
        u32 ttt;
        u32 stat_sn;
        u32 exp_cmd_sn;
        u32 max_cmd_sn;
        u32 rsvd3[3];
    } __packed;

    struct iscsi_login_req_hdr {
        u8  opcode;
        u8  flags;
        u8  max_version;		/* Max. version supported */
        u8  min_version;		/* Min. version supported */
        u8  ahslength;
        u8  datalength[3];
        union iscsi_sid sid;
        u32 itt;			/* Initiator Task Tag */
        u16 cid;			/* Connection ID */
        u16 rsvd1;
        u32 cmd_sn;
        u32 exp_stat_sn;
        u32 rsvd2[4];
    } __packed;

    struct iscsi_login_rsp_hdr {
        u8  opcode;
        u8  flags;
        u8  max_version;		/* Max. version supported */
        u8  active_version;		/* Active version */
        u8  ahslength;
        u8  datalength[3];
        union iscsi_sid sid;
        u32 itt;			/* Initiator Task Tag */
        u32 rsvd1;
        u32 stat_sn;
        u32 exp_cmd_sn;
        u32 max_cmd_sn;
        u8  status_class;		/* see Login RSP ststus classes below */
        u8  status_detail;		/* see Login RSP Status details below */
        u8  rsvd2[10];
    } __packed;

    struct iscsi_logout_req_hdr {
        u8  opcode;
        u8  flags;
        u16 rsvd1;
        u8  ahslength;
        u8  datalength[3];
        u32 rsvd2[2];
        u32 itt;
        u16 cid;
        u16 rsvd3;
        u32 cmd_sn;
        u32 exp_stat_sn;
        u32 rsvd4[4];
    } __packed;

    struct iscsi_logout_rsp_hdr {
        u8  opcode;
        u8  flags;
        u8  response;
        u8  rsvd1;
        u8  ahslength;
        u8  datalength[3];
        u32 rsvd2[2];
        u32 itt;
        u32 rsvd3;
        u32 stat_sn;
        u32 exp_cmd_sn;
        u32 max_cmd_sn;
        u32 rsvd4;
        u16 time2wait;
        u16 time2retain;
        u32 rsvd5;
    } __packed;

    struct iscsi_reject_hdr {
        u8  opcode;
        u8  flags;
        u8  reason;
        u8  rsvd1;
        u8  ahslength;
        u8  datalength[3];
        u32 rsvd2[2];
        u32 ffffffff;
        u32 rsvd3;
        u32 stat_sn;
        u32 exp_cmd_sn;
        u32 max_cmd_sn;
        u32 data_sn;
        u32 rsvd4[2];
    } __packed;


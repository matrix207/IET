netlink 关联到的操作的流程：

iscsi.c
iscsi_init(void) -> register_chrdev

config.c
-> struct file_operations ctr_fops -> ioctl() -> add_target() 

target.c
-> target_thread_start() > iscsi_target_create() ->  target_thread_start()

    nthread.c                                       event.c
-> nthread_start() ->  ietd() -> close_conn() -> event_send() -> notify() -> __nlmsg_put()  

---------------------------------------------------------------------------------------

event.c                               ietd.c
nl_read() -> handle_iscsi_events() -> event_loop() -> main()

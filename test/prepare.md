###iet

    [root@ ~]# dd if=/dev/zero of=/root/iscsi.disk bs=1M count=1024
    1024+0 records in
    1024+0 records out
    1073741824 bytes (1.1 GB) copied, 9.30143 s, 115 MB/s
    [root@ ~]# echo "Target iqn.2014-06.com.matrix:storage.515.disk" >> /etc/ietd.conf
    [root@ ~]# echo "Lun 0 Path=/root/iscsi.disk,Type=fileio">>/etc/ietd.conf
    [root@ ~]# echo "Alias lun0">>/etc/ietd.conf
    [root@ ~]# cat /etc/ietd.conf
    Target iqn.2014-06.com.matrix:storage.515.disk
    Lun 0 Path=/root/iscsi.disk,Type=fileio
    Alias lun0
    [root@ ~]# echo "iqn.2014-06.com.matrix:storage.515.disk 172.16.*.*">> /etc/initiators.allow
    [root@ ~]# cat /etc/initiators.allow
    iqn.2014-06.com.matrix:storage.515.disk 172.16.*.*
    [root@ ~]# netstat -tulpn | grep 3260
    tcp 0 0 172.16.110.10:3260 0.0.0.0:* LISTEN 12147/ietd
    [root@ ~]# cat /proc/net/iet/session
    tid:1 name:iqn.2014-06.com.matrix:storage.515.disk
    [root@ ~]# cat /proc/net/iet/volume
    tid:1 name:iqn.2014-06.com.matrix:storage.515.disk
    lun:0 state:0 iotype:fileio iomode:wt blocks:2097152 blocksize:512 path:/root/iscsi.disk
    [root@ ~]# /etc/init.d/iscsi-target restart
    Stopping iSCSI Target: [ OK ]
    Starting iSCSI Target: FATAL: Error inserting crc32c_intel (/lib/modules/2.6
    .32-279.el6.x86_64/kernel/arch/x86/crypto/crc32c-intel.ko): No such device
    [ OK ]
    [root@ ~]# /etc/init.d/iscsi-target restart
    Stopping iSCSI Target: [ OK ]
    Starting iSCSI Target: [ OK ]
    [root@ ~]# service iscsi-target status
    iSCSI Target (pid 12147) is running...

###iSCSI initiator

安装 `yum install iscsi-initiator-utils`

    [root@localhost dennis]# fdisk -l
    Disk /dev/sda: 149 GiB, 160000000000 bytes, 312500000 sectors
    Units: sectors of 1 * 512 = 512 bytes
    Sector size (logical/physical): 512 bytes / 512 bytes
    I/O size (minimum/optimal): 512 bytes / 512 bytes
    Disklabel type: dos
    Disk identifier: 0x00007f87
    Device Boot Start End Blocks Id System
    /dev/sda1 * 2048 1026047 512000 83 Linux
    /dev/sda2 1026048 312498175 155736064 8e Linux LVM
    (...此处省略不必要的内容...)
    [root@localhost dennis]# iscsiadm -m node -T iqn.2014-06.com.matrix:storage.515.disk -p 172.16.110.10 -l
    Logging in to [iface: default, target: iqn.2014-06.com.matrix:storage.515.disk, portal: 172.16.110.10,3260] (multiple)
    Login to [iface: default, target: iqn.2014-06.com.matrix:storage.515.disk, portal: 172.16.110.10,3260] successful.
    [root@localhost dennis]# fdisk -l
    Disk /dev/sda: 149 GiB, 160000000000 bytes, 312500000 sectors
    Units: sectors of 1 * 512 = 512 bytes
    Sector size (logical/physical): 512 bytes / 512 bytes
    I/O size (minimum/optimal): 512 bytes / 512 bytes
    Disklabel type: dos
    Disk identifier: 0x00007f87
    Device Boot Start End Blocks Id System
    /dev/sda1 * 2048 1026047 512000 83 Linux
    /dev/sda2 1026048 312498175 155736064 8e Linux LVM
    (...此处省略不必要的内容...)
    (成功连接后,多出一块磁盘/dev/sdb，大小刚好是前面dd创建的1G文件)
    Disk /dev/sdb: 1 GiB, 1073741824 bytes, 2097152 sectors
    Units: sectors of 1 * 512 = 512 bytes
    Sector size (logical/physical): 512 bytes / 512 bytes
    I/O size (minimum/optimal): 512 bytes / 512 bytes
    [root@localhost dennis]# iscsiadm -m node -T iqn.2014-06.com.matrix:storage.515.disk -p 172.16.110.10 -u
    Logging out of session [sid: 1, target: iqn.2014-06.com.matrix:storage.515.disk, portal: 172.16.110.10,3260]
    Logout of [sid: 1, target: iqn.2014-06.com.matrix:storage.515.disk, portal: 172.16.110.10,3260] successful.
    [root@localhost dennis]# 

连接成功后，iet端显示如下：

    [root@ ~]# cat /proc/net/iet/session
    tid:1 name:iqn.2014-06.com.matrix:storage.515.disk
    sid:562949990973952 initiator:iqn.1994-05.com.redhat:7d366003913
    cid:0 ip:172.16.50.39 state:active hd:none dd:none tip:172.16.110.10

第一次连接时，磁盘是还没有分区化, 磁盘分区：

    [root@localhost dennis]# fdisk /dev/sdb
    Welcome to fdisk (util-linux 2.24.2).
    Changes will remain in memory only, until you decide to write them.
    Be careful before using the write command.
    Device does not contain a recognized partition table.
    Created a new DOS disklabel with disk identifier 0xf1eb4d09.
    Command (m for help): m
    Help:
    DOS (MBR)
    a toggle a bootable flag
    b edit nested BSD disklabel
    c toggle the dos compatibility flag
    Generic
    d delete a partition
    l list known partition types
    n add a new partition
    p print the partition table
    t change a partition type
    v verify the partition table
    Misc
    m print this menu
    u change display/entry units
    x extra functionality (experts only)
    Save & Exit
    w write table to disk and exit
    q quit without saving changes
    Create a new label
    g create a new empty GPT partition table
    G create a new empty SGI (IRIX) partition table
    o create a new empty DOS partition table
    s create a new empty Sun partition table
    Command (m for help): g
    Created a new GPT disklabel (GUID: 22C002F1-D9F7-4E4C-8F89-9EFEBB971A1D).
    Command (m for help): n
    Partition number (1-128, default 1): 1
    First sector (2048-2097118, default 2048):
    Last sector, +sectors or +size{K,M,G,T,P} (2048-2097118, default 2097118):
    Created a new partition 1 of type 'Linux filesystem' and of size 1023 MiB.
    Command (m for help): p
    Disk /dev/sdb: 1 GiB, 1073741824 bytes, 2097152 sectors
    Units: sectors of 1 * 512 = 512 bytes
    Sector size (logical/physical): 512 bytes / 512 bytes
    I/O size (minimum/optimal): 512 bytes / 512 bytes
    Disklabel type: gpt
    Disk identifier: 22C002F1-D9F7-4E4C-8F89-9EFEBB971A1D
    Device Start End Size Type
    /dev/sdb1 2048 2097118 1023M Linux filesystem
    Command (m for help): w
    The partition table has been altered.
    Calling ioctl() to re-read partition table.
    Syncing disks.
    [root@localhost dennis]# fdisk -l
    Disk /dev/sda: 149 GiB, 160000000000 bytes, 312500000 sectors
    Units: sectors of 1 * 512 = 512 bytes
    Sector size (logical/physical): 512 bytes / 512 bytes
    I/O size (minimum/optimal): 512 bytes / 512 bytes
    Disklabel type: dos
    Disk identifier: 0x00007f87
    Device Boot Start End Blocks Id System
    /dev/sda1 * 2048 1026047 512000 83 Linux
    /dev/sda2 1026048 312498175 155736064 8e Linux LVM
    (...此处省略不必要的内容...)
    Disk /dev/sdb: 1 GiB, 1073741824 bytes, 2097152 sectors
    Units: sectors of 1 * 512 = 512 bytes
    Sector size (logical/physical): 512 bytes / 512 bytes
    I/O size (minimum/optimal): 512 bytes / 512 bytes
    Disklabel type: gpt
    Disk identifier: 22C002F1-D9F7-4E4C-8F89-9EFEBB971A1D
    Device Start End Size Type
    /dev/sdb1 2048 2097118 1023M Linux filesystem

分区完毕后，对磁盘该分区进行格式化：

    [root@localhost dennis]# mkfs.ext3 /dev/sdb1
    mke2fs 1.42.8 (20-Jun-2013)
    Filesystem label=
    OS type: Linux
    Block size=4096 (log=2)
    Fragment size=4096 (log=2)
    Stride=0 blocks, Stripe width=0 blocks
    65536 inodes, 261883 blocks
    13094 blocks (5.00%) reserved for the super user
    First data block=0
    Maximum filesystem blocks=268435456
    8 block groups
    32768 blocks per group, 32768 fragments per group
    8192 inodes per group
    Superblock backups stored on blocks:
    32768, 98304, 163840, 229376
    Allocating group tables: done
    Writing inode tables: done
    Creating journal (4096 blocks): done
    Writing superblocks and filesystem accounting information: done

格式化完毕后,进行挂载，然后就可以像对待本地目录一样进行相关操作了(拷贝、删除等) 
**注：下一次登陆iscsi卷后，不用进行分区、格式化了，直接挂在就可以使用了**

    [root@localhost dennis]# mkdir /mnt/iscsi
    [root@localhost dennis]# mount /dev/sdb1 /mnt/iscsi
    [root@localhost dennis]# ls /mnt/iscsi/
    lost+found
    [root@localhost dennis]# cp Downloads/VirtualBox-4.3-4.3.12_93733_fedora18-1.x86_64.rpm /mnt/iscsi/
    [root@localhost dennis]# ls -lh /mnt/iscsi/
    total 74M
    drwx------. 2 root root 16K Jun 27 12:10 lost+found
    -rw-r--r--. 1 root root 74M Jun 27 12:13 VirtualBox-4.3-4.3.12_93733_fedora18-1.x86_64.rpm
    [root@localhost dennis]# 

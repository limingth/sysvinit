Sysvinit 项目测试案例分析
=========================

测试 wall 命令
-------------------------

### 编译 sysvinit 项目获得可执行文件 wall
	$ cd sysvinit-2.88dsf/
	$ make
	make -C src all
	make[1]: Entering directory `/home/akaedu/Github/sysvinit/sysvinit-2.88dsf/src'
	make[1]: Nothing to be done for `all'.
	make[1]: Leaving directory `/home/akaedu/Github/sysvinit/sysvinit-2.88dsf/src'
	$ ls src/wall -l
	-rwxrwxr-x 1 akaedu akaedu 13243 Jun 22 14:49 src/wall
	$ 

查看 src 目录下已经生成 wall 命令。

### 执行 wall 命令加消息参数
	$ src/wall "hello msg"
	$ 
	Broadcast message from akaedu@ubuntu (pts/1) (Sun Jun 23 08:59:32 2013):

	hello msg

能够看到有广播的消息显示在终端窗口。

### 打开新的 Terminal 窗口，再次执行该命令
	$ 
	Broadcast message from akaedu@ubuntu (pts/1) (Sun Jun 23 09:00:36 2013):

	hello msg

此时，新打开的终端窗口也能够看到有广播的消息显示出来。


测试 mesg 命令
-------------------------

### 使用 tty 命令查看终端名称 tty
	$ tty
	/dev/pts/1
	$ 

获得终端名称 pts/1

### 使用 who 命令查看当前登录用户名 user
	$ who
	akaedu   tty2         2013-06-22 18:44
	akaedu   pts/1        2013-06-22 18:45 (:0.0)
	$ 

### 使用 write 命令给当前终端发消息
	$ write akaedu pts/1

	Message from akaedu@ubuntu on pts/1 at 09:13 ...
	hello msg
	hello msg
	test write cmd
	test write cmd
	EOF
	$ 

可以看到 write 命令能够实现自己给自己当前的终端发消息。按 ctrl+d 结束输入。

### 使用 mesg n 禁止消息接收功能
	$ mesg n
	$ write akaedu pts/1
	write: write: you have write permission turned off.

	write: akaedu has messages disabled on pts/1

可以看到当前终端如果使用 mesg n 命令之后，就不再接收 write 发来的消息。但如果用 wall 命令发送仍然可以接收。

write 也支持给其他终端发消息，做法是打开新的 Terminal 窗口，同样需要查看登录用户名和终端名称。
	

测试 killall5 命令
-------------------------

### 打开3个终端窗口
	$ (ctrl+alt+f1) 三个键同时按下

	$ (ctrl+alt+f2)

	$ (ctrl+alt+f3)

### 选择第2个和第3个输入某个命令
	(ctrl+alt+f2 选择2号窗口)
	$ ls

	(ctrl+alt+f3 选择3号窗口)
	$ cat

### 切换到第1个窗口，运行 killall5 命令
	$ ./src/killall5 

此时切换回刚才的两个窗口，发现都已经退出，重新回到登录界面，等待输入用户名和密码。

其他进程收到 kill 命令后，都被杀死，只有当前终端窗口仍然可以工作。

注： 不能在 X 窗口的终端里面测试该命令，会造成黑屏，无法恢复。

### 测试 -o 选项
	$ ./src/killall5 -o 2640


测试 pidof 命令
-------------------------

### pidof 命令直接跟进程名称
	$ pidof bash
	3023 2213
	$ ps aux | grep bash
	akaedu    2213  0.1  0.1  11412   972 tty2     S    16:11   0:01 -bash
	akaedu    3023  0.0  0.8   8192  4276 pts/2    Ss   16:12   0:00 bash
	akaedu    3383  0.0  0.1   4388   840 pts/2    S+   16:23   0:00 grep --color=auto bash
	$ 


### pidof 命令加 -s 参数
	$ pidof -s bash
	3023
	$ 

### pidof 命令和 kill 联合使用杀死进程
	$ 打开一个 Terminal 窗口启动一个 vim 程序
	$ 回到原来的窗口运行 pidof 命令
	$ pidof vim
	3471
	$ pidof vim | xargs kill

切换到刚才的新窗口，查看 vim 进程已经被杀死

	$ vim
	Vim: Caught deadly signal TERM
	Vim: Finished.


	Terminated
	$ 

测试 mountpoint 命令
-------------------------

### 查看一个目录是否为一个挂载点
	$ df
	Filesystem     1K-blocks    Used Available Use% Mounted on
	/dev/sda1        9928244 8427228   1002996  90% /
	udev              245968       4    245964   1% /dev
	tmpfs             101416     976    100440   1% /run
	none                5120       0      5120   0% /run/lock
	none              253536     260    253276   1% /run/shm
	$ src/mountpoint /
	/ is a mountpoint
	$ src/mountpoint /dev
	/dev is a mountpoint
	$ src/mountpoint /bin
	/bin is not a mountpoint
	$ src/mountpoint /home
	/home is not a mountpoint
	$ 

### 查看某个文件系统的主/从设备号：
	$ df
	Filesystem     1K-blocks    Used Available Use% Mounted on
	/dev/sda1        9928244 8427232   1002992  90% /
	udev              245968       4    245964   1% /dev
	tmpfs             101416     976    100440   1% /run
	none                5120       0      5120   0% /run/lock
	none              253536     260    253276   1% /run/shm
	$ src/mountpoint -d /
	8:1
	$ ls -l /dev/sda1
	brw-rw---- 1 root disk 8, 1 Jun 24 16:10 /dev/sda1
	$ 

### 不打印输出任何信息
	$ src/mountpoint -q /
	$ 



测试 runlevel 命令
-------------------------

### 查看当前运行级别
	$ runlevel -v
	N 2
	$ 

测试 sulogin 命令
-------------------------

### 以超级用户登录
	$ sulogin 
	sulogin: only root can run sulogin.
	$ sudo sulogin 
	[sudo] password for akaedu: 
	root@ubuntu:~# ls
	Desktop  Documents  Downloads  Music  Pictures  Public  Templates  Videos
	root@ubuntu:~# pwd
	/root
	root@ubuntu:~# 

测试 /proc 文件系统命令
-------------------------

### 查看 smbd 的进程号

	$ ps aux |grep smb
	root       845  0.0  0.2  21404  1156 ?        Ss   Jun27   0:02 smbd -F
	root       859  0.0  0.0  21508   156 ?        S    Jun27   0:00 smbd -F
	akaedu   32393  0.0  0.1   4388   840 pts/1    S+   08:54   0:00 grep --color=auto smb

### 查看 proc 文件系统中相关进程信息

	$ sudo cat /proc/845/stat
	845 (smbd) S 1 845 845 0 -1 4202752 22500 308803 1868 2976 60 237 105 662 20 0 1 0 652 21917696 289 4294967295 3068432384 3078609744 3214059904 3214058768 3068290084 0 6272 0 83553 3239400174 0 0 17 0 0 0 383 0 0
	$ 

	$ sudo cat /proc/845/cmdline 
	smbd-F$
	
### 使用 stat 查看文件节点信息

	$ stat /proc/845/exe
	  File: `/proc/845/exe'stat: cannot read symbolic link `/proc/845/exe': Permission denied
	  Size: 0         	Blocks: 0          IO Block: 1024   symbolic link
	Device: 3h/3d	Inode: 209481      Links: 1
	Access: (0777/lrwxrwxrwx)  Uid: (    0/    root)   Gid: (    0/    root)
	Access: 2013-06-28 08:54:40.836988876 +0800
	Modify: 2013-06-28 08:54:28.644988994 +0800
	Change: 2013-06-28 08:54:28.644988994 +0800
	 Birth: -
	$ 


测试 pidof 命令
-------------------------

### 使用 pidof 查看进程号

	$ pidof smbd
	859 845
	$ 

### 使用 -o omitpid 过滤进程

	$ pidof -o 845 smbd
	859
	$ 

### 使用完整路径名

	$ ps aux | grep smbd
	root       845  0.0  0.2  21404  1212 ?        Ss   Jun27   0:03 smbd -F
	root       859  0.0  0.0  21508   376 ?        S    Jun27   0:00 smbd -F
	akaedu    2356  0.0  0.1   4388   836 pts/0    S+   14:58   0:00 grep --color=auto smbd
	$ 

	$ pidof /bin/smbd
	859 845
	$ pidof /sbin/smbd
	859 845
	$ pidof /etc/smbd
	859 845
	$ 

可以看到 即使传入参数中有路径，也不影响输出结果，都能够实现匹配

### 测试进程名称中有路径的情况

	$ ps aux | grep sh
	akaedu   16768  0.0  0.0   2232     4 tty2     S+   Jun27   0:00 /bin/sh /usr/bin/startx
	akaedu   16818  0.0  0.0      0     0 tty2     Z    Jun27   0:00 [sh] <defunct>

	$ pidof sh
	16818 16768
	$ pidof /sbin/sh
	16818
	$ 

可以看到，如果进程名称中有路径，则只能匹配一个。


测试 bootlogd 命令
-------------------------

### 修改配置文件，启动时允许进行 bootlogd 命令

	$ sudo vi /etc/default/bootlogd 
	  1 # Run bootlogd at startup ?
	  2 BOOTLOGD_ENABLE=yes

	$ cat /etc/default/bootlogd 
	# Run bootlogd at startup ?
	BOOTLOGD_ENABLE=yes
	$ 








init命令的大致工作流程如下：

首先，由于init本身具有两面性(既是init，又是telinit)，因此init通过检查自己的进程号来判断自己是 init 还是 telinit ；真正的 init 的进程号(pid)永远都是 1。此外，用户还可通过参数-i，或者—init明确指定强制执行init（源码中有相关处理，但是man page没有给出说明）。

如果init发现要执行的是telinit，它会调用telinit()函数:
	if (!isinit) exit(telinit(p, argc, argv));
telinit()函数的原型如下：

int telinit(char *progname, int argc, char **argv);

实际调用telinit()函数时，是将用户的输入参数列表完全传递给telinit()函数的。在执行telinit时，实际上是通过向INIT_FIFO（/dev/initctl）写入命令的方式，通知init执行相应的操作。Telinit()根据不同请求，构造如下结构体类型的变量并向INIT_FIFO（/dev/initctl）写入该请求来完成其使命：

struct init_request {
	int	magic;			/* Magic number                 */
	int	cmd;			/* What kind of request         */
	int	runlevel;		/* Runlevel to change to        */
	int	sleeptime;		/* Time between TERM and KILL   */
	union {
		struct init_request_bsd	bsd;
		char			data[368];
	} i;
};

如果执行的是真正的init，则又分为两种情形：

对init的重新执行（re-exec）
标准init的执行（首次执行）

在从判断是否telinit()之后的第一步就是检查是否是对init的重新执行（re-exec）（通过读取STATE_PIPE，看是否收到一个Signature = "12567362"的字符串来确定）。如果是re-exec，则继续从STATE_PIPE读取完整的state信息（这些信息被保存在CHILD类型的链表family上），然后调用init_main()来重新执行init(注意，这里没有对/etc/inittab进行解析，这也就是re-exec的特点)。下面在对标准init的执行过程的描述中会谈到如何发起对init的重新执行。

如果不是对init的重新执行（re-exec），则是标准init的执行（首次执行）。
首先，会通过检查命令参数，设置dfl_level，emerg_shell变量，如果参数有-a,auto的话，还会设置环境变量AUTOBOOT=YES。

如果sysvinit编译时使能了SELINUX，即定义了WITH_SELINUX，则首先检查SELINUX_INIT是否被设置。如果SELINUX_INIT未被设置，则装载/proc文件系统（实际上这是为了确保/proc文件系统已经被装载上）。之后用is_selinux_enabled()判断是否系统真的使能了SELINUX。如果是的话，则卸载掉/proc文件系统，然后再调用selinux_init_load_policy()加载策略，并在成功时调用execv()再执行init；否则若SELINUX处于强制模式，则输出警告消息“Unable to load SELinux Policy…”并退出。此处似乎有一个问题，参加下面的链接：

http://us.generation-nt.com/answer/bug-580272-sysvinit-2-88-selinux-policy-help-198006521.html

在进行前面的一系列检测之后，最终开始调用init_main()进入标准的init主函数。下面对该函数做初步分析。

首先，会通过调用reboot(RB_DISABLE_CAD)禁止标准的CTRL-ALT-DEL组合键的响应，从而当按下这个组合键时，会发送SIGINT给init进程，让init来进一步决定采取何种动作（负责该组合键会导致系统直接重启）。

接着，安装一些默认的信号处理函数，包括：

signal_handler(),处理SIGALRM，SIGHUP，SIGINT，SIGPWR，SIGWINCH，SIGUSR1
chld_handler()，处理SIGCHLD
stop_handler()，处理SIGSTOP，SIGTSTP
cont_handler()，处理SIGCONT
segv_handler()，处理SIGSEGV

再之后，考虑首次运行init的情形（reload=0），init_main()会初始化终端，并对终端进行一些默认的设置（在console_stty()函数中通过tcsetattr()实现），设置有一些快捷键，例如：
ctrl+d 退出登陆，等效于logout命令
ctrl+c 杀死应用程序
ctrl+s 暂停应用程序运行，可用ctrl+q恢复运行
ctrl+z 挂起应用程序，此时ps显示进程状态变为T

紧接着，init_main()设置PATH环境变量，并初始化/var/run/utmp。如果emerg_shell被设置（参数中有-b或者emergency），表示需要启动Emergency shell，则通过调用spawn()初始化Emergency shell子进程，并等待该子进程退出。

当从Emergency shell退出（或者不需要Emergency shell的话），init_main()会调用read_inittab()来读入/etc/inittab文件。该函数主要将/etc/inittab文件解析的结果存入CHILD类型的链表family上，供之后的执行使用。

紧接着，调用start_if_needed()，启动需要在相应运行级别中运行的程序和服务。而该函数主要又是通过调用startup()函数，继而调用spawn()来启动程序或者服务的运行的。

在此之后，init_main()进入其主循环，该循环大致如下：
while(1)
{
    /* See if we need to make the boot transitions. */
     boot_transitions();
     /* Check if there are processes to be waited on. */
     for(ch = family; ch; ch = ch->next)
	    if ((ch->flags & RUNNING) && ch->action != BOOT) break;
     if (ch != NULL && got_signals == 0) check_init_fifo();
     /* Check the 'failing' flags */
     fail_check();
     /* Process any signals. */
     process_signals();
     /* See what we need to start up (again) */
     start_if_needed();
}

该主循环的大致功能是，先判断是否有需要切换运行级别，然后等待需要被等待退出的进程退出；并检测是否有任何失败情形并发出警告；之后处理接收到的信号（检查got_signals）；然后再看有没有需要被启动的程序或者服务。

下面是对上述循环中的一些需要注意的特殊点的描述。

	1. 对于首次运行,上述代码中会调用get_init_default()，解析/etc/inittab文件查找是否有 initdefault 记录。 initdefault 记录决定系统初始运行级别。如果没有这条记录，就调用ask_runlevel()，让用户在系统控制台输入想要进入的运行级别。此后，init会解析/etc/inittab 文件中的各个条目并执行相应操作。

	2. 在正常运行期间，也会对/etc/inittab 文件重新扫描，当发现runlevel为‘U’时，便会调用re_exec()；而该函数实际上会创建STATE_PIPE，并向STATE_PIPE写入Signature = "12567362"，接着fork()出一个子进程，通过子进程向STATE_PIPE写入父进程（当前init进程）的状态信息；接着，父进程调用execle()重新执行init程序，并且传递参数“--init”,也就是强制init重新执行。而这个重新执行的init进程，就会进入前面的re-exec一段代码（见前面的分析），从而无需做初始化就能调用init_main()。

	3. 运行级别 S 或 s 把系统带入单用户模式，此模式不需要 /etc/initttab 文件。单用户模式中， /sbin/sulogin 会在 /dev/console 这个设备上打开。

	4. 当第一次进入多用户模式时，init 会执行boot 和 bootwait 记录以便在用户可以登录之前挂载文件系统。然后再执行相应指定的各进程。

	5. 当调用spawn()启动新进程时， init 会检查是否存在 /etc/initscript 文件。如果存在该文件，则使用该脚本来启动该进程。

	6) 如果系统中存在文件 /var/run/utmp 和 /var/log/wtmp，那么当每个子进程终止时，init 会将终止信息和原因记录进这两个文件中。

	7) 当 init 启动了所有指定的子进程后，它会不断地监测系统进程情况，例如：某个子进程被终止、电源失效、或由 telinit 发出的改变运行级别的信号。当它接收到以上的这些信号时，会自动重新扫描 /etc/inittab 文件，并执行相应操作。因此，新的记录可以随时加入到/etc/inittab文件中。在更新了各种系统文件后，如果希望及时更新，就可以使用telinit Q 或 q 命令来唤醒 init 让它即刻重新检测/etc/inittab 文件。

	8) 当 init 得到更新运行级别的请求， init会向所有没有在新运行级别中定义的进程发送一个警告信号 SIGTERM 。在等待 5 秒钟之后，它会发出的信号 SIGKILL（强制中断所有进程的运行）。init 假设所有的这些进程（包括它们的后代）都仍然在 init 最初创建它们的同一进程组里。如果有进程改变了自己的进程组，那么它就收不到这些信号。这样的进程，就需要分别进行手工终止。



	<mydbug> begin to call init_main
	<mydebug> init_main()
	<mydebug> console init ok
	<mydebug> reload = 0 
	<mydebug> 0 
	<mydebug> 1 
	<mydebug> 2 
	<mydebug> reload = 0 
	<mydebug> reload = 0 
	<mydbug> log buf = version 2.88 booting
	<mydbug> begin to read_inittab()
	<mydebug> buf = id:1:initdefault:

	<mydebug> id = id
	<mydebug> rlevel = 1
	<mydebug> action = initdefault
	<mydebug> process = 
	<mydebug> ch->id = id
	<mydebug> ch->process = 
	<mydebug> buf = 

	<mydebug> buf = rc::bootwait:/bin/date

	<mydebug> id = rc
	<mydebug> rlevel = 
	<mydebug> action = bootwait
	<mydebug> process = /bin/date
	<mydebug> ch->id = rc
	<mydebug> ch->process = /bin/date
	<mydebug> buf = 

	<mydebug> buf = 1:1:respawn:/etc/getty 9600 tty1

	<mydebug> id = 1
	<mydebug> rlevel = 1
	<mydebug> action = respawn
	<mydebug> process = /etc/getty 9600 tty1
	<mydebug> ch->id = 1
	<mydebug> ch->process = /etc/getty 9600 tty1
	<mydebug> buf = 

	<mydebug> buf = 2:1:respawn:/etc/getty 9600 tty2

	<mydebug> id = 2
	<mydebug> rlevel = 1
	<mydebug> action = respawn
	<mydebug> process = /etc/getty 9600 tty2
	<mydebug> ch->id = 2
	<mydebug> ch->process = /etc/getty 9600 tty2
	<mydebug> buf = 

	<mydebug> buf = 3:1:respawn:/etc/getty 9600 tty3

	<mydebug> id = 3
	<mydebug> rlevel = 1
	<mydebug> action = respawn
	<mydebug> process = /etc/getty 9600 tty3
	<mydebug> ch->id = 3
	<mydebug> ch->process = /etc/getty 9600 tty3
	<mydebug> buf = 

	<mydebug> buf = 4:1:respawn:/etc/getty 9600 tty4

	<mydebug> id = 4
	<mydebug> rlevel = 1
	<mydebug> action = respawn
	<mydebug> process = /etc/getty 9600 tty4
	<mydebug> ch->id = 4
	<mydebug> ch->process = /etc/getty 9600 tty4
	<mydebug> buf = ~~:S:wait:/sbin/sulogin
































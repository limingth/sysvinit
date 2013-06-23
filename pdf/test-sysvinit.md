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
	

#测试 killall5 命令
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





























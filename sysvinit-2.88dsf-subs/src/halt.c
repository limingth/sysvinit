/*
 * Halt		Stop the system running.
 *		It re-enables CTRL-ALT-DEL, so that a hard reboot can
 *		be done. If called as reboot, it will reboot the system.
 *
 *		If the system is not in runlevel 0 or 6, halt will just
 *		execute a "shutdown -h" to halt the system, and reboot will
 *		execute an "shutdown -r". This is for compatibility with
 *		sysvinit 2.4.
 *
 * Usage:	halt [-n] [-w] [-d] [-f] [-h] [-i] [-p]
 *		-n: don't sync before halting the system
 *		-w: only write a wtmp reboot record and exit.
 *		-d: don't write a wtmp record.
 *		-f: force halt/reboot, don't call shutdown.
 *		-h: put harddisks in standby mode
 *		-i: shut down all network interfaces.
 *		-p: power down the system (if possible, otherwise halt).
 *
 *		Reboot and halt are both this program. Reboot
 *		is just a link to halt. Invoking the program
 *		as poweroff implies the -p option.
 *
 * Author:	Miquel van Smoorenburg, miquels@cistron.nl
 *
 * Version:	2.86,  30-Jul-2004
 *
 *		This file is part of the sysvinit suite,
 *		Copyright (C) 1991-2004 Miquel van Smoorenburg.
 *
 *		This program is free software; you can redistribute it and/or modify
 *		it under the terms of the GNU General Public License as published by
 *		the Free Software Foundation; either version 2 of the License, or
 *		(at your option) any later version.
 *
 *		This program is distributed in the hope that it will be useful,
 *		but WITHOUT ANY WARRANTY; without even the implied warranty of
 *		MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *		GNU General Public License for more details.
 *
 *		You should have received a copy of the GNU General Public License
 *		along with this program; if not, write to the Free Software
 *		Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <stdlib.h>
#include <utmp.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/times.h>
#include <time.h>
#include <signal.h>
#include <stdio.h>
#include <getopt.h>
#include "reboot.h"

char *Version = "@(#)halt  2.86  31-Jul-2004 miquels@cistron.nl";
char *progname;

#define KERNEL_MONITOR	1 /* If halt() puts you into the kernel monitor. */
#define RUNLVL_PICKY	0 /* Be picky about the runlevel */

extern int ifdown(void);
extern int hddown(void);
extern int hdflush(void);
extern void write_wtmp(char *user, char *id, int pid, int type, char *line);

/*
 *	Send usage message.
 */
/**                                                                  
 * @attention 本注释得到了"核高基"科技重大专项2012年课题             
 *             “开源操作系统内核分析和安全性评估                     
 *            （课题编号：2012ZX01039-004）”的资助。                 
 *                                                                    
 * @copyright 注释添加单位：清华大学——03任务                         
 *            （Linux内核相关通用基础软件包分析）                     
 *                                                                    
 * @author 注释添加人员： 李明                                       
 *             (电子邮件 <limingth@gmail.com>)                       
 *                                                                    
 * @date 注释添加日期：                                              
 *                      2013-6-1                                      
 *                                                                    
 * @note 注释详细内容:                                                
 *             (注释内容主要参考 sysvinit 项目详细分析文档)           
 *
 * @brief 通过 fprintf() 函数，向标准出错 stderr 打印该条命令的用户使用帮助信息
 */
void usage(void)
{
	fprintf(stderr, "usage: %s [-n] [-w] [-d] [-f] [-h] [-i]%s\n",
		progname, strcmp(progname, "halt") ? "" : " [-p]");
	fprintf(stderr, "\t-n: don't sync before halting the system\n");
	fprintf(stderr, "\t-w: only write a wtmp reboot record and exit.\n");
	fprintf(stderr, "\t-d: don't write a wtmp record.\n");
	fprintf(stderr, "\t-f: force halt/reboot, don't call shutdown.\n");
	fprintf(stderr, "\t-h: put harddisks in standby mode.\n");
	fprintf(stderr, "\t-i: shut down all network interfaces.\n");
	if (!strcmp(progname, "halt"))
		fprintf(stderr, "\t-p: power down the system (if possible, otherwise halt).\n");
	exit(1);
}

/*
 *	See if we were started directly from init.
 *	Get the runlevel from /var/run/utmp or the environment.
 */
int get_runlevel(void)
{
	struct utmp *ut;
	char *r;
#if RUNLVL_PICKY
	time_t boottime;
#endif

	/*
	 *	First see if we were started directly from init.
	 */
	if (getenv("INIT_VERSION") && (r = getenv("RUNLEVEL")) != NULL)
		return *r;

	/*
	 *	Hmm, failed - read runlevel from /var/run/utmp..
	 */
#if RUNLVL_PICKY
	/*
	 *	Get boottime from the kernel.
	 */
	time(&boottime);
	boottime -= (times(NULL) / HZ);
#endif

	/*
	 *	Find runlevel in utmp.
	 */
	setutent();
	while ((ut = getutent()) != NULL) {
#if RUNLVL_PICKY
		/*
		 *	Only accept value if it's from after boottime.
		 */
		if (ut->ut_type == RUN_LVL && ut->ut_time > boottime)
			return (ut->ut_pid & 255);
#else
		if (ut->ut_type == RUN_LVL)
			return (ut->ut_pid & 255);
#endif
	}
	endutent();

	/* This should not happen but warn the user! */
	fprintf(stderr, "WARNING: could not determine runlevel"
		" - doing soft %s\n", progname);
	fprintf(stderr, "  (it's better to use shutdown instead of %s"
		" from the command line)\n", progname);

	return -1;
}

/*
 *	Switch to another runlevel.
 */
/**                                                                  
 * @attention 本注释得到了"核高基"科技重大专项2012年课题             
 *             “开源操作系统内核分析和安全性评估                     
 *            （课题编号：2012ZX01039-004）”的资助。                 
 *                                                                    
 * @copyright 注释添加单位：清华大学——03任务                         
 *            （Linux内核相关通用基础软件包分析）                     
 *                                                                    
 * @author 注释添加人员： 李明                                       
 *             (电子邮件 <limingth@gmail.com>)                       
 *                                                                    
 * @date 注释添加日期：                                              
 *                      2013-6-1                                      
 *                                                                    
 * @note 注释详细内容:                                                
 *             (注释内容主要参考 sysvinit 项目详细分析文档)           
 *
 * @brief 通过 execv 系统调用，执行 shutdown 命令来切换运行级别
 */
void do_shutdown(char *fl, char *tm)
{
	char *args[8];
	int i = 0;

	args[i++] = "shutdown";
	args[i++] = fl;
	if (tm) {
		args[i++] = "-t";
		args[i++] = tm;
	}
	args[i++] = "now";
	args[i++] = NULL;

	execv("/sbin/shutdown", args);
	execv("/etc/shutdown", args);
	execv("/bin/shutdown", args);

	perror("shutdown");
	exit(1);
}

/*
 *	Main program.
 *	Write a wtmp entry and reboot cq. halt.
 */
/**                                                                  
 * @attention 本注释得到了"核高基"科技重大专项2012年课题             
 *             “开源操作系统内核分析和安全性评估                     
 *            （课题编号：2012ZX01039-004）”的资助。                 
 *                                                                    
 * @copyright 注释添加单位：清华大学——03任务                         
 *            （Linux内核相关通用基础软件包分析）                     
 *                                                                    
 * @author 注释添加人员： 李明                                       
 *             (电子邮件 <limingth@gmail.com>)                       
 *                                                                    
 * @date 注释添加日期：                                              
 *                      2013-6-1                                      
 *                                                                    
 * @note 注释详细内容:                                                
 *             (注释内容主要参考 sysvinit 项目详细分析文档)           
 *
 * @brief halt 命令实现的主函数，负责写入 wtmp 日志文件和调用 shutdown 命令来关闭系统
 *
 * @details halt 命令详细用法

   halt 用来停止系统。正常情况下等效于 shutdown 加上 -h 参数(当前系统运行级别是 0 时除外)。它将告诉内核去中止系统，并在系统正在关闭的过程中将日志记录到 /var/log/wtmp 文件里。

	* 命令格式
		/sbin/halt [-n] [-w] [-d] [-f] [-i] [-p] [-h]

	* 主要选项
	 -n  
		reboot或者halt之前，不同步(sync)数据.
	 -w  
		仅仅往/var/log/wtmp里写一个记录，并不实际做reboot或者halt操作.
	 -f  
		强制halt或者reboot，不等其他程序退出或者服务停止就重新启动系统.这样会造成数据丢失，建议一般不要这样做.
	 -i  
		halt或reboot前，关闭所有网络接口.
	 -h  
		halt或poweroff前，使系统中所有的硬件处于等待状态.
	 -p  
		在系统halt同时，做poweroff操作.即停止系统同时关闭电源.

 *
 */




int main(int argc, char **argv)
{
	int do_reboot = 0;
	int do_sync = 1;
	int do_wtmp = 1;
	int do_nothing = 0;
	int do_hard = 0;
	int do_ifdown = 0;
	int do_hddown = 0;
	int do_poweroff = 0;
	int c;
	char *tm = NULL;

	/*
	 *	Find out who we are
	 */
	/* Remove dash passed on in argv[0] when used as login shell. */
	if (argv[0][0] == '-') argv[0]++;
	if ((progname = strrchr(argv[0], '/')) != NULL)
		progname++;
	else
		progname = argv[0];

	if (!strcmp(progname, "reboot")) do_reboot = 1;
	if (!strcmp(progname, "poweroff")) do_poweroff = 1;

	/*
	 *	Get flags
	 */
	while((c = getopt(argc, argv, ":ihdfnpwt:")) != EOF) {
		switch(c) {
			case 'n':
				do_sync = 0;
				do_wtmp = 0;
				break;
			case 'w':
				do_nothing = 1;
				break;
			case 'd':
				do_wtmp = 0;
				break;
			case 'f':
				do_hard = 1;
				break;
			case 'i':
				do_ifdown = 1;
				break;
			case 'h':
				do_hddown = 1;
				break;
			case 'p':
				do_poweroff = 1;
				break;
			case 't':
				tm = optarg;
				break;
			default:
				usage();
		}
	 }
	if (argc != optind) usage();

	if (geteuid() != 0) {
		fprintf(stderr, "%s: must be superuser.\n", progname);
		exit(1);
	}

	(void)chdir("/");

	if (!do_hard && !do_nothing) {
		/*
		 *	See if we are in runlevel 0 or 6.
		 */
		c = get_runlevel();
		if (c != '0' && c != '6')
			do_shutdown(do_reboot ? "-r" : "-h", tm);
	}

	/*
	 *	Record the fact that we're going down
	 */
	if (do_wtmp)
		write_wtmp("shutdown", "~~", 0, RUN_LVL, "~~");

	/*
	 *	Exit if all we wanted to do was write a wtmp record.
	 */
	if (do_nothing && !do_hddown && !do_ifdown) exit(0);

	if (do_sync) {
		sync();
		sleep(2);
	}

	if (do_ifdown)
		(void)ifdown();

	if (do_hddown)
		(void)hddown();
	else
		(void)hdflush();

	if (do_nothing) exit(0);

	if (do_reboot) {
		init_reboot(BMAGIC_REBOOT);
	} else {
		/*
		 *	Turn on hard reboot, CTRL-ALT-DEL will reboot now
		 */
#ifdef BMAGIC_HARD
		init_reboot(BMAGIC_HARD);
#endif

		/*
		 *	Stop init; it is insensitive to the signals sent
		 *	by the kernel.
		 */
		kill(1, SIGTSTP);

		/*
		 *	Halt or poweroff.
		 */
		if (do_poweroff)
			init_reboot(BMAGIC_POWEROFF);
		/*
		 *	Fallthrough if failed.
		 */
		init_reboot(BMAGIC_HALT);
	}

	/*
	 *	If we return, we (c)ontinued from the kernel monitor.
	 */
#ifdef BMAGIC_SOFT
	init_reboot(BMAGIC_SOFT);
#endif
	kill(1, SIGCONT);

	exit(0);
}

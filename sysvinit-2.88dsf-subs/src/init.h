/*
 * init.h	Several defines and declarations to be
 *		included by all modules of the init program.
 *
 * Version:	@(#)init.h  2.85-5  02-Jul-2003  miquels@cistron.nl
 *
 * Copyright (C) 1998-2003 Miquel van Smoorenburg.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

/* Standard configuration */
#define CHANGE_WAIT 0			/* Change runlevel while
					   waiting for a process to exit? */
/* Debug and test modes */
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
 * @brief 该宏定义主要用来对代码中的调试输出打印语句，进行条件编译控制，如果需要打印调试信息，则将此项置为 1
 */
#define DEBUG	   1			/* Debug code off */
#define INITDEBUG  0			/* Fork at startup to debug init. */

/* Some constants */
#define INITPID	   1			/* pid of first process */
#define PIPE_FD    10			/* Fileno of initfifo. */
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
 * @brief 该宏定义主要用来给 init 进程通过 re-exec 方式的父子进程之间用 state pipe 管道通信时的 fd 号
 */
#define STATE_PIPE 11			/* used to pass state through exec */

/* Failsafe configuration */
#define MAXSPAWN   10			/* Max times respawned in.. */
#define TESTTIME   120			/* this much seconds */
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
 * @brief 睡眠时间超过300秒=5分钟的进程，将会被清除标志位
 */
#define SLEEPTIME  300			/* Disable time */

/* Default path inherited by every child. */
#define PATH_DEFAULT   "/sbin:/usr/sbin:/bin:/usr/bin"


/* Prototypes. */
void write_utmp_wtmp(char *user, char *id, int pid, int type, char *line);
void write_wtmp(char *user, char *id, int pid, int type, char *line);
#ifdef __GNUC__
__attribute__ ((format (printf, 2, 3)))
#endif
void initlog(int loglevel, char *fmt, ...);
void set_term(int how);
void print(char *fmt);

/* from dowall.c */
void wall(const char *text, int remote);

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
 * @brief 该宏定义主要用来打印 init 进程运行过程中的调试信息，是否打印取决于 DEBUG 宏开关
 */
#if DEBUG
#  define INITDBG(level, fmt, args...) initlog(level, fmt, ##args)
#else
#  define INITDBG(level, fmt, args...)
#endif
#undef INITDBG
#define INITDBG(level, fmt, args...) printf(fmt "\n", ##args)

/* Actions to be taken by init */
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
 * @brief 该宏定义主要用来标识在解析 /etc/inittab 文件时，不同 runlevel 对应要采取的不同动作
 */
#define RESPAWN			1
#define WAIT			2
#define ONCE			3
#define	BOOT			4
#define BOOTWAIT		5
#define POWERFAIL		6
#define POWERWAIT		7
#define POWEROKWAIT		8
#define CTRLALTDEL		9
#define OFF		       10
#define	ONDEMAND	       11
#define	INITDEFAULT	       12
#define SYSINIT		       13
#define POWERFAILNOW           14
#define KBREQUEST               15

/* Information about a process in the in-core inittab */
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
 * @brief 这个链表数据结构在 init.h 头文件中，是实现根据 init 运行级别加载不同用户程序的最重要的数据结构。
 *
 */

	
typedef struct _child_ {
  int flags;			/* Status of this entry */
  int exstat;			/* Exit status of process */
  int pid;			/* Pid of this process */
  time_t tm;			/* When respawned last */
  int count;			/* Times respawned in the last 2 minutes */
  char id[8];			/* Inittab id (must be unique) */
  char rlevel[12];		/* run levels */
  int action;			/* what to do (see list below) */
  char process[128];		/* The command line */
  struct _child_ *new;		/* New entry (after inittab re-read) */
  struct _child_ *next;		/* For the linked list */
} CHILD;

/* Values for the 'flags' field */
#define RUNNING			2	/* Process is still running */
#define KILLME			4	/* Kill this process */
#define DEMAND			8	/* "runlevels" a b c */
#define FAILING			16	/* process respawns rapidly */
#define WAITING			32	/* We're waiting for this process */
#define ZOMBIE			64	/* This process is already dead */
#define XECUTED		128	/* Set if spawned once or more times */

/* Log levels. */
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
 * @brief 该宏定义主要用来标识输出打印信息时候的级别。包括输出到控制台和输出到系统日志两种。
 */
#define L_CO	1		/* Log on the console. */
#define L_SY	2		/* Log with syslog() */
#define L_VB	(L_CO|L_SY)	/* Log with both. */

#ifndef NO_PROCESS
#  define NO_PROCESS 0
#endif

/*
 *	Global variables.
 */
extern CHILD *family;
extern int wrote_wtmp_reboot;
extern int wrote_utmp_reboot;
extern int wrote_wtmp_rlevel;
extern int wrote_utmp_rlevel;
extern char thislevel;
extern char prevlevel;

/* Tokens in state parser */
#define C_VER		1
#define	C_END		2
#define C_REC		3
#define	C_EOR		4
#define	C_LEV		5
#define C_FLAG		6
#define	C_ACTION	7
#define C_PROCESS	8
#define C_PID		9
#define C_EXS	       10
#define C_EOF          -1
#define D_RUNLEVEL     -2
#define D_THISLEVEL    -3
#define D_PREVLEVEL    -4
#define D_GOTSIGN      -5
#define D_WROTE_WTMP_REBOOT -6
#define D_WROTE_UTMP_REBOOT -7
#define D_SLTIME       -8
#define D_DIDBOOT      -9
#define D_WROTE_WTMP_RLEVEL -16
#define D_WROTE_UTMP_RLEVEL -17


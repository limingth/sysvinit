/*
 * Init		A System-V Init Clone.
 *
 * Usage:	/sbin/init
 *		     init [0123456SsQqAaBbCc]
 *		  telinit [0123456SsQqAaBbCc]
 *
 * Version:	@(#)init.c  2.86  30-Jul-2004  miquels@cistron.nl
 */
#define VERSION "2.88"
#define DATE    "26-Mar-2010"
/*
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
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#ifdef __linux__
#include <sys/kd.h>
#endif
#include <sys/resource.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <termios.h>
#include <utmp.h>
#include <ctype.h>
#include <stdarg.h>
#include <sys/syslog.h>
#include <sys/time.h>

#ifdef WITH_SELINUX
#  include <selinux/selinux.h>
#  include <sys/mount.h>
#  ifndef MNT_DETACH /* present in glibc 2.10, missing in 2.7 */
#    define MNT_DETACH 2
#  endif
#endif

#ifdef __i386__
#  ifdef __GLIBC__
     /* GNU libc 2.x */
#    define STACK_DEBUG 1
#    if (__GLIBC__ == 2 && __GLIBC_MINOR__ == 0)
       /* Only glibc 2.0 needs this */
#      include <sigcontext.h>
#    elif ( __GLIBC__ > 2) && ((__GLIBC__ == 2) && (__GLIBC_MINOR__ >= 1))
#      include <bits/sigcontext.h>
#    endif
#  endif
#endif

#include "init.h"
#include "initreq.h"
#include "paths.h"
#include "reboot.h"
#include "set.h"

#ifndef SIGPWR
#  define SIGPWR SIGUSR2
#endif

#ifndef CBAUD
#  define CBAUD		0
#endif
#ifndef CBAUDEX
#  define CBAUDEX	0
#endif

/* Set a signal handler. */
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
 * @brief 该宏定义主要完成注册一个信号处理函数，包含了一组语句，最后通过 sigaction() 函数完成注册
 */

#define SETSIG(sa, sig, fun, flags) \
		do { \
			sa.sa_handler = fun; \
			sa.sa_flags = flags; \
			sigemptyset(&sa.sa_mask); \
			sigaction(sig, &sa, NULL); \
		} while(0)
/* add by limingth */		
#undef SETSIG
#define SETSIG(sa, sig, fun, flags)     INITDBG(L_VB, "setsig %s : %s\t", #sig, #fun)


/* Version information */
char *Version = "@(#) init " VERSION "  " DATE "  miquels@cistron.nl";
char *bootmsg = "version " VERSION " %s";
#define E_VERSION "INIT_VERSION=sysvinit-" VERSION

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
 * @brief 该全局变量是一个链表头指针，用于保存上一次分析 inittab 文件后形成的 init 要加载子进程的列表
 */

CHILD *family = NULL;		/* The linked list of all entries */

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
 * @brief 该全局变量是一个链表头指针，用于保存上一次分析 inittab 文件后形成的 init 要加载子进程的列表
 */

CHILD *newFamily = NULL;	/* The list after inittab re-read */

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
 * @brief 该全局变量是用于定义应急shell Emergency shell 使用 /sbin/sulogin
 */

CHILD ch_emerg = {		/* Emergency shell */
	WAITING, 0, 0, 0, 0,
	"~~",
	"S",
	3,
	"/sbin/sulogin",
	NULL,
	NULL
};

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
 * @brief 该全局变量是保存当次 init 进程启动时的 runlevel 
 */

char runlevel = 'S';		/* The current run level */
char thislevel = 'S';		/* The current runlevel */
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
 * @brief 该全局变量是保存上一次 init 进程启动时的 runlevel 
 */

char prevlevel = 'N';		/* Previous runlevel */
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
 * @brief 该全局变量是保存从 inittab 文件中获得的，也可以是用户输入的，默认 runlevel 
 */

int dfl_level = 0;		/* Default runlevel */
sig_atomic_t got_cont = 0;	/* Set if we received the SIGCONT signal */
sig_atomic_t got_signals;	/* Set if we received a signal. */
int emerg_shell = 0;		/* Start emergency shell? */
int wrote_wtmp_reboot = 1;	/* Set when we wrote the reboot record */
int wrote_utmp_reboot = 1;	/* Set when we wrote the reboot record */
int wrote_wtmp_rlevel = 1;	/* Set when we wrote the runlevel record */
int wrote_utmp_rlevel = 1;	/* Set when we wrote the runlevel record */
int sltime = 5;			/* Sleep time between TERM and KILL */
char *argv0;			/* First arguments; show up in ps listing */
int maxproclen;			/* Maximal length of argv[0] with \0 */
struct utmp utproto;		/* Only used for sizeof(utproto.ut_id) */
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
 * @brief 该全局变量是一个字符串指针，用来记录控制台设备文件名
 */

char *console_dev;		/* Console device. */
int pipe_fd = -1;		/* /dev/initctl */
int did_boot = 0;		/* Did we already do BOOT* stuff? */
int main(int, char **);

/*	Used by re-exec part */
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
 * @brief 该全局变量是一个整型数，用来表示本次 init 进程启动是否属于 reload 方式，也就是通过 re-exec 创建的二次启动
 */

int reload = 0;			/* Should we do initialization stuff? */
char *myname="/sbin/init";	/* What should we exec */
int oops_error;			/* Used by some of the re-exec code. */
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
 * @brief 该全局变量是一个字符串用于签名验证，通过 re-exec 创建的 init 二次启动时会通过 STATE_PIPE 写入信息，其中就包括这个签名
 */

const char *Signature = "12567362";	/* Signature for re-exec fd */

/* Macro to see if this is a special action */
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
 * @brief 该宏定义带参数，用来判断传入的参数是否是一个特殊的动作
 */

#define ISPOWER(i) ((i) == POWERWAIT || (i) == POWERFAIL || \
		    (i) == POWEROKWAIT || (i) == POWERFAILNOW || \
		    (i) == CTRLALTDEL)

/* ascii values for the `action' field. */
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
 * @brief 这个数组保存的都是常量，包括常量字符串和宏定义，主要是一组对应关系，方便把 /etc/inittab 文件中的字符串转换为整型数。
 *
 */

struct actions {
  char *name;
  int act;
} actions[] = {
  { "respawn", 	   RESPAWN	},
  { "wait",	   WAIT		},
  { "once",	   ONCE		},
  { "boot",	   BOOT		},
  { "bootwait",	   BOOTWAIT	},
  { "powerfail",   POWERFAIL	},
  { "powerfailnow",POWERFAILNOW },
  { "powerwait",   POWERWAIT	},
  { "powerokwait", POWEROKWAIT	},
  { "ctrlaltdel",  CTRLALTDEL	},
  { "off",	   OFF		},
  { "ondemand",	   ONDEMAND	},
  { "initdefault", INITDEFAULT	},
  { "sysinit",	   SYSINIT	},
  { "kbrequest",   KBREQUEST    },
  { NULL,	   0		},
};

/*
 *	State parser token table (see receive_state)
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
 * @brief 该全局变量是一个结构体数组，用于在 receive_state 函数中判断的标识符表
 */

struct {
  char name[4];	
  int cmd;
} cmds[] = {
  { "VER", 	   C_VER	},
  { "END",	   C_END	},
  { "REC",	   C_REC	},
  { "EOR",	   C_EOR	},
  { "LEV",	   C_LEV	},
  { "FL ",	   C_FLAG	},
  { "AC ",	   C_ACTION	},
  { "CMD",	   C_PROCESS	},
  { "PID",	   C_PID	},
  { "EXS",	   C_EXS	},
  { "-RL",	   D_RUNLEVEL	},
  { "-TL",	   D_THISLEVEL	},
  { "-PL",	   D_PREVLEVEL	},
  { "-SI",	   D_GOTSIGN	},
  { "-WR",	   D_WROTE_WTMP_REBOOT},
  { "-WU",	   D_WROTE_UTMP_REBOOT},
  { "-ST",	   D_SLTIME	},
  { "-DB",	   D_DIDBOOT	},
  { "-LW",	   D_WROTE_WTMP_RLEVEL},
  { "-LU",	   D_WROTE_UTMP_RLEVEL},
  { "",	   	   0		}
};
struct {
	char *name;
	int mask;
} flags[]={
	{"RU",RUNNING},
	{"DE",DEMAND},
	{"XD",XECUTED},
	{"WT",WAITING},
	{NULL,0}
};

#define NR_EXTRA_ENV	16
char *extra_env[NR_EXTRA_ENV];


/*
 *	Sleep a number of seconds.
 *
 *	This only works correctly because the linux select updates
 *	the elapsed time in the struct timeval passed to select!
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
 * @brief 该函数使得进程睡眠 sec 秒
 */
static
void do_sleep(int sec)
{
	struct timeval tv;

	tv.tv_sec = sec;
	tv.tv_usec = 0;

	while(select(0, NULL, NULL, NULL, &tv) < 0 && errno == EINTR)
		;
}


/*
 *	Non-failing allocation routines (init cannot fail).
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
 * @brief 该函数用于给该项目范围的代码，分配内存空间，可以看成是确保一定能够分配成功的 malloc 函数
 */
static
void *imalloc(size_t size)
{
	void	*m;

	while ((m = malloc(size)) == NULL) {
		initlog(L_VB, "out of memory");
		do_sleep(5);
	}
	memset(m, 0, size);
	return m;
}

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
 * @brief 该函数为传入的字符串 s 复制一个新的相同的字符串传出。内部需要用到 imalloc 函数为新的字符串分配空间
 */

static
char *istrdup(char *s)
{
	char	*m;
	int	l;

	l = strlen(s) + 1;
	m = imalloc(l);
	memcpy(m, s, l);
	return m;
}


/*
 *	Send the state info of the previous running init to
 *	the new one, in a version-independant way.
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
 * @brief 该函数用于给之前运行的 init 进程发送一个 state 状态信息
 */
static
void send_state(int fd)
{
	FILE	*fp;
	CHILD	*p;
	int	i,val;

	fp = fdopen(fd,"w");

	fprintf(fp, "VER%s\n", Version);
	fprintf(fp, "-RL%c\n", runlevel);
	fprintf(fp, "-TL%c\n", thislevel);
	fprintf(fp, "-PL%c\n", prevlevel);
	fprintf(fp, "-SI%u\n", got_signals);
	fprintf(fp, "-WR%d\n", wrote_wtmp_reboot);
	fprintf(fp, "-WU%d\n", wrote_utmp_reboot);
	fprintf(fp, "-ST%d\n", sltime);
	fprintf(fp, "-DB%d\n", did_boot);

	for (p = family; p; p = p->next) {
		fprintf(fp, "REC%s\n", p->id);
		fprintf(fp, "LEV%s\n", p->rlevel);
		for (i = 0, val = p->flags; flags[i].mask; i++)
			if (val & flags[i].mask) {
				val &= ~flags[i].mask;
				fprintf(fp, "FL %s\n",flags[i].name);
			}
		fprintf(fp, "PID%d\n",p->pid);
		fprintf(fp, "EXS%u\n",p->exstat);
		for(i = 0; actions[i].act; i++)
			if (actions[i].act == p->action) {
				fprintf(fp, "AC %s\n", actions[i].name);
				break;
			}
		fprintf(fp, "CMD%s\n", p->process);
		fprintf(fp, "EOR\n");
	}
	fprintf(fp, "END\n");
	fclose(fp);
}

/*
 *	Read a string from a file descriptor.
 *	FIXME: why not use fgets() ?
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
 * @brief 该函数从给定的文件指针 f 上，读取一行字符串（碰到 \n 返回）。
 */

static int get_string(char *p, int size, FILE *f)
{
	int	c;

	while ((c = getc(f)) != EOF && c != '\n') {
		if (--size > 0)
			*p++ = c;
	}
	*p = '\0';
	return (c != EOF) && (size > 0);
}

/*
 *	Read trailing data from the state pipe until we see a newline.
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
 * @brief 该函数从给定的文件指针 f 上，读取一行字符串（碰到 \n 返回），读取的内容不保存也不返回，直接忽略。
 */

static int get_void(FILE *f)
{
	int	c;

	while ((c = getc(f)) != EOF && c != '\n')
		;

	return (c != EOF);
}

/*
 *	Read the next "command" from the state pipe.
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
 * @brief 该函数用于从 state pipe 管道中读取一个 command 指令
 */
static int get_cmd(FILE *f)
{
	char	cmd[4] = "   ";
	int	i;

	if (fread(cmd, 1, sizeof(cmd) - 1, f) != sizeof(cmd) - 1)
		return C_EOF;

	for(i = 0; cmds[i].cmd && strcmp(cmds[i].name, cmd) != 0; i++)
		;
	return cmds[i].cmd;
}

/*
 *	Read a CHILD * from the state pipe.
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
 * @brief 该函数用于从 state pipe 管道中读取一个 CHILD 结构体的数据，如果成功则返回这条记录的指针
 */
static CHILD *get_record(FILE *f)
{
	int	cmd;
	char	s[32];
	int	i;
	CHILD	*p;

	do {
		switch (cmd = get_cmd(f)) {
			case C_END:
				get_void(f);
				return NULL;
			case 0:
				get_void(f);
				break;
			case C_REC:
				break;
			case D_RUNLEVEL:
				fscanf(f, "%c\n", &runlevel);
				break;
			case D_THISLEVEL:
				fscanf(f, "%c\n", &thislevel);
				break;
			case D_PREVLEVEL:
				fscanf(f, "%c\n", &prevlevel);
				break;
			case D_GOTSIGN:
				fscanf(f, "%u\n", &got_signals);
				break;
			case D_WROTE_WTMP_REBOOT:
				fscanf(f, "%d\n", &wrote_wtmp_reboot);
				break;
			case D_WROTE_UTMP_REBOOT:
				fscanf(f, "%d\n", &wrote_utmp_reboot);
				break;
			case D_SLTIME:
				fscanf(f, "%d\n", &sltime);
				break;
			case D_DIDBOOT:
				fscanf(f, "%d\n", &did_boot);
				break;
			case D_WROTE_WTMP_RLEVEL:
				fscanf(f, "%d\n", &wrote_wtmp_rlevel);
				break;
			case D_WROTE_UTMP_RLEVEL:
				fscanf(f, "%d\n", &wrote_utmp_rlevel);
				break;
			default:
				if (cmd > 0 || cmd == C_EOF) {
					oops_error = -1;
					return NULL;
				}
		}
	} while (cmd != C_REC);

	p = imalloc(sizeof(CHILD));
	get_string(p->id, sizeof(p->id), f);

	do switch(cmd = get_cmd(f)) {
		case 0:
		case C_EOR:
			get_void(f);
			break;
		case C_PID:
			fscanf(f, "%d\n", &(p->pid));
			break;
		case C_EXS:
			fscanf(f, "%u\n", &(p->exstat));
			break;
		case C_LEV:
			get_string(p->rlevel, sizeof(p->rlevel), f);
			break;
		case C_PROCESS:
			get_string(p->process, sizeof(p->process), f);
			break;
		case C_FLAG:
			get_string(s, sizeof(s), f);
			for(i = 0; flags[i].name; i++) {
				if (strcmp(flags[i].name,s) == 0)
					break;
			}
			p->flags |= flags[i].mask;
			break;
		case C_ACTION:
			get_string(s, sizeof(s), f);
			for(i = 0; actions[i].name; i++) {
				if (strcmp(actions[i].name, s) == 0)
					break;
			}
			p->action = actions[i].act ? actions[i].act : OFF;
			break;
		default:
			free(p);
			oops_error = -1;
			return NULL;
	} while( cmd != C_EOR);

	return p;
}

/*
 *	Read the complete state info from the state pipe.
 *	Returns 0 on success
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
 * @brief 该函数用于从 state pipe 管道中读取一条完整的记录信息，如果成功则返回0
 */
static
int receive_state(int fd)
{
	FILE	*f;
	char	old_version[256];
	CHILD	**pp;

	f = fdopen(fd, "r");

 	if (get_cmd(f) != C_VER)
		return -1;
	get_string(old_version, sizeof(old_version), f);
	oops_error = 0;
	for (pp = &family; (*pp = get_record(f)) != NULL; pp = &((*pp)->next))
		;
	fclose(f);
	return oops_error;
}

/*
 *	Set the process title.
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
 * @brief 设置进程 process 的名称 title
 */
#ifdef __GNUC__
__attribute__ ((format (printf, 1, 2)))
#endif
static int setproctitle(char *fmt, ...)
{
	va_list ap;
	int len;
	char buf[256];

	buf[0] = 0;

	va_start(ap, fmt);
	len = vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	if (maxproclen > 1) {
		memset(argv0, 0, maxproclen);
		strncpy(argv0, buf, maxproclen - 1);
	}

	return len;
}

/*
 *	Set console_dev to a working console.
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
 * @brief 设置 console_dev 变量为一个可以工作的 console
 *
 * @details 函数执行流程分析：

	1. 获取 CONSOLE 环境变量的值，赋值给 console_dev 全局变量（ char * 类型 ）。

	2. 以只读非阻塞方式打开 console_dev 所代表的设备文件。

	3. 初始化成功，则关闭该设备文件；如果失败，则将 console_dev 置为 /dev/null 。
 *
 */
static
void console_init(void)
{
	int fd;
	int tried_devcons = 0;
	int tried_vtmaster = 0;
	char *s;

	if ((s = getenv("CONSOLE")) != NULL)
		console_dev = s;
	else {
		console_dev = CONSOLE;
		tried_devcons++;
	}

	while ((fd = open(console_dev, O_RDONLY|O_NONBLOCK)) < 0) {
		if (!tried_devcons) {
			tried_devcons++;
			console_dev = CONSOLE;
			continue;
		}
		if (!tried_vtmaster) {
			tried_vtmaster++;
			console_dev = VT_MASTER;
			continue;
		}
		break;
	}
	if (fd < 0)
		console_dev = "/dev/null";
	else
		close(fd);
}


/*
 *	Open the console with retries.
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
 * @brief 打开控制台 console，如果不成功，则重新尝试5次。
 */
static
int console_open(int mode)
{
	int f, fd = -1;
	int m;

	/*
	 *	Open device in nonblocking mode.
	 */
	m = mode | O_NONBLOCK;

	/*
	 *	Retry the open five times.
	 */
	for(f = 0; f < 5; f++) {
		if ((fd = open(console_dev, m)) >= 0) break;
		usleep(10000);
	}

	if (fd < 0) return fd;

	/*
	 *	Set original flags.
	 */
	if (m != mode)
  		fcntl(fd, F_SETFL, mode);
	return fd;
}

/*
 *	We got a signal (HUP PWR WINCH ALRM INT)
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
 * @brief 该函数调用了 ADDSET() 宏操作，完成添加一个要处理的信号标志位
 */

static
void signal_handler(int sig)
{
	ADDSET(got_signals, sig);
}

/*
 *	SIGCHLD: one of our children has died.
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
 * @brief 该函数是一个信号处理函数，负责处理 SIGCHLD 信号。
 */

static
# ifdef __GNUC__
void chld_handler(int sig __attribute__((unused)))
# else
void chld_handler(int sig)
# endif
{
	CHILD		*ch;
	int		pid, st;
	int		saved_errno = errno;

	/*
	 *	Find out which process(es) this was (were)
	 */
	while((pid = waitpid(-1, &st, WNOHANG)) != 0) {
		if (errno == ECHILD) break;
		for( ch = family; ch; ch = ch->next )
			if ( ch->pid == pid && (ch->flags & RUNNING) ) {
				INITDBG(L_VB,
					"chld_handler: marked %d as zombie",
					ch->pid);
				ADDSET(got_signals, SIGCHLD);
				ch->exstat = st;
				ch->flags |= ZOMBIE;
				if (ch->new) {
					ch->new->exstat = st;
					ch->new->flags |= ZOMBIE;
				}
				break;
			}
		if (ch == NULL) {
			INITDBG(L_VB, "chld_handler: unknown child %d exited.",
				pid);
		}
	}
	errno = saved_errno;
}

/*
 *	Linux ignores all signals sent to init when the
 *	SIG_DFL handler is installed. Therefore we must catch SIGTSTP
 *	and SIGCONT, or else they won't work....
 *
 *	The SIGCONT handler
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
 * @brief 该函数是一个信号处理函数，负责处理 SIGCOND 信号。
 */

static
# ifdef __GNUC__
void cont_handler(int sig __attribute__((unused)))
# else
void cont_handler(int sig)
# endif
{
	got_cont = 1;
}

/*
 *	Fork and dump core in /.
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
 * @brief 该函数实现 coredump 功能， 通过 Fork 子进程，完成在根目录下 dump core 
 */

static
void coredump(void)
{
	static int		dumped = 0;
	struct rlimit		rlim;
	sigset_t		mask;

	if (dumped) return;
	dumped = 1;

	if (fork() != 0) return;

	sigfillset(&mask);
	sigprocmask(SIG_SETMASK, &mask, NULL);

	rlim.rlim_cur = RLIM_INFINITY;
	rlim.rlim_max = RLIM_INFINITY;
	setrlimit(RLIMIT_CORE, &rlim);
	chdir("/");

	signal(SIGSEGV, SIG_DFL);
	raise(SIGSEGV);
	sigdelset(&mask, SIGSEGV);
	sigprocmask(SIG_SETMASK, &mask, NULL);

	do_sleep(5);
	exit(0);
}

/*
 *	OOPS: segmentation violation!
 *	If we have the info, print where it occured.
 *	Then sleep 30 seconds and try to continue.
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
 * @brief 该函数是一个信号处理函数，负责处理 SIGSEG 信号。
 */

static
#if defined(STACK_DEBUG) && defined(__linux__)
# ifdef __GNUC__
void segv_handler(int sig __attribute__((unused)), struct sigcontext ctx)
# else
void segv_handler(int sig, struct sigcontext ctx)
# endif
{
	char	*p = "";
	int	saved_errno = errno;

	if ((void *)ctx.eip >= (void *)do_sleep &&
	    (void *)ctx.eip < (void *)main)
		p = " (code)";
	initlog(L_VB, "PANIC: segmentation violation at %p%s! "
		  "sleeping for 30 seconds.", (void *)ctx.eip, p);
	coredump();
	do_sleep(30);
	errno = saved_errno;
}
#else
# ifdef __GNUC__
void segv_handler(int sig __attribute__((unused)))
# else
void segv_handler(int sig)
# endif
{
	int	saved_errno = errno;

	initlog(L_VB,
		"PANIC: segmentation violation! sleeping for 30 seconds.");
	coredump();
	do_sleep(30);
	errno = saved_errno;
}
#endif

/*
 *	The SIGSTOP & SIGTSTP handler
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
 * @brief 该函数是一个信号处理函数，负责处理 SIGSTOP & SIGTSTP 信号。
 */

static
# ifdef __GNUC__
void stop_handler(int sig __attribute__((unused)))
# else
void stop_handler(int sig)
# endif
{
	int	saved_errno = errno;

	got_cont = 0;
	while(!got_cont) pause();
	got_cont = 0;
	errno = saved_errno;
}

/*
 *	Set terminal settings to reasonable defaults
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
 * @brief 设置终端工作参数
 *
 * @details 函数执行流程分析：

	1. 调用 console_open 打开 console_dev 设备，模式为读写+非阻塞方式。

	2. 调用 tcgetattr() 函数获得当前终端属性 tty (struct termios 结构体)

	3. 设置 tty.c_cflag 和 tty.c_cc[] 的参数配置。

	4. 设置 tty.c_iflag 和 tty.c_oflag 以及 tty.c_lflag 参数配置。

	5. 调用 tcsetattr() 和 tcflush() 完成设置终端属性的操作。

	6. 调用 close(fd) 关闭终端设备文件。
 *
 */
static
void console_stty(void)
{
	struct termios tty;
	int fd;

	if ((fd = console_open(O_RDWR|O_NOCTTY)) < 0) {
		initlog(L_VB, "can't open %s", console_dev);
		return;
	}

#ifdef __FreeBSD_kernel__
	/*
	 * The kernel of FreeBSD expects userland to set TERM.  Usually, we want
	 * "cons25".  Later, gettys might disagree on this (i.e. we're not using
	 * syscons) but some boot scripts, like /etc/init.d/xserver-xorg, still
	 * need a non-dumb terminal.
	 */
	putenv ("TERM=cons25");
#endif

	(void) tcgetattr(fd, &tty);

	tty.c_cflag &= CBAUD|CBAUDEX|CSIZE|CSTOPB|PARENB|PARODD;
	tty.c_cflag |= HUPCL|CLOCAL|CREAD;

	tty.c_cc[VINTR]	    = CINTR;
	tty.c_cc[VQUIT]	    = CQUIT;
	tty.c_cc[VERASE]    = CERASE; /* ASCII DEL (0177) */
	tty.c_cc[VKILL]	    = CKILL;
	tty.c_cc[VEOF]	    = CEOF;
	tty.c_cc[VTIME]	    = 0;
	tty.c_cc[VMIN]	    = 1;
	tty.c_cc[VSWTC]	    = _POSIX_VDISABLE;
	tty.c_cc[VSTART]    = CSTART;
	tty.c_cc[VSTOP]	    = CSTOP;
	tty.c_cc[VSUSP]	    = CSUSP;
	tty.c_cc[VEOL]	    = _POSIX_VDISABLE;
	tty.c_cc[VREPRINT]  = CREPRINT;
	tty.c_cc[VDISCARD]  = CDISCARD;
	tty.c_cc[VWERASE]   = CWERASE;
	tty.c_cc[VLNEXT]    = CLNEXT;
	tty.c_cc[VEOL2]	    = _POSIX_VDISABLE;

	/*
	 *	Set pre and post processing
	 */
	tty.c_iflag = IGNPAR|ICRNL|IXON|IXANY;
#ifdef IUTF8 /* Not defined on FreeBSD */
	tty.c_iflag |= IUTF8;
#endif /* IUTF8 */
	tty.c_oflag = OPOST|ONLCR;
	tty.c_lflag = ISIG|ICANON|ECHO|ECHOCTL|ECHOPRT|ECHOKE;

#if defined(SANE_TIO) && (SANE_TIO == 1)
	/*
	 *	Disable flow control (-ixon), ignore break (ignbrk),
	 *	and make nl/cr more usable (sane).
	 */
	tty.c_iflag |=  IGNBRK;
	tty.c_iflag &= ~(BRKINT|INLCR|IGNCR|IXON);
	tty.c_oflag &= ~(OCRNL|ONLRET);
#endif
	/*
	 *	Now set the terminal line.
	 *	We don't care about non-transmitted output data
	 *	and non-read input data.
	 */
	(void) tcsetattr(fd, TCSANOW, &tty);
	(void) tcflush(fd, TCIOFLUSH);
	(void) close(fd);
}

/*
 *	Print to the system console
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
 * @brief 打印调试或者运行时提示信息到当前的终端
 */
void print(char *s)
{
	int fd;

	if ((fd = console_open(O_WRONLY|O_NOCTTY|O_NDELAY)) >= 0) {
		write(fd, s, strlen(s));
		close(fd);
	}
}

/*
 *	Log something to a logfile and the console.
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
 * @brief 该函数用于 init 进程执行过程中，输出相关调试信息和写入运行情况到日志文件
 */
#ifdef __GNUC__
__attribute__ ((format (printf, 2, 3)))
#endif
void initlog(int loglevel, char *s, ...)
{
	va_list va_alist;
	char buf[256];
	sigset_t nmask, omask;

	va_start(va_alist, s);
	vsnprintf(buf, sizeof(buf), s, va_alist);
	va_end(va_alist);

	if (loglevel & L_SY) {
		/*
		 *	Re-establish connection with syslogd every time.
		 *	Block signals while talking to syslog.
		 */
		sigfillset(&nmask);
		sigprocmask(SIG_BLOCK, &nmask, &omask);
		openlog("init", 0, LOG_DAEMON);
		syslog(LOG_INFO, "%s", buf);
		closelog();
		sigprocmask(SIG_SETMASK, &omask, NULL);
	}

	/*
	 *	And log to the console.
	 */
	if (loglevel & L_CO) {
		print("\rINIT: ");
		print(buf);
		print("\r\n");
	}
}


/*
 *	Build a new environment for execve().
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
 * @brief 该函数主要用于为 execve() 系统调用，初始化环境变量 RUNLEVEL, PRELEVEL, SHELL, CONSOLE 等
 */
char **init_buildenv(int child)
{
	char		i_lvl[] = "RUNLEVEL=x";
	char		i_prev[] = "PREVLEVEL=x";
	char		i_cons[32];
	char		i_shell[] = "SHELL=" SHELL;
	char		**e;
	int		n, i;

	for (n = 0; environ[n]; n++)
		;
	n += NR_EXTRA_ENV;
	if (child)
		n += 8;
	e = calloc(n, sizeof(char *));

	for (n = 0; environ[n]; n++)
		e[n] = istrdup(environ[n]);

	for (i = 0; i < NR_EXTRA_ENV; i++) {
		if (extra_env[i])
			e[n++] = istrdup(extra_env[i]);
	}

	if (child) {
		snprintf(i_cons, sizeof(i_cons), "CONSOLE=%s", console_dev);
		i_lvl[9]   = thislevel;
		i_prev[10] = prevlevel;
		e[n++] = istrdup(i_shell);
		e[n++] = istrdup(i_lvl);
		e[n++] = istrdup(i_prev);
		e[n++] = istrdup(i_cons);
		e[n++] = istrdup(E_VERSION);
	}

	e[n++] = NULL;

	return e;
}

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
 * @brief 该函数主要释放 init_buildenv() 函数创建的指针数组和内存空间
 */
void init_freeenv(char **e)
{
	int		n;

	for (n = 0; e[n]; n++)
		free(e[n]);
	free(e);
}


/*
 *	Fork and execute.
 *
 *	This function is too long and indents too deep.
 *
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
 * @brief 调用 fork 和 execp 来启动子进程。这个函数非常长，但基本上是属于最底层的函数了。
 *
 * @details 函数执行流程分析：

	1. spawn 整个程序比较长，从927-1192行约有270多行。整个代码逻辑以 fork 调用为分界线，可以分为2个部分。前面部分主要完成启动前的准备工作，后面通过 fork 和 execp 来实际创建出子进程执行 CHILD 节点上规定的程序。

	2. 先分析第一部分。这部分代码主要处理三种情况，1是action 为“RESPAWN”与“ONDEMAND”类型的命令；2是 /etc/initscript 初始化脚本为后继 execp 调用准备参数。

	3. 第二部分进入到一个无限循环中，以便确保能够成功创建出子进程。在调用 fork 创建出 init 的子进程之后，init 的这个子进程将按照 daemon 进程的方式工作，包括需要关闭0，1，2打开文件。也就是说，真正用来创建用户子进程的，不是 pid = 1 的那个原始进程，而是原始进程的子进程再通过一个 fork 和 execp 才能够实现执行真正的用户程序。

	4. 第2次执行 fork 之后，由子进程调用 execp 来完成加载用户程序，而父进程通过调用 waitpid 来等待子进程的结束。

	5. 上述步骤完成之后，父进程又会创建出一个临时的子进程，来完成 setsid() 和 ioctl(f, TIOCSCTTY, 1) 这2个函数调用，来分配一个控制终端，创建一个新的会话，失去原有的控制终端的所有联系。
 *
 */
static
pid_t spawn(CHILD *ch, int *res)
{
  char *args[16];		/* Argv array */
  char buf[136];		/* Line buffer */
  int f, st;			/* Scratch variables */
  char *ptr;			/* Ditto */
  time_t t;			/* System time */
  int oldAlarm;			/* Previous alarm value */
  char *proc = ch->process;	/* Command line */
  pid_t pid, pgrp;		/* child, console process group. */
  sigset_t nmask, omask;	/* For blocking SIGCHLD */
  struct sigaction sa;

  INITDBG(L_VB, "spawn: %s (pid=%d)\n", ch->process, ch->pid);
  return 0;

  *res = -1;
  buf[sizeof(buf) - 1] = 0;

  /* Skip '+' if it's there */
  if (proc[0] == '+') proc++;

  ch->flags |= XECUTED;

  if (ch->action == RESPAWN || ch->action == ONDEMAND) {
	/* Is the date stamp from less than 2 minutes ago? */
	time(&t);
	if (ch->tm + TESTTIME > t) {
		ch->count++;
	} else {
		ch->count = 0;
		ch->tm = t;
	}

	/* Do we try to respawn too fast? */
	if (ch->count >= MAXSPAWN) {

	  initlog(L_VB,
		"Id \"%s\" respawning too fast: disabled for %d minutes",
		ch->id, SLEEPTIME / 60);
	  ch->flags &= ~RUNNING;
	  ch->flags |= FAILING;

	  /* Remember the time we stopped */
	  ch->tm = t;

	  /* Try again in 5 minutes */
	  oldAlarm = alarm(0);
	  if (oldAlarm > SLEEPTIME || oldAlarm <= 0) oldAlarm = SLEEPTIME;
	  alarm(oldAlarm);
	  return(-1);
	}
  }

  /* See if there is an "initscript" (except in single user mode). */
  if (access(INITSCRIPT, R_OK) == 0 && runlevel != 'S') {
	/* Build command line using "initscript" */
	args[1] = SHELL;
	args[2] = INITSCRIPT;
	args[3] = ch->id;
	args[4] = ch->rlevel;
	args[5] = "unknown";
	for(f = 0; actions[f].name; f++) {
		if (ch->action == actions[f].act) {
			args[5] = actions[f].name;
			break;
		}
	}
	args[6] = proc;
	args[7] = NULL;
  } else if (strpbrk(proc, "~`!$^&*()=|\\{}[];\"'<>?")) {
  /* See if we need to fire off a shell for this command */
  	/* Give command line to shell */
  	args[1] = SHELL;
  	args[2] = "-c";
  	strcpy(buf, "exec ");
  	strncat(buf, proc, sizeof(buf) - strlen(buf) - 1);
  	args[3] = buf;
  	args[4] = NULL;
  } else {
	/* Split up command line arguments */
	buf[0] = 0;
  	strncat(buf, proc, sizeof(buf) - 1);
  	ptr = buf;
  	for(f = 1; f < 15; f++) {
  		/* Skip white space */
  		while(*ptr == ' ' || *ptr == '\t') ptr++;
  		args[f] = ptr;
  		
		/* May be trailing space.. */
		if (*ptr == 0) break;

  		/* Skip this `word' */
  		while(*ptr && *ptr != ' ' && *ptr != '\t' && *ptr != '#')
  			ptr++;
  		
  		/* If end-of-line, break */	
  		if (*ptr == '#' || *ptr == 0) {
  			f++;
  			*ptr = 0;
  			break;
  		}
  		/* End word with \0 and continue */
  		*ptr++ = 0;
  	}
  	args[f] = NULL;
  }
  args[0] = args[1];
  while(1) {
	/*
	 *	Block sigchild while forking.
	 */
	sigemptyset(&nmask);
	sigaddset(&nmask, SIGCHLD);
	sigprocmask(SIG_BLOCK, &nmask, &omask);

	if ((pid = fork()) == 0) {

#if 0
		close(0);
		close(1);
		close(2);
#endif
		if (pipe_fd >= 0) close(pipe_fd);

  		sigprocmask(SIG_SETMASK, &omask, NULL);

		/*
		 *	In sysinit, boot, bootwait or single user mode:
		 *	for any wait-type subprocess we _force_ the console
		 *	to be its controlling tty.
		 */
  		if (strchr("*#sS", runlevel) && ch->flags & WAITING) {
			/*
			 *	We fork once extra. This is so that we can
			 *	wait and change the process group and session
			 *	of the console after exit of the leader.
			 */
			setsid();
			if ((f = console_open(O_RDWR|O_NOCTTY)) >= 0) {
				/* Take over controlling tty by force */
				(void)ioctl(f, TIOCSCTTY, 1);
  				dup(f);
  				dup(f);
			}

			/*
			 * 4 Sep 2001, Andrea Arcangeli:
			 * Fix a race in spawn() that is used to deadlock init in a
			 * waitpid() loop: must set the childhandler as default before forking
			 * off the child or the chld_handler could run before the waitpid loop
			 * has a chance to find its zombie-child.
			 */
			SETSIG(sa, SIGCHLD, SIG_DFL, SA_RESTART);
			if ((pid = fork()) < 0) {
  				initlog(L_VB, "cannot fork: %s",
					strerror(errno));
				exit(1);
			}
			if (pid > 0) {
				pid_t rc;
				/*
				 *	Ignore keyboard signals etc.
				 *	Then wait for child to exit.
				 */
				SETSIG(sa, SIGINT, SIG_IGN, SA_RESTART);
				SETSIG(sa, SIGTSTP, SIG_IGN, SA_RESTART);
				SETSIG(sa, SIGQUIT, SIG_IGN, SA_RESTART);

				while ((rc = waitpid(pid, &st, 0)) != pid)
					if (rc < 0 && errno == ECHILD)
						break;

				/*
				 *	Small optimization. See if stealing
				 *	controlling tty back is needed.
				 */
				pgrp = tcgetpgrp(f);
				if (pgrp != getpid())
					exit(0);

				/*
				 *	Steal controlling tty away. We do
				 *	this with a temporary process.
				 */
				if ((pid = fork()) < 0) {
  					initlog(L_VB, "cannot fork: %s",
						strerror(errno));
					exit(1);
				}
				if (pid == 0) {
					setsid();
					(void)ioctl(f, TIOCSCTTY, 1);
					exit(0);
				}
				while((rc = waitpid(pid, &st, 0)) != pid)
					if (rc < 0 && errno == ECHILD)
						break;
				exit(0);
			}

			/* Set ioctl settings to default ones */
			console_stty();

  		} else {
			setsid();
			if ((f = console_open(O_RDWR|O_NOCTTY)) < 0) {
				initlog(L_VB, "open(%s): %s", console_dev,
					strerror(errno));
				f = open("/dev/null", O_RDWR);
			}
			dup(f);
			dup(f);
		}

		/*
		 * Update utmp/wtmp file prior to starting
		 * any child.  This MUST be done right here in
		 * the child process in order to prevent a race
		 * condition that occurs when the child
		 * process' time slice executes before the
		 * parent (can and does happen in a uniprocessor
		 * environment).  If the child is a getty and
		 * the race condition happens, then init's utmp
		 * update will happen AFTER the getty runs
		 * and expects utmp to be updated already!
		 *
		 * Do NOT log if process field starts with '+'
		 * FIXME: that's for compatibility with *very*
		 * old getties - probably it can be taken out.
		 */
		if (ch->process[0] != '+')
			write_utmp_wtmp("", ch->id, getpid(), INIT_PROCESS, "");

  		/* Reset all the signals, set up environment */
  		for(f = 1; f < NSIG; f++) SETSIG(sa, f, SIG_DFL, SA_RESTART);
		environ = init_buildenv(1);

		/*
		 *	Execute prog. In case of ENOEXEC try again
		 *	as a shell script.
		 */
  		execvp(args[1], args + 1);
		if (errno == ENOEXEC) {
  			args[1] = SHELL;
  			args[2] = "-c";
  			strcpy(buf, "exec ");
  			strncat(buf, proc, sizeof(buf) - strlen(buf) - 1);
  			args[3] = buf;
  			args[4] = NULL;
			execvp(args[1], args + 1);
		}
  		initlog(L_VB, "cannot execute \"%s\"", args[1]);

		if (ch->process[0] != '+')
			write_utmp_wtmp("", ch->id, getpid(), DEAD_PROCESS, NULL);
  		exit(1);
  	}
	*res = pid;
  	sigprocmask(SIG_SETMASK, &omask, NULL);

	INITDBG(L_VB, "Started id %s (pid %d)", ch->id, pid);

	if (pid == -1) {
		initlog(L_VB, "cannot fork, retry..");
		do_sleep(5);
		continue;
	}
	return(pid);
  }
}

/*
 *	Start a child running!
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
 * @brief 执行 CHILD 节点所代表的配置行上的命令行，通常是个脚本程序。
 *
 * @details 函数执行流程分析：

	1. 对于 CHILD *ch 节点中的 action 字段来进行判别。如果是 SYSINIT，BOOTWAIT，WAIT，POWERWAIT，POWERFAILNOW，POWEROKWAIT，CTRLALTDEL 这些情况，则设置标志为 WAITING，然后执行 spawn 函数。这个函数是完成启动子进程的真正的函数，spawn 名字的含义是产卵的意思，顾名思义就是产生后继的子进程。后面我们再对这个函数做详细分析。

	2. 如果是 KBREQUEST，BOOT，POWERFAIL，ONCE 则直接退出，不进行后继的 spawn 函数调用。

	3. 如果是 ONDEMAND，RESPAWN，则将 flags 设置为 RUNNING 后，立即执行 spawn 操作。
 *
 */
static
void startup(CHILD *ch)
{
	/*
	 *	See if it's disabled
	 */
	if (ch->flags & FAILING) return;

	switch(ch->action) {

		case SYSINIT:
		case BOOTWAIT:
		case WAIT:
		case POWERWAIT:
		case POWERFAILNOW:
		case POWEROKWAIT:
		case CTRLALTDEL:
			if (!(ch->flags & XECUTED)) ch->flags |= WAITING;
		case KBREQUEST:
		case BOOT:
		case POWERFAIL:
		case ONCE:
			if (ch->flags & XECUTED) break;
		case ONDEMAND:
		case RESPAWN:
  			ch->flags |= RUNNING;
  			(void)spawn(ch, &(ch->pid));
  			break;
	}
}


/*
 *	Read the inittab file.
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
 * @brief 读取 /etc/inittab 文件，解析其中的约定规则，形成一个 CHILD 链表数据结构中。
 *
 * @details 函数执行流程分析：

	该函数中用到的重要数据结构有 CHILD (struct _child_) 和 actions 数组 (struct actions)

	1. 读取 /etc/inittab 文件，按行读取，到 buf 数组中。

	2. 遇到开头是空格或者TAB制表符的行，忽略直到第一个字母，如果发现是第一个字母是#开头的注释，或者\n开头的空行，都直接跳过。

	3. 使用 strsep 函数，以 : 冒号作为间隔符号，依次找到 id, rlevel, action, process 这4个字段，分别代表的含义可参考下面的详细说明。同时将 action 字段中的字符串关键字转换为整型数 actionNo，方便后面的判别。

	4. 检查当前的 id 字段，是否是唯一的，如果之前已经出现过，则忽略掉。

	5. 通过 imalloc 函数，动态分配 CHILD 结构体节点ch，结构体的定义见上面。然后将刚才分析的结果填入结构体中，并将这个节点，添加到链表 newFamily 中。其中包括 actionNo 填入 ch->action, id 填入 ch->id, process 填入 ch->process 等。

	6. 关闭 /etc/inittab 文件。

	7. 接下来，查看老的启动进程列表 family，看是否有进程需要被杀死的。这里有两轮检查，第一轮会给所有没有在新的运行级别中定义的进程发送一个警告信号 SIGTERM。如果在第一轮中有这样的进程，则会等待5秒，然后进入下一轮检查。在第2轮检查中，它会发送 SIGKILL 信号来强制中止所有子进程的运行。

	8. 等所有子进程被杀死后，init 通过调用 write_utmp_wtmp() 来将终止信息和原因记录进这两个文件中。记录的信息包括子进程在 inittab 文件中的 id，子进程本身的 pid 等。

	9. 这2个步骤7，8完成之后，init 开始清除老的 family 链表上的所有节点，释放空间。

	10. 最后 init 把刚才新建成的 newFamily 链表赋值给 -> family 链表，完成重建链表的操作即结束。
 *
 */

static
void read_inittab(void)
{
  FILE		*fp;			/* The INITTAB file */
  CHILD		*ch, *old, *i;		/* Pointers to CHILD structure */
  CHILD		*head = NULL;		/* Head of linked list */
#ifdef INITLVL
  struct stat	st;			/* To stat INITLVL */
#endif
  sigset_t	nmask, omask;		/* For blocking SIGCHLD. */
  char		buf[256];		/* Line buffer */
  char		err[64];		/* Error message. */
  char		*id, *rlevel,
		*action, *process;	/* Fields of a line */
  char		*p;
  int		lineNo = 0;		/* Line number in INITTAB file */
  int		actionNo;		/* Decoded action field */
  int		f;			/* Counter */
  int		round;			/* round 0 for SIGTERM, 1 for SIGKILL */
  int		foundOne = 0;		/* No killing no sleep */
  int		talk;			/* Talk to the user */
  int		done = 0;		/* Ready yet? */

#if DEBUG
  if (newFamily != NULL) {
	INITDBG(L_VB, "PANIC newFamily != NULL");
	exit(1);
  }
  INITDBG(L_VB, "Reading inittab");
#endif

  /*
   *	Open INITTAB and real line by line.
   */
  if ((fp = fopen(INITTAB, "r")) == NULL)
	initlog(L_VB, "No inittab file found");

  while(!done) {
	/*
	 *	Add single user shell entry at the end.
	 */
	if (fp == NULL || fgets(buf, sizeof(buf), fp) == NULL) {
		done = 1;
		/*
		 *	See if we have a single user entry.
		 */
		for(old = newFamily; old; old = old->next)
			if (strpbrk(old->rlevel, "S")) break;
		if (old == NULL)
			snprintf(buf, sizeof(buf), "~~:S:wait:%s\n", SULOGIN);
		else
			continue;
	}
	lineNo++;
	/*
	 *	Skip comments and empty lines
	 */
	for(p = buf; *p == ' ' || *p == '\t'; p++)
		;
	if (*p == '#' || *p == '\n') continue;

	/*
	 *	Decode the fields
	 */
	id =      strsep(&p, ":");
	rlevel =  strsep(&p, ":");
	action =  strsep(&p, ":");
	process = strsep(&p, "\n");

	/*
	 *	Check if syntax is OK. Be very verbose here, to
	 *	avoid newbie postings on comp.os.linux.setup :)
	 */
	err[0] = 0;
	if (!id || !*id) strcpy(err, "missing id field");
	if (!rlevel)     strcpy(err, "missing runlevel field");
	if (!process)    strcpy(err, "missing process field");
	if (!action || !*action)
			strcpy(err, "missing action field");
	if (id && strlen(id) > sizeof(utproto.ut_id))
		sprintf(err, "id field too long (max %d characters)",
			(int)sizeof(utproto.ut_id));
	if (rlevel && strlen(rlevel) > 11)
		strcpy(err, "rlevel field too long (max 11 characters)");
	if (process && strlen(process) > 127)
		strcpy(err, "process field too long");
	if (action && strlen(action) > 32)
		strcpy(err, "action field too long");
	if (err[0] != 0) {
		initlog(L_VB, "%s[%d]: %s", INITTAB, lineNo, err);
		INITDBG(L_VB, "%s:%s:%s:%s", id, rlevel, action, process);
		continue;
	}
  
	/*
	 *	Decode the "action" field
	 */
	actionNo = -1;
	for(f = 0; actions[f].name; f++)
		if (strcasecmp(action, actions[f].name) == 0) {
			actionNo = actions[f].act;
			break;
		}
	if (actionNo == -1) {
		initlog(L_VB, "%s[%d]: %s: unknown action field",
			INITTAB, lineNo, action);
		continue;
	}

	/*
	 *	See if the id field is unique
	 */
	for(old = newFamily; old; old = old->next) {
		if(strcmp(old->id, id) == 0 && strcmp(id, "~~")) {
			initlog(L_VB, "%s[%d]: duplicate ID field \"%s\"",
				INITTAB, lineNo, id);
			break;
		}
	}
	if (old) continue;

	/*
	 *	Allocate a CHILD structure
	 */
	ch = imalloc(sizeof(CHILD));

	/*
	 *	And fill it in.
	 */
	ch->action = actionNo;
	strncpy(ch->id, id, sizeof(utproto.ut_id) + 1); /* Hack for different libs. */
	strncpy(ch->process, process, sizeof(ch->process) - 1);
	if (rlevel[0]) {
		for(f = 0; f < (int)sizeof(rlevel) - 1 && rlevel[f]; f++) {
			ch->rlevel[f] = rlevel[f];
			if (ch->rlevel[f] == 's') ch->rlevel[f] = 'S';
		}
		strncpy(ch->rlevel, rlevel, sizeof(ch->rlevel) - 1);
	} else {
		strcpy(ch->rlevel, "0123456789");
		if (ISPOWER(ch->action))
			strcpy(ch->rlevel, "S0123456789");
	}
	/*
	 *	We have the fake runlevel '#' for SYSINIT  and
	 *	'*' for BOOT and BOOTWAIT.
	 */
	if (ch->action == SYSINIT) strcpy(ch->rlevel, "#");
	if (ch->action == BOOT || ch->action == BOOTWAIT)
		strcpy(ch->rlevel, "*");

	/*
	 *	Now add it to the linked list. Special for powerfail.
	 */
	if (ISPOWER(ch->action)) {

		/*
		 *	Disable by default
		 */
		ch->flags |= XECUTED;

		/*
		 *	Tricky: insert at the front of the list..
		 */
		old = NULL;
		for(i = newFamily; i; i = i->next) {
			if (!ISPOWER(i->action)) break;
			old = i;
		}
		/*
		 *	Now add after entry "old"
		 */
		if (old) {
			ch->next = i;
			old->next = ch;
			if (i == NULL) head = ch;
		} else {
			ch->next = newFamily;
			newFamily = ch;
			if (ch->next == NULL) head = ch;
		}
	} else {
		/*
		 *	Just add at end of the list
		 */
		if (ch->action == KBREQUEST) ch->flags |= XECUTED;
		ch->next = NULL;
		if (head)
			head->next = ch;
		else
			newFamily = ch;
		head = ch;
	}

	/*
	 *	Walk through the old list comparing id fields
	 */
	for(old = family; old; old = old->next)
		if (strcmp(old->id, ch->id) == 0) {
			old->new = ch;
			break;
		}
  }
  /*
   *	We're done.
   */
  if (fp) fclose(fp);

  /*
   *	Loop through the list of children, and see if they need to
   *	be killed. 
   */

  INITDBG(L_VB, "Checking for children to kill");
  for(round = 0; round < 2; round++) {
    talk = 1;
    for(ch = family; ch; ch = ch->next) {
	ch->flags &= ~KILLME;

	/*
	 *	Is this line deleted?
	 */
	if (ch->new == NULL) ch->flags |= KILLME;

	/*
	 *	If the entry has changed, kill it anyway. Note that
	 *	we do not check ch->process, only the "action" field.
	 *	This way, you can turn an entry "off" immediately, but
	 *	changes in the command line will only become effective
	 *	after the running version has exited.
	 */
	if (ch->new && ch->action != ch->new->action) ch->flags |= KILLME;

	/*
	 *	Only BOOT processes may live in all levels
	 */
	if (ch->action != BOOT &&
	    strchr(ch->rlevel, runlevel) == NULL) {
		/*
		 *	Ondemand procedures live always,
		 *	except in single user
		 */
		if (runlevel == 'S' || !(ch->flags & DEMAND))
			ch->flags |= KILLME;
	}

	/*
	 *	Now, if this process may live note so in the new list
	 */
	if ((ch->flags & KILLME) == 0) {
		ch->new->flags  = ch->flags;
		ch->new->pid    = ch->pid;
		ch->new->exstat = ch->exstat;
		continue;
	}


	/*
	 *	Is this process still around?
	 */
	if ((ch->flags & RUNNING) == 0) {
		ch->flags &= ~KILLME;
		continue;
	}
	INITDBG(L_VB, "Killing \"%s\"", ch->process);
	switch(round) {
		case 0: /* Send TERM signal */
			if (talk)
				initlog(L_CO,
					"Sending processes the TERM signal");
			kill(-(ch->pid), SIGTERM);
			foundOne = 1;
			break;
		case 1: /* Send KILL signal and collect status */
			if (talk)
				initlog(L_CO,
					"Sending processes the KILL signal");
			kill(-(ch->pid), SIGKILL);
			break;
	}
	talk = 0;
	
    }
    /*
     *	See if we have to wait 5 seconds
     */
    if (foundOne && round == 0) {
	/*
	 *	Yup, but check every second if we still have children.
	 */
	for(f = 0; f < sltime; f++) {
		for(ch = family; ch; ch = ch->next) {
			if (!(ch->flags & KILLME)) continue;
			if ((ch->flags & RUNNING) && !(ch->flags & ZOMBIE))
				break;
		}
		if (ch == NULL) {
			/*
			 *	No running children, skip SIGKILL
			 */
			round = 1;
			foundOne = 0; /* Skip the sleep below. */
			break;
		}
		do_sleep(1);
	}
    }
  }

  /*
   *	Now give all processes the chance to die and collect exit statuses.
   */
  if (foundOne) do_sleep(1);
  for(ch = family; ch; ch = ch->next)
	if (ch->flags & KILLME) {
		if (!(ch->flags & ZOMBIE))
		    initlog(L_CO, "Pid %d [id %s] seems to hang", ch->pid,
				ch->id);
		else {
		    INITDBG(L_VB, "Updating utmp for pid %d [id %s]",
				ch->pid, ch->id);
		    ch->flags &= ~RUNNING;
		    if (ch->process[0] != '+')
		    	write_utmp_wtmp("", ch->id, ch->pid, DEAD_PROCESS, NULL);
		}
	}

  /*
   *	Both rounds done; clean up the list.
   */
  sigemptyset(&nmask);
  sigaddset(&nmask, SIGCHLD);
  sigprocmask(SIG_BLOCK, &nmask, &omask);
  for(ch = family; ch; ch = old) {
	old = ch->next;
	free(ch);
  }
  family = newFamily;
  for(ch = family; ch; ch = ch->next) ch->new = NULL;
  newFamily = NULL;
  sigprocmask(SIG_SETMASK, &omask, NULL);

#ifdef INITLVL
  /*
   *	Dispose of INITLVL file.
   */
  if (lstat(INITLVL, &st) >= 0 && S_ISLNK(st.st_mode)) {
	/*
	 *	INITLVL is a symbolic link, so just truncate the file.
	 */
	close(open(INITLVL, O_WRONLY|O_TRUNC));
  } else {
	/*
	 *	Delete INITLVL file.
	 */
  	unlink(INITLVL);
  }
#endif
#ifdef INITLVL2
  /*
   *	Dispose of INITLVL2 file.
   */
  if (lstat(INITLVL2, &st) >= 0 && S_ISLNK(st.st_mode)) {
	/*
	 *	INITLVL2 is a symbolic link, so just truncate the file.
	 */
	close(open(INITLVL2, O_WRONLY|O_TRUNC));
  } else {
	/*
	 *	Delete INITLVL2 file.
	 */
  	unlink(INITLVL2);
  }
#endif
}

/*
 *	Walk through the family list and start up children.
 *	The entries that do not belong here at all are removed
 *	from the list.
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
 * @brief 遍历 family 链表，调用 startup 启动链表上的子进程。
 *
 * @details 函数执行流程分析：

	1. 从 family 链表的表头开始遍历该链表，根据每一个节点 ch 的 flags 标志来进行判别。

	2. 如果当前节点 flags 表示 WAITING, 则说明正在等待，之前的工作未完成，立即退出该函数。

	3. 如果当前节点 flags 表示 RUNNING, 则对这个正在运行的进程不做任何操作，继续下一个。

	4. 如果当前节点的运行级别正好是当前 init 运行级别，则调用 startup 函数启动这个进程。

	5. 如果当前节点不属于在当前运行级别中运行的程序，则将节点 flags 设置为 ~(RUNNING | WAITING) 表示不是运行中，也不是等待中。
 *
 */

static
void start_if_needed(void)
{
	CHILD *ch;		/* Pointer to child */
	int delete;		/* Delete this entry from list? */

	INITDBG(L_VB, "Checking for children to start");

	for(ch = family; ch; ch = ch->next) {

#if DEBUG
		if (ch->rlevel[0] == 'C') {
			INITDBG(L_VB, "%s: flags %d", ch->process, ch->flags);
		}
#endif

		/* Are we waiting for this process? Then quit here. */
		if (ch->flags & WAITING) break;

		/* Already running? OK, don't touch it */
		if (ch->flags & RUNNING) continue;

		/* See if we have to start it up */
		delete = 1;
		if (strchr(ch->rlevel, runlevel) ||
		    ((ch->flags & DEMAND) && !strchr("#*Ss", runlevel))) {
			startup(ch);
			delete = 0;
		}

		if (delete) {
			/* FIXME: is this OK? */
			ch->flags &= ~(RUNNING|WAITING);
			if (!ISPOWER(ch->action) && ch->action != KBREQUEST)
				ch->flags &= ~XECUTED;
			ch->pid = 0;
		} else
			/* Do we have to wait for this process? */
			if (ch->flags & WAITING) break;
	}
	/* Done. */
}

/*
 *	Ask the user on the console for a runlevel
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
 * @brief 该全局变量是保存当次 init 进程启动时的 runlevel 
 */

static
int ask_runlevel(void)
{
	const char	prompt[] = "\nEnter runlevel: ";
	char		buf[8];
	int		lvl = -1;
	int		fd;

	console_stty();
	fd = console_open(O_RDWR|O_NOCTTY);

	if (fd < 0) return('S');

	while(!strchr("0123456789S", lvl)) {
  		write(fd, prompt, sizeof(prompt) - 1);
		buf[0] = 0;
  		read(fd, buf, sizeof(buf));
  		if (buf[0] != 0 && (buf[1] == '\r' || buf[1] == '\n'))
			lvl = buf[0];
		if (islower(lvl)) lvl = toupper(lvl);
	}
	close(fd);
	return lvl;
}

/*
 *	Search the INITTAB file for the 'initdefault' field, with the default
 *	runlevel. If this fails, ask the user to supply a runlevel.
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
 * @brief 查找 /etc/inittab 文件中的 initdefault 默认运行级别，如果有则返回，如果没有则请用户输入。
 *
 * @details 函数执行流程分析：

	1. 实际上这个函数是从 family 链表中遍历，取出每一个节点 ch

	2. 如果 ch->action == INITDEFAULT ，则将当前 ch 的运行级别赋值给 lvl

	3. 判断如果 lvl 是小写，则转换为大写。并且对 lvl 进行判别，看它是否属于 “0123456789S“ 的其中之一。

	4. 如果从文件中得到的 lvl 正确，则返回 lvl; 

	5. 如果从文件中无法得到正确的 lvl，则调用 ask_runlevel() 函数返回。这个函数中会通过终端来询问用户，并要求用户输入一个默认运行级别。
 *
 */

static
int get_init_default(void)
{
	CHILD *ch;
	int lvl = -1;
	char *p;

	/*
	 *	Look for initdefault.
	 */
	for(ch = family; ch; ch = ch->next)
		if (ch->action == INITDEFAULT) {
			p = ch->rlevel;
			while(*p) {
				if (*p > lvl) lvl = *p;
				p++;
			}
			break;
		}
	/*
	 *	See if level is valid
	 */
	if (lvl > 0) {
		if (islower(lvl)) lvl = toupper(lvl);
		if (strchr("0123456789S", lvl) == NULL) {
			initlog(L_VB,
				"Initdefault level '%c' is invalid", lvl);
			lvl = 0;
		}
	}
	/*
	 *	Ask for runlevel on console if needed.
	 */
	if (lvl <= 0) lvl = ask_runlevel();

	/*
	 *	Log the fact that we have a runlevel now.
	 */
	return lvl;
}


/*
 *	We got signaled.
 *
 *	Do actions for the new level. If we are compatible with
 *	the "old" INITLVL and arg == 0, try to read the new
 *	runlevel from that file first.
 */
static
int read_level(int arg)
{
	CHILD		*ch;			/* Walk through list */
	unsigned char	foo = 'X';		/* Contents of INITLVL */
	int		ok = 1;
#ifdef INITLVL
	FILE		*fp;
	struct stat	stt;
	int		st;
#endif

	if (arg) foo = arg;

#ifdef INITLVL
	ok = 0;

	if (arg == 0) {
		fp = NULL;
		if (stat(INITLVL, &stt) != 0 || stt.st_size != 0L)
			fp = fopen(INITLVL, "r");
#ifdef INITLVL2
		if (fp == NULL &&
		    (stat(INITLVL2, &stt) != 0 || stt.st_size != 0L))
			fp = fopen(INITLVL2, "r");
#endif
		if (fp == NULL) {
			/* INITLVL file empty or not there - act as 'init q' */
			initlog(L_SY, "Re-reading inittab");
  			return(runlevel);
		}
		ok = fscanf(fp, "%c %d", &foo, &st);
		fclose(fp);
	} else {
		/* We go to the new runlevel passed as an argument. */
		foo = arg;
		ok = 1;
	}
	if (ok == 2) sltime = st;

#endif /* INITLVL */

	if (islower(foo)) foo = toupper(foo);
	if (ok < 1 || ok > 2 || strchr("QS0123456789ABCU", foo) == NULL) {
 		initlog(L_VB, "bad runlevel: %c", foo);
  		return runlevel;
	}

	/* Log this action */
	switch(foo) {
		case 'S':
  			initlog(L_VB, "Going single user");
			break;
		case 'Q':
			initlog(L_SY, "Re-reading inittab");
			break;
		case 'A':
		case 'B':
		case 'C':
			initlog(L_SY,
				"Activating demand-procedures for '%c'", foo);
			break;
		case 'U':
			initlog(L_SY, "Trying to re-exec init");
			return 'U';
		default:
		  	initlog(L_VB, "Switching to runlevel: %c", foo);
	}

	if (foo == 'Q') {
#if defined(SIGINT_ONLYONCE) && (SIGINT_ONLYONCE == 1)
		/* Re-enable signal from keyboard */
		struct sigaction sa;
		SETSIG(sa, SIGINT, signal_handler, 0);
#endif
		return runlevel;
	}

	/* Check if this is a runlevel a, b or c */
	if (strchr("ABC", foo)) {
		if (runlevel == 'S') return(runlevel);

		/* Read inittab again first! */
		read_inittab();

  		/* Mark those special tasks */
		for(ch = family; ch; ch = ch->next)
			if (strchr(ch->rlevel, foo) != NULL ||
			    strchr(ch->rlevel, tolower(foo)) != NULL) {
				ch->flags |= DEMAND;
				ch->flags &= ~XECUTED;
				INITDBG(L_VB,
					"Marking (%s) as ondemand, flags %d",
					ch->id, ch->flags);
			}
  		return runlevel;
	}

	/* Store both the old and the new runlevel. */
	wrote_utmp_rlevel = 0;
	wrote_wtmp_rlevel = 0;
	write_utmp_wtmp("runlevel", "~~", foo + 256*runlevel, RUN_LVL, "~");
	thislevel = foo;
	prevlevel = runlevel;
	return foo;
}


/*
 *	This procedure is called after every signal (SIGHUP, SIGALRM..)
 *
 *	Only clear the 'failing' flag if the process is sleeping
 *	longer than 5 minutes, or inittab was read again due
 *	to user interaction.
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
 * @brief 在每次信号处理完成之后，遍历 family 链表检查每个节点的状态
 *
 * @details 函数执行流程分析：

	1. 首先调用 time(&t) 获得系统时间。

	2. 从 family 链表头开始，遍历整个链表，直到结束。 

	3. 检查每一个节点 ch 的 flags 是否表示 FAILING

	4. 如果是，并且这个进程已经睡眠 sleep 了至少5分钟，则会清除掉  flags 中的 FAILING 标识位。

	5. 如果不是，则设置下一次 alarm 的时间为这个进程 sleep 的时间加上 5 分钟。
 *
 */
static
void fail_check(void)
{
	CHILD	*ch;			/* Pointer to child structure */
	time_t	t;			/* System time */
	time_t	next_alarm = 0;		/* When to set next alarm */

	time(&t);

	for(ch = family; ch; ch = ch->next) {

		if (ch->flags & FAILING) {
			/* Can we free this sucker? */
			if (ch->tm + SLEEPTIME < t) {
				ch->flags &= ~FAILING;
				ch->count = 0;
				ch->tm = 0;
			} else {
				/* No, we'll look again later */
				if (next_alarm == 0 ||
				    ch->tm + SLEEPTIME > next_alarm)
					next_alarm = ch->tm + SLEEPTIME;
			}
		}
	}
	if (next_alarm) {
		next_alarm -= t;
		if (next_alarm < 1) next_alarm = 1;
		alarm(next_alarm);
	}
}

/* Set all 'Fail' timers to 0 */
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
 * @brief 通知每一个正在运行的进程，设置 'Fail' 定时器为 0
 */
static
void fail_cancel(void)
{
	CHILD *ch;

	for(ch = family; ch; ch = ch->next) {
		ch->count = 0;
		ch->tm = 0;
		ch->flags &= ~FAILING;
	}
}

/*
 *	Start up powerfail entries.
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
 * @brief 通知每一个正在运行的进程，设置 powerwait 和 powerfail 标志位
 */
static
void do_power_fail(int pwrstat)
{
	CHILD *ch;

	/*
	 *	Tell powerwait & powerfail entries to start up
	 */
	for (ch = family; ch; ch = ch->next) {
		if (pwrstat == 'O') {
			/*
		 	 *	The power is OK again.
		 	 */
			if (ch->action == POWEROKWAIT)
				ch->flags &= ~XECUTED;
		} else if (pwrstat == 'L') {
			/*
			 *	Low battery, shut down now.
			 */
			if (ch->action == POWERFAILNOW)
				ch->flags &= ~XECUTED;
		} else {
			/*
			 *	Power is failing, shutdown imminent
			 */
			if (ch->action == POWERFAIL || ch->action == POWERWAIT)
				ch->flags &= ~XECUTED;
		}
	}
}

/*
 *	Check for state-pipe presence
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
 * @brief 检查 state pipe 是否存在，如果存在则从 pipe 中读取 sinature 签名，并验证签名。
 */
static
int check_pipe(int fd)
{
	struct timeval	t;
	fd_set		s;
	char		signature[8];

	FD_ZERO(&s);
	FD_SET(fd, &s);
	t.tv_sec = t.tv_usec = 0;

	if (select(fd+1, &s, NULL, NULL, &t) != 1)
		return 0;
	if (read(fd, signature, 8) != 8)
		 return 0;
	return strncmp(Signature, signature, 8) == 0;
}

/*
 *	 Make a state-pipe.
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
 * @brief 创建一个用于通信的 STATE_PIPE，用传入的 fd 参数表示管道的读端，返回值是管道的写端。
 */
static
int make_pipe(int fd)
{
	int fds[2];

	pipe(fds);
	dup2(fds[0], fd);
	close(fds[0]);
	fcntl(fds[1], F_SETFD, 1);
	fcntl(fd, F_SETFD, 0);
	write(fds[1], Signature, 8);

	return fds[1];
}

/*
 *	Attempt to re-exec.
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
 * @brief 强制 init 程序重新执行。
 *
 * @details 函数执行流程分析：
	
	1. 该函数会创建 STATE_PIPE，并向 STATE_PIPE 写入 Signature = "12567362"

	2. 接着fork()出一个子进程，通过子进程调用 send_state() 向 STATE_PIPE 写入父进程（当前init进程）的状态信息；

	3. 然后父进程调用 execle() 重新执行 init 程序，并且传递参数“--init”, 也就是强制init重新执行。而这个重新执行的 init 进程，无需做初始化读取 /etc/inittab 就能调用 init_main()。
 *
 */
static
void re_exec(void)
{
	CHILD		*ch;
	sigset_t	mask, oldset;
	pid_t		pid;
	char		**env;
	int		fd;

	if (strchr("S0123456",runlevel) == NULL)
		return;

	/*
	 *	Reset the alarm, and block all signals.
	 */
	alarm(0);
	sigfillset(&mask);
	sigprocmask(SIG_BLOCK, &mask, &oldset);

	/*
	 *	construct a pipe fd --> STATE_PIPE and write a signature
	 */
	fd = make_pipe(STATE_PIPE);

	/* 
	 * It's a backup day today, so I'm pissed off.  Being a BOFH, however, 
	 * does have it's advantages...
	 */
	fail_cancel();
	close(pipe_fd);
	pipe_fd = -1;
	DELSET(got_signals, SIGCHLD);
	DELSET(got_signals, SIGHUP);
	DELSET(got_signals, SIGUSR1);

	/*
	 *	That should be cleaned.
	 */
	for(ch = family; ch; ch = ch->next)
	    if (ch->flags & ZOMBIE) {
		INITDBG(L_VB, "Child died, PID= %d", ch->pid);
		ch->flags &= ~(RUNNING|ZOMBIE|WAITING);
		if (ch->process[0] != '+')
			write_utmp_wtmp("", ch->id, ch->pid, DEAD_PROCESS, NULL);
	    }

	if ((pid = fork()) == 0) {
		/*
		 *	Child sends state information to the parent.
		 */
		send_state(fd);
		exit(0);
	}

	/*
	 *	The existing init process execs a new init binary.
	 */
	env = init_buildenv(0);
	execle(myname, myname, "--init", NULL, env);

	/*
	 *	We shouldn't be here, something failed. 
	 *	Bitch, close the state pipe, unblock signals and return.
	 */
	close(fd);
	close(STATE_PIPE);
	sigprocmask(SIG_SETMASK, &oldset, NULL);
	init_freeenv(env);
	initlog(L_CO, "Attempt to re-exec failed");
}

/*
 *	Redo utmp/wtmp entries if required or requested
 *	Check for written records and size of utmp
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
 * @brief 检查已经写入 utmp/wtmp 的记录，包括 utmp 的大小。再写入一条 reboot 或者 runlevel 的记录。
 */
static
void redo_utmp_wtmp(void)
{
	struct stat ustat;
	const int ret = stat(UTMP_FILE, &ustat);

	if ((ret < 0) || (ustat.st_size == 0))
		wrote_utmp_rlevel = wrote_utmp_reboot = 0;

	if ((wrote_wtmp_reboot == 0) || (wrote_utmp_reboot == 0))
		write_utmp_wtmp("reboot", "~~", 0, BOOT_TIME, "~");

	if ((wrote_wtmp_rlevel == 0) || (wrote_wtmp_rlevel == 0))
		write_utmp_wtmp("runlevel", "~~", thislevel + 256 * prevlevel, RUN_LVL, "~");
}

/*
 *	We got a change runlevel request through the
 *	init.fifo. Process it.
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
 * @brief 真正完成改变 runlevel 的 request 请求，目标为传入参数 level，通过重新读取 inittab 文件来启动与新 runlevel 匹配的命令脚本。
 *
 * @details 函数执行流程分析：

	1. 如果传入参数 level 和当前的 runlevel 运行级别一致，则无需修改直接返回。

	2. 如果新的 runlevel = 'U'，则通过调用 re_exec() 来执行改变 runlevel 的操作。

	3. 如果新的 runlevel ！= 'U'，则通过调用 read_inittab() 来重新生成 family 链表。
 *
 */
static
void fifo_new_level(int level)
{
#if CHANGE_WAIT
	CHILD	*ch;
#endif
	int	oldlevel;

	if (level == runlevel) return;

#if CHANGE_WAIT
	/* Are we waiting for a child? */
	for(ch = family; ch; ch = ch->next)
		if (ch->flags & WAITING) break;
	if (ch == NULL)
#endif
	{
		/* We need to go into a new runlevel */
		oldlevel = runlevel;
		runlevel = read_level(level);
		if (runlevel == 'U') {
			runlevel = oldlevel;
			re_exec();
		} else {
			if (oldlevel != 'S' && runlevel == 'S') console_stty();
			if (runlevel == '6' || runlevel == '0' ||
			    runlevel == '1') console_stty();
			if (runlevel  > '1' && runlevel  < '6') redo_utmp_wtmp();
			read_inittab();
			fail_cancel();
			setproctitle("init [%c]", (int)runlevel);
		}
	}
}


/*
 *	Set/unset environment variables. The variables are
 *	encoded as KEY=VAL\0KEY=VAL\0\0. With "=VAL" it means
 *	setenv, without it means unsetenv.
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
 * @brief 设置或取消环境变量，类似 KEY=VAL 表示设置 如果没有 =VAL 表示取消
 */
static
void initcmd_setenv(char *data, int size)
{
	char		*env, *p, *e, *eq;
	int		i, sz;

	e = data + size;

	while (*data && data < e) {
		eq = NULL;
		for (p = data; *p && p < e; p++)
			if (*p == '=') eq = p;
		if (*p) break;
		env = data;
		data = ++p;

		sz = eq ? (eq - env) : (p - env);

		/*initlog(L_SY, "init_setenv: %s, %s, %d", env, eq, sz);*/

		/*
		 *	We only allow INIT_* to be set.
		 */
		if (strncmp(env, "INIT_", 5) != 0)
			continue;

		/* Free existing vars. */
		for (i = 0; i < NR_EXTRA_ENV; i++) {
			if (extra_env[i] == NULL) continue;
			if (!strncmp(extra_env[i], env, sz) &&
			    extra_env[i][sz] == '=') {
				free(extra_env[i]);
				extra_env[i] = NULL;
			}
		}

		/* Set new vars if needed. */
		if (eq == NULL) continue;
		for (i = 0; i < NR_EXTRA_ENV; i++) {
			if (extra_env[i] == NULL) {
				extra_env[i] = istrdup(env);
				break;
			}
		}
	}
}


/*
 *	Read from the init FIFO. Processes like telnetd and rlogind can
 *	ask us to create login processes on their behalf.
 *
 *	FIXME:	this needs to be finished. NOT that it is buggy, but we need
 *		to add the telnetd/rlogind stuff so people can start using it.
 *		Maybe move to using an AF_UNIX socket so we can use
 *		the 2.2 kernel credential stuff to see who we're talking to.
 *	
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
 * @brief 主要用于 init daemon 程序中，通过 select 函数监听来自于 /dev/initctl 管道的请求 request，分析并执行该请求 request。
 *
 * @details 函数执行流程分析：

	1. 如果 /etc/initctl 管道不存在，则创建这个管道，并设置权限 0600，只允许 root 用户读写。

	2. 如果管道已经打开，则比较该管道是否是最初原始打开的管道。如果不是，则关闭后，重新打开。

	3. 以读写+非阻塞方式打开管道，并且使用 dup2 将采用 PIPE_FD = 10 来使用管道，而不使用 0，1，2

	4. 使用 select 调用在该管道上等待来自于 init N 的切换运行级别的请求 request

	5. 一旦有来自这个管道的 request ，则检查这个 request 数据的合法性

	6. 对于输入正确的 request 请求，则分析是什么请求，并判断要采取什么动作。
	
	7. 请求包括进行 
		INIT_CMD_RUNLVL （runlevel 的切换） -> 调用 fifo_new_level()
		INIT_CMD_POWERFAIL
		INIT_CMD_POWERFAILNOW
		INIT_CMD_POWEROK (以上三个请求都是和电源事件有关)  -> 调用 do_power_fail()
		INIT_CMD_SETENV (设置环境变量) -> 调用 initcmd_setenv()
 *
 */

static
void check_init_fifo(void)
{
  struct init_request	request;
  struct timeval	tv;
  struct stat		st, st2;
  fd_set		fds;
  int			n;
  int			quit = 0;

  /*
   *	First, try to create /dev/initctl if not present.
   */
  if (stat(INIT_FIFO, &st2) < 0 && errno == ENOENT)
	(void)mkfifo(INIT_FIFO, 0600);

  /*
   *	If /dev/initctl is open, stat the file to see if it
   *	is still the _same_ inode.
   */
  if (pipe_fd >= 0) {
	fstat(pipe_fd, &st);
	if (stat(INIT_FIFO, &st2) < 0 ||
	    st.st_dev != st2.st_dev ||
	    st.st_ino != st2.st_ino) {
		close(pipe_fd);
		pipe_fd = -1;
	}
  }

  /*
   *	Now finally try to open /dev/initctl
   */
  if (pipe_fd < 0) {
	if ((pipe_fd = open(INIT_FIFO, O_RDWR|O_NONBLOCK)) >= 0) {
		fstat(pipe_fd, &st);
		if (!S_ISFIFO(st.st_mode)) {
			initlog(L_VB, "%s is not a fifo", INIT_FIFO);
			close(pipe_fd);
			pipe_fd = -1;
		}
	}
	if (pipe_fd >= 0) {
		/*
		 *	Don't use fd's 0, 1 or 2.
		 */
		(void) dup2(pipe_fd, PIPE_FD);
		close(pipe_fd);
		pipe_fd = PIPE_FD;

		/*
		 *	Return to caller - we'll be back later.
		 */
	}
  }

  /* Wait for data to appear, _if_ the pipe was opened. */
  if (pipe_fd >= 0) while(!quit) {

	/* Do select, return on EINTR. */
	FD_ZERO(&fds);
	FD_SET(pipe_fd, &fds);
	tv.tv_sec = 5;
	tv.tv_usec = 0;
	n = select(pipe_fd + 1, &fds, NULL, NULL, &tv);
	if (n <= 0) {
		if (n == 0 || errno == EINTR) return;
		continue;
	}

	/* Read the data, return on EINTR. */
	n = read(pipe_fd, &request, sizeof(request));
	printf("read fifo %d bytes\n", n);
	if (n == 0) {
		/*
		 *	End of file. This can't happen under Linux (because
		 *	the pipe is opened O_RDWR - see select() in the
		 *	kernel) but you never know...
		 */
		close(pipe_fd);
		pipe_fd = -1;
		return;
	}
	if (n <= 0) {
		if (errno == EINTR) return;
		initlog(L_VB, "error reading initrequest");
		continue;
	}

	/*
	 *	This is a convenient point to also try to
	 *	find the console device or check if it changed.
	 */
	console_init();

	/*
	 *	Process request.
	 */
	printf("request size need to be = %d\n", sizeof(request));
	printf("request.magic = %x, should be %x\n", request.magic, INIT_MAGIC);
	if (request.magic != INIT_MAGIC || n != sizeof(request)) {
		initlog(L_VB, "got bogus initrequest");
		printf("request to be = %d\n", sizeof(request));
		continue;
	}
	
	initlog(L_VB, "request: %d, runlevel: %c\n", request.cmd, request.runlevel);
	printf("request: %d, runlevel: %c\n", request.cmd, request.runlevel);
	switch(request.cmd) {
		case INIT_CMD_RUNLVL:
			sltime = request.sleeptime;
			fifo_new_level(request.runlevel);
			quit = 1;
			break;
		case INIT_CMD_POWERFAIL:
			sltime = request.sleeptime;
			do_power_fail('F');
			quit = 1;
			break;
		case INIT_CMD_POWERFAILNOW:
			sltime = request.sleeptime;
			do_power_fail('L');
			quit = 1;
			break;
		case INIT_CMD_POWEROK:
			sltime = request.sleeptime;
			do_power_fail('O');
			quit = 1;
			break;
		case INIT_CMD_SETENV:
			initcmd_setenv(request.i.data, sizeof(request.i.data));
			break;
		default:
			initlog(L_VB, "got unimplemented initrequest.");
			break;
	}
  }

  /*
   *	We come here if the pipe couldn't be opened.
   */
  if (pipe_fd < 0) pause();

}


/*
 *	This function is used in the transition
 *	sysinit (-> single user) boot -> multi-user.
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
 * @brief 实现一个启动过程中所需要的状态机，完成状态的迁移。
 *
 * @details 函数执行流程分析：

	1. 以 runlevel 代表状态，如果当前 runlevel = '#' 状态开始，系统进入 SYSINIT -> BOOT 的转变。

	2. 如果在 read_inittab 时从文件中获得了 def_level,则直接用这个变量的值，否则通过 get_init_default() 得到的是默认的运行级别并赋值给 newlevel

	3. 如果 newlevel 是 'S', 则下一个状态为 'S'，否则下一个状态设为 '*'

	4. 如果当前 runlevel 是 '*'，则系统从 BOOT -> NORMAL。

	5. 如果当前 runlevel 是 'S'，则代表着 SU 模式已经结束，重新调用 get_init_default() 得到新的运行级别 newlevel.

	6. 将本次状态变迁的信息写入日志 write_utmp_wtmp()
 *
 */

static
void boot_transitions()
{
  CHILD		*ch;
  static int	newlevel = 0;
  static int	warn = 1;
  int		loglevel;
  int		oldlevel;

  /* Check if there is something to wait for! */
  for( ch = family; ch; ch = ch->next )
	if ((ch->flags & RUNNING) && ch->action != BOOT) break;
     
  if (ch == NULL) {
	/* No processes left in this level, proceed to next level. */
	loglevel = -1;
	oldlevel = 'N';
	switch(runlevel) {
		case '#': /* SYSINIT -> BOOT */
			INITDBG(L_VB, "SYSINIT -> BOOT");

			/* Write a boot record. */
			wrote_utmp_reboot = 0;
			wrote_wtmp_reboot = 0;
			write_utmp_wtmp("reboot", "~~", 0, BOOT_TIME, "~");

  			/* Get our run level */
  			newlevel = dfl_level ? dfl_level : get_init_default();
			if (newlevel == 'S') {
				runlevel = newlevel;
				/* Not really 'S' but show anyway. */
				setproctitle("init [S]");
			} else
				runlevel = '*';
			break;
		case '*': /* BOOT -> NORMAL */
			INITDBG(L_VB, "BOOT -> NORMAL");
			if (runlevel != newlevel)
				loglevel = newlevel;
			runlevel = newlevel;
			did_boot = 1;
			warn = 1;
			break;
		case 'S': /* Ended SU mode */
		case 's':
			INITDBG(L_VB, "END SU MODE");
			newlevel = get_init_default();
			if (!did_boot && newlevel != 'S')
				runlevel = '*';
			else {
				if (runlevel != newlevel)
					loglevel = newlevel;
				runlevel = newlevel;
				oldlevel = 'S';
			}
			warn = 1;
			for(ch = family; ch; ch = ch->next)
			    if (strcmp(ch->rlevel, "S") == 0)
				ch->flags &= ~(FAILING|WAITING|XECUTED);
			break;
		default:
			if (warn)
			  initlog(L_VB,
				"no more processes left in this runlevel");
			warn = 0;
			loglevel = -1;
			if (got_signals == 0)
				check_init_fifo();
			break;
	}
	if (loglevel > 0) {
		initlog(L_VB, "Entering runlevel: %c", runlevel);
		wrote_utmp_rlevel = 0;
		wrote_wtmp_rlevel = 0;
		write_utmp_wtmp("runlevel", "~~", runlevel + 256 * oldlevel, RUN_LVL, "~");
		thislevel = runlevel;
		prevlevel = oldlevel;
		setproctitle("init [%c]", (int)runlevel);
	}
  }
}

/*
 *	Init got hit by a signal. See which signal it is,
 *	and act accordingly.
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
 * @brief 根据全局变量 got_signals 中哪些标志位被设置了，获得信号类型，进行相应的处理。
 *
 * @details 函数执行流程分析：

	程序执行逻辑很简单，就是依次判别 ISMEMBER(got_signals, SIGXXXX) 对于以下信号进行相应处理。

	1. SIGPWR 信号 -> do_power_fail()

	2. SIGINT 信号 -> 通知 ctrlaltdel 入口启动

	3. SIGWINCH 信号 -> 通知 KBREQUEST 入口启动

	4. SIGALRM 信号 -> 定时器到时，忽略

	5. SIGCHLD 信号 -> 查看是哪个子进程结束，调用 write_utmp_wtmp() 写入日志

	6. SIGHUP 信号 -> 是否在等待子进程，进行 runlevel 切换

	7. SIGUSR1 信号 -> 这个信号代表要求关闭然后重新打开 /dev/initctl
 *
 */
static
void process_signals()
{
  CHILD		*ch;
  int		pwrstat;
  int		oldlevel;
  int		fd;
  char		c;

  if (ISMEMBER(got_signals, SIGPWR)) {
	INITDBG(L_VB, "got SIGPWR");
	/* See _what_ kind of SIGPWR this is. */
	pwrstat = 0;
	if ((fd = open(PWRSTAT, O_RDONLY)) >= 0) {
		c = 0;
		read(fd, &c, 1);
		pwrstat = c;
		close(fd);
		unlink(PWRSTAT);
	} else if ((fd = open(PWRSTAT_OLD, O_RDONLY)) >= 0) {
		/* Path changed 2010-03-20.  Look for the old path for a while. */
		initlog(L_VB, "warning: found obsolete path %s, use %s instead",
			PWRSTAT_OLD, PWRSTAT);
		c = 0;
		read(fd, &c, 1);
		pwrstat = c;
		close(fd);
		unlink(PWRSTAT_OLD);
        }
	do_power_fail(pwrstat);
	DELSET(got_signals, SIGPWR);
  }

  if (ISMEMBER(got_signals, SIGINT)) {
#if defined(SIGINT_ONLYONCE) && (SIGINT_ONLYONCE == 1)
	/* Ignore any further signal from keyboard */
	struct sigaction sa;
	SETSIG(sa, SIGINT, SIG_IGN, SA_RESTART);
#endif
	INITDBG(L_VB, "got SIGINT");
	/* Tell ctrlaltdel entry to start up */
	for(ch = family; ch; ch = ch->next)
		if (ch->action == CTRLALTDEL)
			ch->flags &= ~XECUTED;
	DELSET(got_signals, SIGINT);
  }

  if (ISMEMBER(got_signals, SIGWINCH)) {
	INITDBG(L_VB, "got SIGWINCH");
	/* Tell kbrequest entry to start up */
	for(ch = family; ch; ch = ch->next)
		if (ch->action == KBREQUEST)
			ch->flags &= ~XECUTED;
	DELSET(got_signals, SIGWINCH);
  }

  if (ISMEMBER(got_signals, SIGALRM)) {
	INITDBG(L_VB, "got SIGALRM");
	/* The timer went off: check it out */
	DELSET(got_signals, SIGALRM);
  }

  if (ISMEMBER(got_signals, SIGCHLD)) {
	INITDBG(L_VB, "got SIGCHLD");
	/* First set flag to 0 */
	DELSET(got_signals, SIGCHLD);

	/* See which child this was */
	for(ch = family; ch; ch = ch->next)
	    if (ch->flags & ZOMBIE) {
		INITDBG(L_VB, "Child died, PID= %d", ch->pid);
		ch->flags &= ~(RUNNING|ZOMBIE|WAITING);
		if (ch->process[0] != '+')
			write_utmp_wtmp("", ch->id, ch->pid, DEAD_PROCESS, NULL);
	    }

  }

  if (ISMEMBER(got_signals, SIGHUP)) {
	INITDBG(L_VB, "got SIGHUP");
#if CHANGE_WAIT
	/* Are we waiting for a child? */
	for(ch = family; ch; ch = ch->next)
		if (ch->flags & WAITING) break;
	if (ch == NULL)
#endif
	{
		/* We need to go into a new runlevel */
		oldlevel = runlevel;
#ifdef INITLVL
		runlevel = read_level(0);
#endif
		if (runlevel == 'U') {
			runlevel = oldlevel;
			re_exec();
		} else {
			if (oldlevel != 'S' && runlevel == 'S') console_stty();
			if (runlevel == '6' || runlevel == '0' ||
			    runlevel == '1') console_stty();
			read_inittab();
			fail_cancel();
			setproctitle("init [%c]", (int)runlevel);
			DELSET(got_signals, SIGHUP);
		}
	}
  }
  if (ISMEMBER(got_signals, SIGUSR1)) {
	/*
	 *	SIGUSR1 means close and reopen /dev/initctl
	 */
	INITDBG(L_VB, "got SIGUSR1");
	close(pipe_fd);
	pipe_fd = -1;
	DELSET(got_signals, SIGUSR1);
  }
}

/*
 *	The main loop
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
 * @brief 切换运行级别，检查出错情况，接受信号，启动相应服务例程。
 *
	1. 调用 init_reboot 宏定义（其实就是 reboot 函数）告诉内核，当 ctrl + alt + del 三个键被同时按下时，给当前进程发送 SIGINT 信号，以便 init 进程可以处理来自键盘的这一信号，进一步决定采取何种动作。

	2. 接下来将会安装一些信号处理函数。如下：

	signal_handler(),处理SIGALRM，SIGHUP，SIGINT，SIGPWR，SIGWINCH，SIGUSR1
	chld_handler()，处理SIGCHLD
	stop_handler()，处理SIGSTOP，SIGTSTP
	cont_handler()，处理SIGCONT
	segv_handler()，处理SIGSEGV

	3. 然后初始化终端，调用 console_init 函数。这个函数我们在下面也会再次详细分析。

	4. 终端初始化完成后，接着对 reload 这个变量进行判别，是否属于是首次执行？ 

	5. 如果是首次执行，则依次执行下列步骤：
		5.1 关闭所有打开文件0，1，2，
		5.2 然后调用 console_stty() 函数对终端进行设置，主要是通过 tcsetattr() 函数来设置一些快捷键。
		5.3 以覆盖 overwrite 方式设置 PATH 环境变量，通过 PATH_DEFAULT 宏定义，默认值是  "/sbin:/usr/sbin:/bin:/usr/bin"
		5.4 初始化 /var/run/utmp 文件。通过日志输出 booting 信息
		5.5 如果 emerg_shell 被设置（参数中有-b或者emergency），表示需要启动 emergency shell，则通过调用 spawn()初始化 emergency shell 子进程，并等待该子进程退出。
		5.6 设置当前的 runlevel = '#', 表示这是正常的 Kernel 首次启动 init 的方式 SYSINIT。
		5.7 当从 emergency shell退出（或者不需要 emergency shell 的话），则调用 read_inittab() 来读入 /etc/inittab 文件。该函数主要将 /etc/inittab 文件解析的结果存入CHILD类型的链表family上，供之后的执行使用。

	6. 如果不是首次执行，也就是 reload 为真，则只执行下列步骤：
		6.1 通过日志输出 reloading 信息
		6.2 以非覆盖 non overwrite 方式设置 PATH 环境变量，通过 PATH_DEFAULT 宏定义，默认值是  "/sbin:/usr/sbin:/bin:/usr/bin"

	7. 5或者6执行完之后，调用 start_if_needed() 函数，启动需要在相应运行级别中运行的程序和服务。而该函数主要又是通过调用startup()函数，继而调用spawn()来启动程序或者服务的运行的。

	8. 在此之后，init_main() 就进入一个主循环中，主要完成切换运行级别，检查出错情况，接受信号，启动相应服务例程。
	在这个主循环中，需要调用如下这些重要的函数：

		boot_transitions() -> get_init_default() -> ask_runlevel()
		check_init_fifo() -> console_init()
		fail_check()
		process_signals() -> console_stty()
		start_if_needed() -> startup() -> spawn()

 */
static
void init_main(void)
{
  CHILD			*ch;
  struct sigaction	sa;
  sigset_t		sgt;
  int			f, st;

  if (!reload) {
  
#if INITDEBUG
	/*
	 * Fork so we can debug the init process.
	 */
	if ((f = fork()) > 0) {
		static const char killmsg[] = "PRNT: init killed.\r\n";
		pid_t rc;

		while((rc = wait(&st)) != f)
			if (rc < 0 && errno == ECHILD)
				break;
		write(1, killmsg, sizeof(killmsg) - 1);
		while(1) pause();
	}
#endif

#ifdef __linux__
	/*
	 *	Tell the kernel to send us SIGINT when CTRL-ALT-DEL
	 *	is pressed, and that we want to handle keyboard signals.
	 */
	init_reboot(BMAGIC_SOFT);
	if ((f = open(VT_MASTER, O_RDWR | O_NOCTTY)) >= 0) {
		(void) ioctl(f, KDSIGACCEPT, SIGWINCH);
		close(f);
	} else
		(void) ioctl(0, KDSIGACCEPT, SIGWINCH);
#endif

	/*
	 *	Ignore all signals.
	 */
	for(f = 1; f <= NSIG; f++)
		SETSIG(sa, f, SIG_IGN, SA_RESTART);
  }

  SETSIG(sa, SIGALRM,  signal_handler, 0);
  SETSIG(sa, SIGHUP,   signal_handler, 0);
  SETSIG(sa, SIGINT,   signal_handler, 0);
  SETSIG(sa, SIGCHLD,  chld_handler, SA_RESTART);
  SETSIG(sa, SIGPWR,   signal_handler, 0);
  SETSIG(sa, SIGWINCH, signal_handler, 0);
  SETSIG(sa, SIGUSR1,  signal_handler, 0);
  SETSIG(sa, SIGSTOP,  stop_handler, SA_RESTART);
  SETSIG(sa, SIGTSTP,  stop_handler, SA_RESTART);
  SETSIG(sa, SIGCONT,  cont_handler, SA_RESTART);
  SETSIG(sa, SIGSEGV,  (void (*)(int))segv_handler, SA_RESTART);

  console_init();

  if (!reload) {
	int fd;

  	/* Close whatever files are open, and reset the console. */
#if 0
	close(0);
	close(1);
	close(2);
#endif
  	console_stty();
  	setsid();

  	/*
	 *	Set default PATH variable.
	 */
  	setenv("PATH", PATH_DEFAULT, 1 /* Overwrite */);

  	/*
	 *	Initialize /var/run/utmp (only works if /var is on
	 *	root and mounted rw)
	 */
	if ((fd = open(UTMP_FILE, O_WRONLY|O_CREAT|O_TRUNC, 0644)) >= 0)
		close(fd);

  	/*
	 *	Say hello to the world
	 */
  	initlog(L_CO, bootmsg, "booting");

  	/*
	 *	See if we have to start an emergency shell.
	 */
	if (emerg_shell) {
		pid_t rc;
		SETSIG(sa, SIGCHLD, SIG_DFL, SA_RESTART);
		if (spawn(&ch_emerg, &f) > 0) {
			while((rc = wait(&st)) != f)
				if (rc < 0 && errno == ECHILD)
					break;
		}
  		SETSIG(sa, SIGCHLD,  chld_handler, SA_RESTART);
  	}

  	/*
	 *	Start normal boot procedure.
	 */
  	runlevel = '#';
  	read_inittab();
  
  } else {
	/*
	 *	Restart: unblock signals and let the show go on
	 */
	initlog(L_CO, bootmsg, "reloading");
	sigfillset(&sgt);
	sigprocmask(SIG_UNBLOCK, &sgt, NULL);

  	/*
	 *	Set default PATH variable.
	 */
  	setenv("PATH", PATH_DEFAULT, 0 /* Don't overwrite */);
  }
  start_if_needed();

  while(1) {

     /* See if we need to make the boot transitions. */
     boot_transitions();
     INITDBG(L_VB, "init_main: waiting..");

     /* Check if there are processes to be waited on. */
     for(ch = family; ch; ch = ch->next)
	if ((ch->flags & RUNNING) && ch->action != BOOT) break;

#if CHANGE_WAIT
     /* Wait until we get hit by some signal. */
     while (ch != NULL && got_signals == 0) {
	if (ISMEMBER(got_signals, SIGHUP)) {
		/* See if there are processes to be waited on. */
		for(ch = family; ch; ch = ch->next)
			if (ch->flags & WAITING) break;
	}
	if (ch != NULL) check_init_fifo();
     }
#else /* CHANGE_WAIT */
     if (ch != NULL && got_signals == 0) check_init_fifo();
#endif /* CHANGE_WAIT */

     /* Check the 'failing' flags */
     fail_check();

     /* Process any signals. */
     process_signals();

     /* See what we need to start up (again) */
     start_if_needed();
  }
  /*NOTREACHED*/
}

/*
 * Tell the user about the syntax we expect.
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
static
void usage(char *s)
{
	fprintf(stderr, "Usage: %s {-e VAR[=VAL] | [-t SECONDS] {0|1|2|3|4|5|6|S|s|Q|q|A|a|B|b|C|c|U|u}}\n", s);
	exit(1);
}

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
 * @brief 通过 /etc/initctl 管道向 init 进程发送控制命令
 *
 * @details 在执行 telinit 函数时，实际上是通过向INIT_FIFO（/dev/initctl）写入命令的方式，通知 init 执行相应的操作。Telinit()根据不同请求，构造如下结构体类型的变量并向INIT_FIFO（/dev/initctl）写入该请求来完成其使命
 *
 */
static
int telinit(char *progname, int argc, char **argv)
{
#ifdef TELINIT_USES_INITLVL
	FILE			*fp;
#endif
	struct init_request	request;
	struct sigaction	sa;
	int			f, fd, l;
	char			*env = NULL;

	memset(&request, 0, sizeof(request));
	request.magic     = INIT_MAGIC;

	while ((f = getopt(argc, argv, "t:e:")) != EOF) switch(f) {
		case 't':
			sltime = atoi(optarg);
			break;
		case 'e':
			if (env == NULL)
				env = request.i.data;
			l = strlen(optarg);
			if (env + l + 2 > request.i.data + sizeof(request.i.data)) {
				fprintf(stderr, "%s: -e option data "
					"too large\n", progname);
				exit(1);
			}
			memcpy(env, optarg, l);
			env += l;
			*env++ = 0;
			break;
		default:
			usage(progname);
			break;
	}

	if (env) *env++ = 0;

	if (env) {
		if (argc != optind)
			usage(progname);
		request.cmd = INIT_CMD_SETENV;
	} else {
		if (argc - optind != 1 || strlen(argv[optind]) != 1)
			usage(progname);
		if (!strchr("0123456789SsQqAaBbCcUu", argv[optind][0]))
			usage(progname);
		request.cmd = INIT_CMD_RUNLVL;
		request.runlevel  = env ? 0 : argv[optind][0];
		request.sleeptime = sltime;
	}

	/* Change to the root directory. */
	chdir("/");

	/* Open the fifo and write a command. */
	/* Make sure we don't hang on opening /dev/initctl */
	SETSIG(sa, SIGALRM, signal_handler, 0);
	alarm(3);
	if ((fd = open(INIT_FIFO, O_WRONLY)) >= 0) {
		ssize_t p = 0;
		size_t s  = sizeof(request);
		void *ptr = &request;

		while (s > 0) {
			INITDBG(L_VB, "write to INIT_FIFO\n");
			p = write(fd, ptr, s);
			if (p < 0) {
				if (errno == EINTR || errno == EAGAIN)
					continue;
				break;
			}
			ptr += p;
			s -= p;
		}
		close(fd);
		alarm(0);
		return 0;
	}

#ifdef TELINIT_USES_INITLVL
	if (request.cmd == INIT_CMD_RUNLVL) {
		/* Fallthrough to the old method. */

		/* Now write the new runlevel. */
		if ((fp = fopen(INITLVL, "w")) == NULL) {
			fprintf(stderr, "%s: cannot create %s\n",
				progname, INITLVL);
			exit(1);
		}
		fprintf(fp, "%s %d", argv[optind], sltime);
		fclose(fp);

		/* And tell init about the pending runlevel change. */
		if (kill(INITPID, SIGHUP) < 0) perror(progname);

		return 0;
	}
#endif

	fprintf(stderr, "%s: ", progname);
	if (ISMEMBER(got_signals, SIGALRM)) {
		fprintf(stderr, "timeout opening/writing control channel %s\n",
			INIT_FIFO);
	} else {
		perror(INIT_FIFO);
	}
	return 1;
}

/*
 * Main entry for init and telinit.
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
 * @brief init 命令的主函数执行流程分析
 *
	 在 main 函数中主要负责完成以下工作：

	 1. 获取 argv[0] 参数，用以判断用户执行了 init 还是 telinit，因为 telinit 是指向 init 程序的软链接。

	 2. 检查当前执行用户的权限，必须是 superuser，否则直接退出。

	 3. 通过 getpid() 获取当前执行进程的 pid，判断是否为 1 （1 表示是通过内核调用执行的第一个进程，而不是通过用户来执行 init 程序启动的进程）。（同时从源码中可以看出，init 程序也支持用 -i 或者 --init 参数来表示当前要求执行的是 init 进程。不过这个方式在 man -l init.8 的 man page 中没有明确提供此信息）

	 4. 如果不是要求执行 init 进程，则转交控制权给 telinit(p, argc, argv) 函数进行处理。在后面介绍 telinit 函数的地方，我们再对此做详细说明。

	 5. 如果是要求执行 init 进程，还需要接着进行检查是否是属于 re-exec ，也就是重新执行，而不是首次执行。判断思路是通过读取STATE_PIPE，看是否收到一个Signature = "12567362"的字符串来确定。如果是重新执行，则将 reload 全局变量置为1。re-exec 和首次执行最大的区别是没有对/etc/inittab 进行解析，在后面我们会再次提到，为保持思路直接和简单，我们在这里不展开，直奔 init 进程中最关键的代码。

	 6. 如果是属于 init 进程的首次执行，则需要对 argv[] 的参数进行相应处理，简单说来，就是把 -s single 或者 0123456789 这样的数字，转换为 dfl_level 变量，这个变量代表的就是默认的运行级别。

	 7. 如果宏定义了 WITH_SELINUX ，则会通过调用 is_selinux_enabled()判断是否系统使能了 SELINUX, 如果是，则在通过调用 selinux_init_load_policy 来加载策略，最后通过 execv 来再执行 init 。

	 8. 在进行一系列判断检测之后，通过传递 argv[0] -> argv0 这个全局变量，最终调用了 init_main()进入标准的 init 主函数中。

*/
int main(int argc, char **argv)
{
	char			*p;
	int			f;
	int			isinit;
#ifdef WITH_SELINUX
	int			enforce = 0;
#endif

	/* Get my own name */
	if ((p = strrchr(argv[0], '/')) != NULL)
  		p++;
	else
  		p = argv[0];

	/* Common umask */
	umask(022);

	/* Quick check */
	if (geteuid() != 0) {
		fprintf(stderr, "%s: must be superuser.\n", p);
		exit(1);
	}

	/*
	 *	Is this telinit or init ?
	 */
	isinit = (getpid() == 1);
	for (f = 1; f < argc; f++) {
		if (!strcmp(argv[f], "-i") || !strcmp(argv[f], "--init")) {
			isinit = 1;
			break;
		}
	}
	if (!isinit) exit(telinit(p, argc, argv));

	/*
	 *	Check for re-exec
	 */ 	
	if (check_pipe(STATE_PIPE)) {

		receive_state(STATE_PIPE);

		myname = istrdup(argv[0]);
		argv0 = argv[0];
		maxproclen = 0;
		for (f = 0; f < argc; f++)
			maxproclen += strlen(argv[f]) + 1;
		reload = 1;
		setproctitle("init [%c]", (int)runlevel);

		init_main();
	}

  	/* Check command line arguments */
	maxproclen = strlen(argv[0]) + 1;
  	for(f = 1; f < argc; f++) {
		if (!strcmp(argv[f], "single") || !strcmp(argv[f], "-s"))
			dfl_level = 'S';
		else if (!strcmp(argv[f], "-a") || !strcmp(argv[f], "auto"))
			putenv("AUTOBOOT=YES");
		else if (!strcmp(argv[f], "-b") || !strcmp(argv[f],"emergency"))
			emerg_shell = 1;
		else if (!strcmp(argv[f], "-z")) {
			/* Ignore -z xxx */
			if (argv[f + 1]) f++;
		} else if (strchr("0123456789sS", argv[f][0])
			&& strlen(argv[f]) == 1)
			dfl_level = argv[f][0];
		/* "init u" in the very beginning makes no sense */
		if (dfl_level == 's') dfl_level = 'S';
		maxproclen += strlen(argv[f]) + 1;
	}

#ifdef WITH_SELINUX
	if (getenv("SELINUX_INIT") == NULL) {
	  const int rc = mount("proc", "/proc", "proc", 0, 0);
	  if (is_selinux_enabled() > 0) {
	    putenv("SELINUX_INIT=YES");
	    if (rc == 0) umount2("/proc", MNT_DETACH);
	    if (selinux_init_load_policy(&enforce) == 0) {
	      execv(myname, argv);
	    } else {
	      if (enforce > 0) {
		/* SELinux in enforcing mode but load_policy failed */
		/* At this point, we probably can't open /dev/console, so log() won't work */
		fprintf(stderr,"Unable to load SELinux Policy. Machine is in enforcing mode. Halting now.\n");
		exit(1);
	      }
	    }
	  }
	  if (rc == 0) umount2("/proc", MNT_DETACH);
	}
#endif  
	/* Start booting. */
	argv0 = argv[0];
	argv[1] = NULL;
	setproctitle("init boot");
	init_main();

	/*NOTREACHED*/
	return 0;
}

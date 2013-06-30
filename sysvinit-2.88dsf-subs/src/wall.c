/*
 * wall.c	Write to all users logged in.
 *
 * Usage:	wall [text]
 *
 * Version:	@(#)wall  2.79  12-Sep-2000  miquels@cistron.nl
 *
 *		This file is part of the sysvinit suite,
 *		Copyright (C) 1991-2000 Miquel van Smoorenburg.
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include <syslog.h>
#include "init.h"


char *Version = "@(#) wall 2.79 12-Sep-2000 miquels@cistron.nl";
#define MAXLEN 4096
#define MAXLINES 20

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
 * @brief wall 命令实现的主函数，调用 wall() 函数完成消息发送
 *
 * @details wall 命令详细用法

   wall命令用来向所有用户的终端发送一条信息。发送的信息可以作为参数在命令行给出，也可在执行wall命令后，从终端中输入。使用终端输入信息时，按Ctrl-D结束输入。wall的信息长度的限制是20行。只有超级用户有权限，给所有用户的终端发送消息。

	* 命令格式
		wall [-n] [ message ]

	* 用法  
		usage: wall [message]

	* 举例  
		wall "hello msg"
 *
 */
int main(int argc, char **argv)
{
  char buf[MAXLEN];
  char line[83];
  int i, f, ch;
  int len = 0;
  int remote = 0;
  char *p;
  char *whoami;
  struct passwd *pwd;

  buf[0] = 0;
  if ((pwd = getpwuid(getuid())) == NULL) {
	if (getuid() == 0)
		whoami = "root";
	else {
		fprintf(stderr, "You don't exist. Go away.\n");
		exit(1);
	}
  } else
	whoami = pwd->pw_name;

  while((ch = getopt(argc, argv, "n")) != EOF)
	switch(ch) {
		case 'n':
			/*
			 *	Undocumented option for suppressing
			 *	banner from rpc.rwalld. Only works if
			 *	we are root or if we're NOT setgid.
			 */
			if (geteuid() != 0 && getgid() != getegid()) {
				fprintf(stderr, "wall -n: not priviliged\n");
				exit(1);
			}
			remote = 1;
			break;
		default:
			fprintf(stderr, "usage: wall [message]\n");
			return 1;
			break;
	}

  if ((argc - optind) > 0) {
	for(f = optind; f < argc; f++) {
		len += strlen(argv[f]) + 1;
		if (len >= MAXLEN-2) break;
		strcat(buf, argv[f]);
		if (f < argc-1) strcat(buf, " ");
	}
	strcat(buf, "\r\n");
  } else {
	while(fgets(line, 80, stdin)) {
		/*
		 *	Make sure that line ends in \r\n
		 */
		for(p = line; *p && *p != '\r' && *p != '\n'; p++)
			;
		strcpy(p, "\r\n");
		len += strlen(line);
		if (len >= MAXLEN) break;
		strcat(buf, line);
	}
  }

  i = 0;
  for (p = buf; *p; p++) {
	if (*p == '\n' && i++ > MAXLINES) {
		*++p = 0;
		break;
	}
  }

  openlog("wall", LOG_PID, LOG_USER);
  syslog(LOG_INFO, "wall: user %s broadcasted %d lines (%d chars)",
	whoami, i, strlen(buf));
  closelog();

  unsetenv("TZ");
  wall(buf, remote);

  /*NOTREACHED*/
  return 0;
}


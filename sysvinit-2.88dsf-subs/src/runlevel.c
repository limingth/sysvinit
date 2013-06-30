/*
 * runlevel	Prints out the previous and the current runlevel.
 *
 * Version:	@(#)runlevel  1.20  16-Apr-1997  MvS
 *
 *		This file is part of the sysvinit suite,
 *		Copyright (C) 1991-1997 Miquel van Smoorenburg.
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
#include <utmp.h>
#include <time.h>
#include <stdlib.h>

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
 * @brief runlevel 命令实现的主函数，调用 getutent() 获取当前运行级别，并打印出来
 *
 * @details runlevel 命令详细用法

   runlevel 命令读取系统的登录记录文件(一般是/var/run/utmp)把以前和当前的系统运行级输出到标准输出设备。
如果之前的系统运行级别没有找到，则会返回一个 N 字母来代替。

	* 命令格式
		runlevel [utmp]

	* 主要选项
		utmp   指定要读取的 utmp 文件名，默认是读取 /var/run/utmp


 *
 */
int main(argc, argv)
int argc;
char **argv;
{
  struct utmp *ut;
  char prev;

  if (argc > 1) utmpname(argv[1]);

  setutent();
  while ((ut = getutent()) != NULL) {
	if (ut->ut_type == RUN_LVL) {
		prev = ut->ut_pid / 256;
		if (prev == 0) prev = 'N';
		printf("%c %c\n", prev, ut->ut_pid % 256);
		endutent();
		exit(0);
	}
  }
  
  printf("unknown\n");
  endutent();
  return(1);
}


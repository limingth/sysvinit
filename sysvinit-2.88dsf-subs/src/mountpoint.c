/*
 * mountpoint	See if a directory is a mountpoint.
 *
 * Author:	Miquel van Smoorenburg.
 *
 * Version:	@(#)mountpoint  2.85-12  17-Mar-2004	 miquels@cistron.nl
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

#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <getopt.h>
#include <stdio.h>

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
 * @brief 给定一个目录路径 path，通过调用 stat 获得目录的相关属性，返回 struct stat 结构体
 *
 * 
           struct stat {
               dev_t     st_dev;     
               ino_t     st_ino;    
               mode_t    st_mode;   
               nlink_t   st_nlink;  
               uid_t     st_uid;    
               gid_t     st_gid;   
               dev_t     st_rdev;  
               off_t     st_size;  
               blksize_t st_blksize;
               blkcnt_t  st_blocks; 
               time_t    st_atime; 
               time_t    st_mtime; 
               time_t    st_ctime;
           };
 *
 */
int dostat(char *path, struct stat *st, int do_lstat, int quiet)
{
	int		n;

	if (do_lstat)
		n = lstat(path, st);
	else
		n = stat(path, st);

	if (n != 0) {
		if (!quiet)
			fprintf(stderr, "mountpoint: %s: %s\n", path,
				strerror(errno));
		return -1;
	}
	return 0;
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
 * @brief 通过 fprintf() 函数，向标准出错 stderr 打印该条命令的用户使用帮助信息
 */
void usage(void) {
	fprintf(stderr, "Usage: mountpoint [-q] [-d] [-x] path\n");
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
 * @brief mountpoint 命令实现的主函数，通过比较当前目录和上一级目录的设备属性来进行判断
 *
 * @details mountpoint 命令详细用法

   mountpoint 检查给定的目录是否是一个挂载点。

	* 命令格式
		/bin/mountpoint [-q] [-d] /path/to/directory
	       /bin/mountpoint -x /dev/device

	* 主要选项
	       -q     Be quiet - don't print anything.

	       -d     Print major/minor device number of the filesystem on stdout.

	       -x     Print major/minor device number of the blockdevice on stdout.

 *
 */
int main(int argc, char **argv)
{
	struct stat	st, st2;
	char		buf[256];
	char		*path;
	int		quiet = 0;
	int		showdev = 0;
	int		xdev = 0;
	int		c, r;

	while ((c = getopt(argc, argv, "dqx")) != EOF) switch(c) {
		case 'd':
			showdev = 1;
			break;
		case 'q':
			quiet = 1;
			break;
		case 'x':
			xdev = 1;
			break;
		default:
			usage();
			break;
	}
	if (optind != argc - 1) usage();
	path = argv[optind];

	if (dostat(path, &st, !xdev, quiet) < 0)
		return 1;

	if (xdev) {
#ifdef __linux__
		if (!S_ISBLK(st.st_mode))
#else
		if (!S_ISBLK(st.st_mode) && !S_ISCHR(st.st_mode))
#endif
		{
			if (quiet)
				printf("\n");
			else
			fprintf(stderr, "mountpoint: %s: not a block device\n",
				path);
			return 1;
		}
		printf("%u:%u\n", major(st.st_rdev), minor(st.st_rdev));
		return 0;
	}

	if (!S_ISDIR(st.st_mode)) {
		if (!quiet)
			fprintf(stderr, "mountpoint: %s: not a directory\n",
				path);
		return 1;
	}

	memset(buf, 0, sizeof(buf));
	strncpy(buf, path, sizeof(buf) - 4);
	strcat(buf, "/..");
	if (dostat(buf, &st2, 0, quiet) < 0)
		return 1;

	r = (st.st_dev != st2.st_dev) ||
	    (st.st_dev == st2.st_dev && st.st_ino == st2.st_ino);

	if (!quiet && !showdev)
		printf("%s is %sa mountpoint\n", path, r ? "" : "not ");
	if (showdev)
		printf("%u:%u\n", major(st.st_dev), minor(st.st_dev));

	return r ? 0 : 1;
}

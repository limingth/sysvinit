/*
 * ifdown.c	Find all network interfaces on the system and
 *		shut them down.
 *
 * Copyright (C) 1998 Miquel van Smoorenburg.
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
char *v_ifdown = "@(#)ifdown.c  1.11  02-Jun-1998  miquels@cistron.nl";

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/errno.h>

#include <net/if.h>
#include <netinet/in.h>

#define MAX_IFS	64

/*
 *	First, we find all shaper devices and down them. Then we
 *	down all real interfaces. This is because the comment in the
 *	shaper driver says "if you down the shaper device before the
 *	attached inerface your computer will follow".
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
 * @brief 在 halt 命令关机过程中，找到所有的网络设备，并且把它们关闭。
 * 
 * @details 主要依靠调用 ioctl(fd, SIOCSIFFLAGS, &ifr[i]) 系统调用
 */
int ifdown(void)
{
	struct ifreq ifr[MAX_IFS];
	struct ifconf ifc;
	int i, fd;
	int numif;
	int shaper;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		fprintf(stderr, "ifdown: ");
		perror("socket");
		return -1;
	}
	ifc.ifc_len = sizeof(ifr);
	ifc.ifc_req = ifr;

	if (ioctl(fd, SIOCGIFCONF, &ifc) < 0) {
		fprintf(stderr, "ifdown: ");
		perror("SIOCGIFCONF");
		close(fd);
		return -1;
	}
	numif = ifc.ifc_len / sizeof(struct ifreq);

	for (shaper = 1; shaper >= 0; shaper--) {
		for (i = 0; i < numif; i++) {

			if ((strncmp(ifr[i].ifr_name, "shaper", 6) == 0)
			    != shaper) continue;

			if (strcmp(ifr[i].ifr_name, "lo") == 0)
				continue;
			if (strchr(ifr[i].ifr_name, ':') != NULL)
				continue;

			/* Read interface flags */
			if (ioctl(fd, SIOCGIFFLAGS, &ifr[i]) < 0) {
				fprintf(stderr, "ifdown: shutdown ");
				perror(ifr[i].ifr_name);
				continue;
			}
			/*
			 * Expected in <net/if.h> according to
			 * "UNIX Network Programming".
			 */
#ifdef ifr_flags
# define IRFFLAGS	ifr_flags
#else	/* Present on kFreeBSD */
# define IRFFLAGS	ifr_flagshigh
#endif
			if (ifr[i].IRFFLAGS & IFF_UP) {
				ifr[i].IRFFLAGS &= ~(IFF_UP);
				if (ioctl(fd, SIOCSIFFLAGS, &ifr[i]) < 0) {
					fprintf(stderr, "ifdown: shutdown ");
					perror(ifr[i].ifr_name);
				}
			}
#undef IRFFLAGS
		}
	}
	close(fd);

	return 0;
}

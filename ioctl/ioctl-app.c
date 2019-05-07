/*
 * @Author: northward
 * @Github: https://github.com/northwardtop
 * @LastEditors: northward
 * @Description: 本应用程序用于测试LED和KEY驱动程序
 * @Date: 2019-05-01 15:48:23
 * @LastEditTime: 2019-05-05 23:56:35
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "key-led.h"


int main(int argc, char **argv)
{
	int fd_led=-1,fd_key,ret=-1, len=0, i=0;
	int kval;
	
	fd_led=open("/dev/LEDs", O_WRONLY);
	fd_key=open("/dev/KEYs", O_RDONLY);
	if(fd_led<0 || fd_key<0)
		perror("open LEDs/KEYs error\n");

	while(1){
		ret = -1;
		ret=ioctl(fd_key, GEC6818_KALL_STAT, &kval);
		if(ret<0) {
			perror("ioctl error\n");
			continue;
		}

		//k2 k3 k4 k5
		int led; //led=D7~D10
		if(kval & 0x8)
			led=7;
		if(kval & 0x4)
			led=8;
		if(kval & 0x2)
			led=9;
		if(kval & 0x1)
			led=10;
		ioctl(fd_led, GEC6818_LED_ON, led);
	}
	return 0;
}

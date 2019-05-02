/*
 * @Author: northward
 * @Github: https://github.com/northwardtop
 * @LastEditors: northward
 * @Description: 本应用程序用于测试LED和KEY驱动程序
 * @Date: 2019-05-01 15:48:23
 * @LastEditTime: 2019-05-02 19:46:39
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fctl.h>


#define LED_NAME				"LEDs"
#define KEY_NAME				"KEYs"

#define GEC6818_KEY_ON			_IOW('K', 1, unsigned long)
#define GEC6818_KEY_OFF			_IOW('K', 2, unsigned long)

#define GEC6818_KEY2_STAT		_IOR('K', 1, unsigned long)
#define GEC6818_KEY3_STAT		_IOR('K', 2, unsigned long)
#define GEC6818_KEY4_STAT		_IOR('K', 3, unsigned long)
#define GEC6818_KEY5_STAT		_IOR('K', 4, unsigned long)
#define GEC6818_KALL_STAT		_IOR('K', 5, unsigned long)


int main(int argc, char **argv)
{
	int fd_led=-1,fd_key,ret=-1, len=0, i=0;
	
	fd_led=open("/dev/LEDs", O_WRONLY);
	fd_key=open("/dev/KEYs", O_RDONLY);
	if(fd_led<0 || fd_key<0)
		perror("open LEDs/KEYs error\n");


	

}

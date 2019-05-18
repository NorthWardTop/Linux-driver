#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>

#define GEC6818_ADC_IN0		_IOR('A',  1, unsigned long)
#define GEC6818_ADC_IN1		_IOR('A',  2, unsigned long)

int main(int argc, char **argv)
{
	int fd=-1;

	int rt;

	int i=0;
	
	unsigned long adc_vol = 0;
	
	//打开adc设备
	fd = open("/dev/ADC",O_RDWR);
	
	if(fd < 0)
	{
		perror("open /dev/adc_drv:");
		
		return fd;
		
	}
	
	
	while(1)
	{
		//读取ADC通道0的电压值
		rt=ioctl(fd,GEC6818_ADC_IN0,&adc_vol);
		
		if(rt != 0)
		{
			printf("adc in0 read filed\r\n");
			
			usleep(50*1000);
			
			continue;
		}
		
		printf("adc in0 vol: %lu=mv\r\n",adc_vol);

		sleep(1);
	}
	
	close(fd);
	
	
	return 0;
}
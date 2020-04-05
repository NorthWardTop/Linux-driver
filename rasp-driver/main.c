#include <stdio.h>
#include <stdlib.h>


int main(int argc, char const *argv[])
{
	char *p = (char*)malloc(128);

	for (int i = 0; i < 128; ++i)
		printf("%p", &p[i]);

	return 0;
}



kobject --------------------
/     \                    \
/       \                     \
device     cdev                   driver
/     \ (设备驱动操作方法)           \
/       \                              \
miscdevice         platform_device       platform_driver
(设备驱动操作方法)    (设备的资源)          (设备驱动)


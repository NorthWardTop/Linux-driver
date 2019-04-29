#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char** argv)
{
	char buf[64]="I am application layer data!";
	char from_kern[64]="";
	int fd=open("/dev/LED", O_RDWR);
	if(fd<0)
	{
		perror("open LED error\n");
		return fd;
	}
	sleep(3);
	int len=write(fd, buf, sizeof(buf));
	printf("write len=%d\n", len);
	sleep(2);
	len=read(fd, from_kern, 64);
	printf("kernel say: %s\n", from_kern);

	sleep(3);
	close(fd);
	return 0;
}
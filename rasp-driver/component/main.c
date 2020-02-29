
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <string.h>
#include <strings.h>
#include <sys/epoll.h>
#include <errno.h>
#include <signal.h>


#define MEM_SIZE        0x1000

static volatile unsigned int g_sigioflg;
static volatile unsigned int g_sigusr1flg;

static void sigio_handler(int signum)
{
	fprintf(stdout, "receive a signal from lkm: %d\n", signum);
	g_sigioflg=9527;
}

static void sigusr1_handler(int signum)
{
	fprintf(stdout, "receive a signal from lkm: %d\n", signum);
	fprintf(stdout, "can't write\n");
	g_sigusr1flg = 955;
}

static void sigusr2_handler(int signum)
{
	g_sigusr1flg = 0;
}

int write_test(int fd, char *buf, int len)
{
	int ret;
	if (955 != g_sigusr1flg) {
		ret = write(fd, buf, 1024);
		fprintf(stdout, "write from lkm %d bytes: %s\n", ret, buf);
	} else {
		fprintf(stdout, "write error\n");
	}
}


/**
 * Linux实现底半部的机制主要有tasklet、工作队列、软中断和线程化irq。
 */
int main(int argc , char *argv[])
{
	int fd;
	int ret;
	int flags;
	char buf[1024];

	//非阻塞打开
	fd = open("/dev/lkm_memory", O_RDWR | O_NONBLOCK);
	if (fd < 0) {
		fprintf(stderr, "open device failed: %s\n", strerror(errno));
		return -errno;
	}
	signal(SIGIO, sigio_handler);
	signal(SIGUSR1, sigusr1_handler);
	signal(SIGUSR2, sigusr2_handler);
	fcntl(fd, F_SETOWN, getpid());
	flags = fcntl(fd, F_GETFL);
	fcntl(fd, F_SETFL, flags | FASYNC);

	while (1)
	{
		if (9527 == g_sigioflg) {
			g_sigioflg = 0;
			ret = read(fd, buf, 100);
			fprintf(stdout, "read from lkm %d bytes: %s\n", ret, buf);
			memset(buf, 'b', 1024);
			write_test(fd, buf, 1024);
			write_test(fd, buf, 1024);
		}
		sleep(100);
	}


	return 0;
}
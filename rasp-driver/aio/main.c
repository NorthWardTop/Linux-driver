#include <aio.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <errno.h>
#include <string.h>
#include <strings.h>



int main(int argc, char const *argv[])
{
	int fd, ret;
	struct aiocb cb;
	struct aiocb *cblist[MAX_INPUT]; //cb指针数组
	
	//生成3M文件大小
	system("echo > text.txt");
	system("dd if=/dev/zero of=./text.txt bs=1M count=3");

	fd = open("text.txt", O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "open error： %s\n", strerror(errno));
		return -1;
	}

	bzero(&cb, sizeof(struct aiocb));
	bzero((char*)cblist, sizeof(cblist));
	cb.aio_buf = malloc(BUFSIZ);
	if (!cb.aio_buf) 
		perror("malloc");
	cb.aio_fildes = fd;
	cb.aio_nbytes = BUFSIZ;
	cb.aio_offset = 0;

	cblist[0] = &cb;

	ret = aio_read(&cb);
	if (ret < 0)
		perror("aio_read");
	
	//while (aio_error(&cb) == EINPROGRESS);
	aio_suspend(cblist, MAX_INPUT, NULL);

	ret = aio_return(&cb);
	if (ret > 0)
		fprintf(stdout, "read %d Bytes\n", ret);
	else
		perror("read error");


	return 0;
}




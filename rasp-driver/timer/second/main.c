#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include <sys/stat.h>


int main(int argc, char const *argv[])
{
	int fd;
	int counter = 0, old_counter = 0;

	fd = open("/dev/second", O_RDONLY);
	if (fd < 0) {
		perror("open second failed");
		return -1;
	}

	while (1)
	{
		read(fd, &counter, sizeof(unsigned int));
		if (counter != old_counter) {
			printf("counter =%d\n", counter);
			old_counter = counter;
		}
	}

	close(fd);

	return 0;
}

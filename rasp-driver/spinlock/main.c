#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>


int main(int argc, char const *argv[])
{
    int devfd = open("/dev/lkm_memory", O_RDWR);
    if (devfd < 0) {
        fprintf(stderr, "open /dev/lkm_memory failed: %s\n", strerror(errno));
        return -1;
    } else {
        fprintf(stdout, "open /dev/lkm_memory successful!");
    }

    
    close(devfd);
    
    return 0;
}

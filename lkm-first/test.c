
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>



int main(int argc, char const *argv[])
{

    int devfd = open("/dev/lkm_memory", O_RDWR);
    
    
    return 0;
}

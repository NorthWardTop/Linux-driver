
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
#include <linux/signal.h>
#include <errno.h>

#define MEM_SIZE        0x1000
#define FIFO_CLEAR      0x1

int main(int argc , char *argv[])
{
    int fd;
    char aRdch[MEM_SIZE];
    int max_fd;
    struct epoll_event event;
    int epfd;
    int ret;

    //非阻塞打开
    fd = open("/dev/lkm_memory", O_RDWR | O_NONBLOCK);
    if (fd < 0) {
        fprintf(stderr, "open device failed: %s\n", strerror(errno));
        return -errno;
    }

    epfd = epoll_create(5);
    if (epfd < 0) {
        fprintf(stderr, "epoll create failed: %s\n", strerror(errno));
        return -errno;
    }
    //设置可以触发的事件
    bzero(&event, sizeof(struct epoll_event));
    event.events = EPOLL_CTL_ADD | EPOLLPRI;
    //当前句柄加入epoll监控句柄链表
    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event);
    if (ret < 0) {
        fprintf(stderr, "epoll_ctl EPOLL_CTL_ADD: %s\n", strerror(errno));
        return -errno;
    }

    //调动epoll_wait进行epfd句柄链表的监控
    ret = epoll_wait(epfd, &event, 1, 15000);
    if (ret < 0) {
        fprintf(stderr, "epoll_wait failed %s\n", strerror(errno));
        return -errno;
    } else if (ret == 0) {
        fprintf(stdout, "epoll_wait time-out within 15 seconds\n");
    } else {
        read(fd, aRdch, 100);
        fprintf(stdout, "%s\n", aRdch);
    }
    
    //删除fd从epfd链表上
    ret = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &event);
    if (ret < 0) {
        fprintf(stderr, "epoll_ctl EPOLL_CTL_DEL: %s\n", strerror(errno));
        return -errno;
    }



    return 0;
}
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <poll.h>
#include <sys/ioctl.h>

#include "sysmon.h"

int main(void)
{
    int fd;
    int interval = 5000;
    int status;
    struct pollfd pfd;

    fd = open("/dev/sysmon", O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Failed to open /dev/sysmon: %s\n",
                strerror(errno));
        return 1;
    }

    status = ioctl(fd, SYSMON_SET_INTERVAL, &interval);
    if (status < 0) {
        fprintf(stderr, "ioctl failed: %s\n", strerror(errno));
        close(fd);
        return 1;
    }

    pfd.fd = fd;
    pfd.events = POLLIN;
    
    status = poll(&pfd, 1, -1);

    if (status < 0) {
        fprintf(stderr, "poll failed: %s\n", strerror(errno));
        close(fd);
        return 1;
    }

    if (pfd.revents & POLLIN)
        printf("Data is available (POLLIN)\n");

    close(fd);

    return 0;
}


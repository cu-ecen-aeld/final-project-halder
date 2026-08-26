#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>

#include "sysmon.h"

int main(void)
{
    int fd, status;
    int intervals[] = {100, 1000, 2000};

    fd = open("/dev/sysmon", O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Failed to open /dev/sysmon: %s\n", strerror(errno));
        return 1;
    }
    
    for (int i = 0; i < 3; i++) {
        status = ioctl(fd, SYSMON_SET_INTERVAL, &intervals[i]);

        if (status < 0) {
            fprintf(stderr, "ioctl failed for %d ms: %s\n", intervals[i], strerror(errno));
            break;
        }
    }

    printf("ioctl test successful!\n");

    close(fd);

    return 0;
}

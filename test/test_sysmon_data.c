#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "sysmon.h"

int main(void)
{
    int fd;
    struct sysmon_data data;

    fd = open("/dev/sysmon", O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Failed to open /dev/sysmon: %s\n", strerror(errno));
        return 1;
    }

    ssize_t bytes = read(fd, &data, sizeof(data));

    if (bytes < 0) {
        fprintf(stderr, "Failed to read /dev/sysmon: %s\n", strerror(errno));
        close(fd);
        return 1;
    }

    printf("Read %zd bytes\n", bytes);
    printf("Uptime:          %lu\n", data.uptime_seconds);
    printf("Free memory:     %lu\n", data.free_memory_bytes);
    printf("Total memory:    %lu\n", data.total_memory_bytes);
    printf("Hostname:        %s\n", data.hostname);
    printf("CPU frequency:   %ld\n", data.cpu_frequency_khz);
    printf("CPU temperature: %d\n", data.cpu_temperature_millicelsius);

    close(fd);

    return 0;
}

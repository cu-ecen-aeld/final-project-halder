#ifndef SYSMON_H_
#define SYSMON_H_

#ifdef __KERNEL__
#include <linux/cdev.h>
#include <linux/types.h>
#else
#include <stdint.h>
#endif

#define SYSMON_HOSTNAME_LEN 64

struct sysmon_data
{
    uint64_t    uptime_seconds;
    uint64_t    free_memory_bytes;
    uint64_t    total_memory_bytes;
    int32_t     cpu_temperature_millicelsius;
    uint64_t    cpu_frequency_khz;
    char        hostname[SYSMON_HOSTNAME_LEN];
};

#ifdef __KERNEL__

struct sysmon_device
{
    struct cdev         cdev;
    struct sysmon_data  data;
};

#endif /* __KERNEL__ */

#endif /* SYSMON_H_ */

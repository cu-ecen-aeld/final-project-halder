#ifndef SYSMON_H_
#define SYSMON_H_

#ifdef __KERNEL__
#include <linux/cdev.h>
#include <linux/types.h>
#else
#include <stdint.h>
#endif

struct sysmon_data
{
    uint64_t    uptime_seconds;
    uint64_t    free_memory_bytes;
    int32_t     cpu_temperature_millicelsius;
};

#ifdef __KERNEL__

struct sysmon_device
{
    struct cdev         cdev;
    struct sysmon_data  data;
};

#endif /* __KERNEL__ */

#endif /* SYSMON_H_ */

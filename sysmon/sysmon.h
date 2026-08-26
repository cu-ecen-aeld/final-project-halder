#ifndef SYSMON_H_
#define SYSMON_H_

#ifdef __KERNEL__
#include <linux/cdev.h>
#include <linux/types.h>
#include <asm-generic/ioctl.h>
#include <linux/wait.h>
#include <linux/workqueue.h>
#else
#include <stdint.h>
#include <sys/ioctl.h>
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


// Pick an arbitrary unused value from https://github.com/torvalds/linux/blob/master/Documentation/userspace-api/ioctl/ioctl-number.rst
#define SYSMON_IOC_MAGIC 0x16
#define SYSMON_SET_INTERVAL _IOW(SYSMON_IOC_MAGIC, 1, int)


#ifdef __KERNEL__

struct sysmon_device
{
    struct cdev         cdev;
    struct sysmon_data  data;
    int                 update_interval_ms;
    bool                data_available;
    wait_queue_head_t   wait_queue;
    struct delayed_work update_work;
};

#endif /* __KERNEL__ */

#endif /* SYSMON_H_ */

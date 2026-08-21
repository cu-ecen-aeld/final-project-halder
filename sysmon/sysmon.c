#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#include <linux/ktime.h>    // Uptime
#include <linux/mm.h>       // Memory usage
#include <linux/utsname.h>  // Hostname
#include <linux/thermal.h>  // CPU temp
#include <linux/cpufreq.h>  // CPU frequency

#include "sysmon.h"

#define SYSMON_MINORS 1

MODULE_LICENSE("GPL");
MODULE_AUTHOR("halder");
MODULE_DESCRIPTION("CU Boulder ECEA 5307 Final Project - System Monitoring Kernel Module");

static dev_t dev_nr;
static struct class *sysmon_class;
static struct sysmon_device sysmon_dev;

static void sysmon_get_uptime(struct sysmon_data *data)
{
    data->uptime_seconds = ktime_get_boottime_seconds();
}

static void sysmon_get_memory(struct sysmon_data *data)
{
    struct sysinfo info;

    si_meminfo(&info);
    data->free_memory_bytes = info.freeram * info.mem_unit;
    data->total_memory_bytes = info.totalram * info.mem_unit;
}

static void sysmon_get_hostname(struct sysmon_data *data)
{
    strscpy(data->hostname, utsname()->nodename, sizeof(data->hostname)); 
}

static void sysmon_get_cpu_frequency(struct sysmon_data *data)
{
    data->cpu_frequency_khz = cpufreq_get(0);
}

static int sysmon_get_cpu_temperature(struct sysmon_data *data)
{
    struct thermal_zone_device *tz;
    int temp, status;

    tz = thermal_zone_get_zone_by_name("cpu-thermal");
    if (IS_ERR(tz))
        return PTR_ERR(tz);
    
    status = thermal_zone_get_temp(tz, &temp);
    if (status < 0)
        return status;

    data->cpu_temperature_millicelsius = temp;
    return 0;
}

static int sysmon_collect_data(struct sysmon_data *data)
{
    int status;

    sysmon_get_uptime(data);
    sysmon_get_memory(data);
    sysmon_get_hostname(data);
    sysmon_get_cpu_frequency(data);
    status = sysmon_get_cpu_temperature(data);

    if (status)
        return status;

    return 0;
}

static int sysmon_open(struct inode *inode, struct file *filp)
{
    struct sysmon_device *dev;
    
    dev = container_of(inode->i_cdev, struct sysmon_device, cdev);
    filp->private_data = dev;

    printk(KERN_DEBUG "sysmon - Opened sysmon device.\n");
    
    return 0;
}

static int sysmon_release(struct inode *inode, struct file *filp)
{
    printk(KERN_DEBUG "sysmon - Closed sysmon device.\n");
    return 0;
}

static ssize_t sysmon_read(struct file *filp, char __user *buffer, size_t count, loff_t *offset)
{
    struct sysmon_device *dev = filp->private_data;

    if (*offset != 0)
        return 0;

    if (count < sizeof(dev->data))
        return -EINVAL;
    
    sysmon_collect_data(&dev->data);
    
    if (copy_to_user(buffer, &dev->data, sizeof(sysmon_dev.data)))
        return -EFAULT;

    *offset += sizeof(dev->data);

    printk(KERN_DEBUG "sysmon - Copied %zu bytes to user\n", sizeof(dev->data));
    
    return sizeof(dev->data);
}

static const struct file_operations fops = {
    .owner   = THIS_MODULE,
    .open    = sysmon_open,
    .release = sysmon_release,
    .read    = sysmon_read
};

static int __init sysmon_init(void)
{
    int status;

    status = alloc_chrdev_region(&dev_nr, 0, SYSMON_MINORS, "sysmon_device");
    if (status) {
        printk(KERN_ERR "sysmon - Error registering cdev, could not register region of dev numbers\n");
        return status;
    }
    
    cdev_init(&sysmon_dev.cdev, &fops);
    sysmon_dev.cdev.owner = THIS_MODULE;

    status = cdev_add(&sysmon_dev.cdev, dev_nr, SYSMON_MINORS);
    if (status) {
        printk(KERN_ERR "sysmon - Error adding cdev within sysmon_device\n");
        goto free_dev_nr;
    }

    printk(KERN_INFO "sysmon - Registered cdev. Major device number %d, starting with Minor %d\n", MAJOR(dev_nr), MINOR(dev_nr));
    
    sysmon_class = class_create("sysmon_class");
    if (!sysmon_class) {
        printk(KERN_ERR "sysmon - Could not create class 'sysmon_class'\n");
        status = ENOMEM;
        goto delete_cdev;
    }

    if (!device_create(sysmon_class, NULL, dev_nr, NULL, "sysmon")) {
        printk(KERN_ERR "sysmon - Could not create device 'sysmon'\n");
        status = ENOMEM;
        goto delete_class;
    }

    printk(KERN_INFO "sysmon - Created device under /sys/class/sysmon_class/sysmon\n");
    printk(KERN_DEBUG "sysmon - Kernel module sysmon.ko loaded.\n");

    return 0;
        
delete_cdev:
    class_unregister(sysmon_class);
    class_destroy(sysmon_class);
delete_class:
    cdev_del(&sysmon_dev.cdev);
free_dev_nr:
    unregister_chrdev_region(dev_nr, SYSMON_MINORS);
    return status;
}

static void __exit sysmon_exit(void)
{
    device_destroy(sysmon_class, dev_nr);
    class_unregister(sysmon_class);
    class_destroy(sysmon_class);
    cdev_del(&sysmon_dev.cdev);
    printk(KERN_INFO "sysmon - Chardev deleted\n");
    unregister_chrdev_region(dev_nr, SYSMON_MINORS);
    printk(KERN_INFO "sysmon - Kernel module unloaded.\n");
}


module_init(sysmon_init);
module_exit(sysmon_exit);

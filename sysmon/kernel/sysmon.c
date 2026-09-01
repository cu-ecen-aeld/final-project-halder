#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/poll.h>

#include <linux/ktime.h>    // Uptime
#include <linux/mm.h>       // Memory usage
#include <linux/utsname.h>  // Hostname
#include <linux/thermal.h>  // CPU temp
#include <linux/cpufreq.h>  // CPU frequency

#include "sysmon.h"

#define SYSMON_MINORS 1
#define SYSMON_INTERVAL_DEFAULT 10000 /* 10 seconds */


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
    
    /* What `container_of` basically means/does:
     *
     * "Given this pointer to a member, calculate the address of the struct that contains it."
     * Here `inode->i_cdev` is a pointer to the `cdev` member.
     */
    dev = container_of(inode->i_cdev, struct sysmon_device, cdev);

    mutex_lock(&dev->open_lock);
    if (dev->is_opened) {
        mutex_unlock(&dev->open_lock);
        printk(KERN_ERR "sysmon.open - Sysmon device is already open in another process.\n");
        return -EBUSY;
    }

    dev->is_opened = true;
    mutex_unlock(&dev->open_lock);

    filp->private_data = dev;

    printk(KERN_DEBUG "sysmon.open - Opened sysmon device.\n");
    
    return 0;
}

static int sysmon_release(struct inode *inode, struct file *filp)
{
    struct sysmon_device *dev = filp->private_data;

    mutex_lock(&dev->open_lock);
    dev->is_opened = false;
    mutex_unlock(&dev->open_lock);

    printk(KERN_DEBUG "sysmon.release - Closed sysmon device.\n");

    return 0;
}

static ssize_t sysmon_read(struct file *filp, char __user *buffer, size_t count, loff_t *offset)
{
    struct sysmon_device *dev = filp->private_data;

    if (count < sizeof(dev->data))
        return -EINVAL;
   
    for (;;) {
        mutex_lock(&dev->data_lock);

        if (dev->data_available)
            break;

        mutex_unlock(&dev->data_lock);

        if (filp->f_flags & O_NONBLOCK) {
            printk(KERN_ERR "sysmon.read - File is opened in non-blocking mode and no data is available\n");
            return -EAGAIN;
        }
        
        /* Userpace process which called `read` sleeps until data_available == true
         * (vs. poll_wait which does not sleep)
         */
        if (wait_event_interruptible(dev->wait_queue, READ_ONCE(dev->data_available)))
            return -ERESTARTSYS;
    }

    if (copy_to_user(buffer, &dev->data, sizeof(dev->data))) {
        mutex_unlock(&dev->data_lock);
        return -EFAULT;
    }

    printk(KERN_DEBUG "sysmon.read - Copied %zu bytes to user\n", sizeof(dev->data));
    
    dev->data_available = false;
    
    mutex_unlock(&dev->data_lock);

    return sizeof(dev->data);
}

static long int sysmon_ioctl(struct file *filp, unsigned int cmd, unsigned long args)
{
    int interval;

    printk(KERN_DEBUG "sysmon.ioctl - ioctl called with cmd: 0x%x and arg: %ld\n", cmd, args);
    
    struct sysmon_device *dev = filp->private_data;

    switch (cmd) {
        case SYSMON_SET_INTERVAL:
            if (copy_from_user(&interval, (int __user *) args, sizeof(interval)))
                return -EFAULT;

            cancel_delayed_work_sync(&dev->update_work);

            mutex_lock(&dev->data_lock);

            dev->update_interval_ms = interval;
            schedule_delayed_work(&dev->update_work, msecs_to_jiffies(dev->update_interval_ms));

            mutex_unlock(&dev->data_lock);

            printk(KERN_INFO "sysmon.ioctl - Update interval is set to %d\n", sysmon_dev.update_interval_ms);
            break;

        default:
            return -EOPNOTSUPP;
    }

    return 0;
}

static __poll_t sysmon_poll(struct file *filp, poll_table *wait)
{
    struct sysmon_device *dev = filp->private_data;
    __poll_t mask = 0;

    poll_wait(filp, &dev->wait_queue, wait);

    mutex_lock(&dev->data_lock);

    if (dev->data_available)
        mask |= EPOLLIN | EPOLLRDNORM;
    
    mutex_unlock(&dev->data_lock);

    printk(KERN_DEBUG "sysmon.poll - Poll finished waiting\n");

    return mask;
}

static void sysmon_work(struct work_struct *work)
{
    int status;

    struct sysmon_device *dev = container_of(work, struct sysmon_device, update_work.work);

    mutex_lock(&dev->data_lock);

    status = sysmon_collect_data(&dev->data);

    if (status == 0)
        dev->data_available = true;
  
    mutex_unlock(&dev->data_lock);

    if (status == 0) {
        printk(KERN_DEBUG "sysmon.work - Data is collected and ready for transfer to user space\n");
        wake_up_interruptible(&dev->wait_queue);
    }
    
    mutex_lock(&dev->data_lock);

    schedule_delayed_work(&dev->update_work, msecs_to_jiffies(dev->update_interval_ms));
    
    mutex_unlock(&dev->data_lock);
}

static const struct file_operations fops = {
    .owner          = THIS_MODULE,
    .open           = sysmon_open,
    .release        = sysmon_release,
    .read           = sysmon_read,
    .unlocked_ioctl = sysmon_ioctl,
    .poll           = sysmon_poll
};

static int __init sysmon_init(void)
{
    int status;

    sysmon_dev.update_interval_ms = SYSMON_INTERVAL_DEFAULT;
    sysmon_dev.data_available = false;

    mutex_init(&sysmon_dev.data_lock);
    mutex_init(&sysmon_dev.open_lock);
    sysmon_dev.is_opened = false;

    init_waitqueue_head(&sysmon_dev.wait_queue);
    INIT_DELAYED_WORK(&sysmon_dev.update_work, sysmon_work);

    status = alloc_chrdev_region(&dev_nr, 0, SYSMON_MINORS, "sysmon_device");
    if (status) {
        printk(KERN_ERR "sysmon.init - Error registering cdev, could not register region of dev numbers: %d\n", status);
        return status;
    }
    
    cdev_init(&sysmon_dev.cdev, &fops);
    sysmon_dev.cdev.owner = THIS_MODULE;

    status = cdev_add(&sysmon_dev.cdev, dev_nr, SYSMON_MINORS);
    if (status) {
        printk(KERN_ERR "sysmon.init - Error adding cdev within sysmon_device: %d\n", status);
        goto free_dev_nr;
    }

    printk(KERN_INFO "sysmon.init - Registered cdev. Major device number %d, starting with Minor %d\n", MAJOR(dev_nr), MINOR(dev_nr));
    
    sysmon_class = class_create("sysmon_class");
    if (!sysmon_class) {
        printk(KERN_ERR "sysmon.init - Could not create class 'sysmon_class'\n");
        status = ENOMEM;
        goto delete_cdev;
    }

    if (!device_create(sysmon_class, NULL, dev_nr, NULL, "sysmon")) {
        printk(KERN_ERR "sysmon.init - Could not create device 'sysmon'\n");
        status = ENOMEM;
        goto delete_class;
    }

    printk(KERN_INFO "sysmon.init - Created device under /sys/class/sysmon_class/sysmon\n");
    printk(KERN_DEBUG "sysmon.init - Kernel module sysmon.ko loaded.\n");

    schedule_delayed_work(&sysmon_dev.update_work, msecs_to_jiffies(sysmon_dev.update_interval_ms));

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
    cancel_delayed_work_sync(&sysmon_dev.update_work);
    device_destroy(sysmon_class, dev_nr);
    class_unregister(sysmon_class);
    class_destroy(sysmon_class);
    cdev_del(&sysmon_dev.cdev);
    printk(KERN_INFO "sysmon.exit - Chardev deleted\n");
    unregister_chrdev_region(dev_nr, SYSMON_MINORS);
    printk(KERN_INFO "sysmon.exit - Kernel module unloaded.\n");
}


module_init(sysmon_init);
module_exit(sysmon_exit);

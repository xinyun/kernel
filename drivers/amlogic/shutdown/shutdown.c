/*
 * author:  Yao.Zhang@amlogic.com
 * date:	  2012-10-17
 * usage: 	show earlysuspend info
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <mach/watchdog.h>
#include <mach/am_regs.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
static struct early_suspend hw_early_suspend;
#endif
#define AML_WATCH_DOG_START() do { \
        writel(0, (void *)P_WATCHDOG_RESET); \
        writel((10|((1<<WATCHDOG_ENABLE_BIT)|(0xF<<24))), (void *)P_WATCHDOG_TC); \
        while(1); \
    } while(0);

int need_reboot_flag = 0;//add for long press powerkey reboot,and wakeup enter recovery

static ssize_t show_need_reboot(struct class *class, struct class_attribute *attr, char *buf)
{
    int pos = 0;
    pos += sprintf(buf + pos, "%d\n", need_reboot_flag);
    return pos;
}

static ssize_t store_need_reboot(struct class *class, struct class_attribute *attr, const char *buf, size_t count)
{
    need_reboot_flag = (int)simple_strtoul(buf, NULL, 0);
    printk("need_reboot_flag is %d", need_reboot_flag);
    return count;
}

static struct class_attribute shutdown_class_attrs[] = {
    __ATTR(need_reboot, S_IRUGO | S_IWUGO, show_need_reboot, store_need_reboot),
    __ATTR_NULL
};

static struct class shutdown_class = {
    .name = "shutdown",
    .class_attrs = shutdown_class_attrs,
};

static void hw_suspend(struct early_suspend *h)
{
    printk("====hw_suspend====\n");
}

static void hw_resume(struct early_suspend *h)
{
    printk("=====hw_resume =====\n");
    if(need_reboot_flag){
        AML_WATCH_DOG_START();
    }
}

static int __init shutdown_init(void)
{
    printk(KERN_INFO "shutdown Driver init.\n");
    if(class_register(&shutdown_class)){
        printk(" class register shutdown_class fail!\n");
    }
#ifdef CONFIG_HAS_EARLYSUSPEND
    hw_early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB;
    hw_early_suspend.suspend = hw_suspend;
    hw_early_suspend.resume = hw_resume;
    register_early_suspend(&hw_early_suspend);
#endif
    return 0;
}

static void __exit shutdown_exit(void)
{
    printk(KERN_INFO "shutdown Driver exit.\n");
    class_unregister(&shutdown_class);
#ifdef CONFIG_HAS_EARLYSUSPEND
    unregister_early_suspend(&hw_early_suspend);
#endif
}

module_init(shutdown_init);
module_exit(shutdown_exit);

MODULE_AUTHOR("Yao.Zhang@amlogic.com");
MODULE_DESCRIPTION("shutdown info");
MODULE_LICENSE("GPL");
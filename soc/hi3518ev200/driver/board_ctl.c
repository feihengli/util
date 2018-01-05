#include <linux/init.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/module.h>
#include "board_ctl.h"

#define ADC_CH0_MAJOR                   95
#define MAX_ADC_MINORS                  1
#define GPIO_0_BASE     0x20140000

#define HW_REG(reg)     *((volatile unsigned int *)(reg))
#define BCTL_PRINTK(fmt, args...) printk("[%s %d]"fmt"\n", __FUNCTION__, __LINE__, ##args)

typedef struct
{
    unsigned int group;
    unsigned int channel;
    unsigned int  mux_reg;
    unsigned int  mux_value;
}gpio_ctl_s;


static gpio_ctl_s g_ctl_tbl[] =
{
    {
        1,                  //group
        1,                  //channel
        0x200f0080,      //mux_reg
        0                //mux_value
    }
};

int gpio_read(int group, int channel)
{
    unsigned int base_addr;
    unsigned int dir_addr;
    unsigned int mask;
    unsigned int data_addr;
    unsigned int regValue;

    base_addr = GPIO_0_BASE + 0x10000 * group;
    dir_addr = IO_ADDRESS(base_addr + 0x400);
    mask = 1 << channel;
    data_addr = IO_ADDRESS(base_addr + (1 << (channel + 2)));

    regValue = HW_REG(dir_addr);
    regValue &= (~mask);
    HW_REG(dir_addr) = regValue;
    regValue = HW_REG(data_addr);
    return (regValue & mask) >> channel;
}

int gpio_write(int group, int channel, unsigned int value)
{
    unsigned int base_addr;
    unsigned int dir_addr;
    unsigned int data_addr;

    base_addr = GPIO_0_BASE + 0x10000 * group;
    dir_addr = IO_ADDRESS(base_addr + 0x400);
    data_addr = IO_ADDRESS(base_addr + (1 << (channel + 2)));

    HW_REG(dir_addr) |= 1 << channel;
    HW_REG(data_addr) = (value << channel);

    return 0;
}

int get_gpio_value(int ctl_idx)
{
    gpio_ctl_s *ctl = &g_ctl_tbl[ctl_idx];
    unsigned int addr = IO_ADDRESS(ctl->mux_reg);
    if (HW_REG(addr) != ctl->mux_value)
    {
        BCTL_PRINTK("pin %d is not gpio mode, set first.", ctl_idx);
        HW_REG(addr) = ctl->mux_value;
        msleep(200);
    }

    return gpio_read(ctl->group, ctl->channel);
}

int set_gpio_value(int ctl_idx, int value)
{
    gpio_ctl_s *ctl = &g_ctl_tbl[ctl_idx];
    unsigned int addr = IO_ADDRESS(ctl->mux_reg);
    if (HW_REG(addr) != ctl->mux_value)
    {
        BCTL_PRINTK("pin %d is not gpio mode, set first.", ctl_idx);
        HW_REG(addr) = ctl->mux_value;
        msleep(200);
    }

    return gpio_write(ctl->group, ctl->channel, value);
}

static int board_index_get(int group, int channel)
{
    int num = sizeof(g_ctl_tbl)/sizeof(g_ctl_tbl[0]);
    int i = 0;

    for (i = 0; i < num; i++)
    {
        if (g_ctl_tbl[i].group == group && g_ctl_tbl[i].channel == channel)
        {
            return i;
        }
    }
    return -1;
}

static int board_aw8733a(unsigned long arg)
{
    unsigned int i = 0;
    int idx = board_index_get(1, 1); //GPIO1_1 to control
    if (idx >= 0)
    {
        set_gpio_value(idx, 0);
        //BCTL_PRINTK("low...\n");
        udelay(800);

        for (i = 0; i < arg; i++)
        {
            set_gpio_value(idx, 0);
            //BCTL_PRINTK("low...\n");
            udelay(2);
            set_gpio_value(idx, 1);
            //BCTL_PRINTK("high...\n");
            udelay(2);
        }
    }

    return 0;
}

static long board_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    switch (cmd)
    {
        case DRV_CMD_AW8733A:
        {
            board_aw8733a(arg);
            break;
        }
        default:
        {
            BCTL_PRINTK("Unknown cmd %d\n", cmd);
            break;
        }
    }

    return 0;
}

static int board_ctl_open(struct inode *inode, struct file *filp)
{
    int minor = MINOR(inode->i_rdev);

    if (minor >= MAX_ADC_MINORS)
        return -ENODEV;

    filp->private_data = (void *)minor;

    return 0;
}

static int board_ctl_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static struct file_operations board_ctl_fops = {
        .unlocked_ioctl = board_ioctl,
        .open           = board_ctl_open,
        .release        = board_ctl_release,
        .owner          = THIS_MODULE,
};

static struct cdev board_ctl_cdev = {
        .kobj   =   {.name = "ADC_CH0", },
        .owner  =   THIS_MODULE,
};

static struct class * sys_class = NULL;
static dev_t boardctl_devno;

static int __init board_ctl_init(void)
{
    BCTL_PRINTK("board_ctl init start.\n");

    boardctl_devno = MKDEV(ADC_CH0_MAJOR, 0);

    if(register_chrdev_region(boardctl_devno, MAX_ADC_MINORS, "ADC_CH0"))
        goto error;

    cdev_init(&board_ctl_cdev, &board_ctl_fops);
    if (cdev_add(&board_ctl_cdev, boardctl_devno, MAX_ADC_MINORS))
    {
        kobject_put(&board_ctl_cdev.kobj);
        unregister_chrdev_region(boardctl_devno, MAX_ADC_MINORS);
        goto error;
    }

#define DEV_NAME "adc"
    sys_class = class_create(THIS_MODULE, DEV_NAME);
    device_create(sys_class, NULL, boardctl_devno, NULL, DEV_NAME);

    BCTL_PRINTK("insert board_ctl.ko\n");

error:
    return 0;
}

static void board_ctl_exit(void)
{
    device_destroy(sys_class, boardctl_devno);
    class_destroy(sys_class);

    cdev_del(&board_ctl_cdev);
    unregister_chrdev_region(boardctl_devno, MAX_ADC_MINORS);
    BCTL_PRINTK("remove board_ctl.ko\n");

    return;
}

module_init(board_ctl_init);
module_exit(board_ctl_exit);

MODULE_LICENSE("GPL");


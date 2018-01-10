#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <assert.h>

#include "sal_gpio.h"
#include "sal_himm.h"
#include "sal_debug.h"

#define MUXCTRL_BASE_REG (0x200f0000)             //管脚复用寄存器基地址
#define MUXCTRL_OFFSET_GPIO1_6 (0x94)             //button
#define MUXCTRL_OFFSET_GPIO1_0 (0x7c)             //LED red
#define MUXCTRL_OFFSET_GPIO1_4 (0x8c)             //LED green
#define MUXCTRL_OFFSET_GPIO1_5 (0x90)             //LED blue
#define MUXCTRL_OFFSET_GPIO8_0 (0x100)             //IRCUT
#define MUXCTRL_OFFSET_GPIO1_2 (0x84)             //infrared light




#define GPIO0_BASE (0x20140000)
#define GPIO_DIR_OFFSET (0x400)

typedef struct gpio_ctrl_s
{
    unsigned int  mux_reg_offset;
    unsigned int  mux_value;
}gpio_ctrl_s;

static gpio_ctrl_s gpio_ctrl[] =
{
    {MUXCTRL_OFFSET_GPIO1_6, 0},
    {MUXCTRL_OFFSET_GPIO1_0, 0},
    {MUXCTRL_OFFSET_GPIO1_4, 0},
    {MUXCTRL_OFFSET_GPIO1_5, 0},
    {MUXCTRL_OFFSET_GPIO8_0, 1},
    {MUXCTRL_OFFSET_GPIO1_2, 0},
};


int gpio_init()
{
    unsigned int i = 0;
    for (i = 0; i < sizeof(gpio_ctrl)/sizeof(gpio_ctrl[0]); i++)
    {
        unsigned int mux_reg = MUXCTRL_BASE_REG+gpio_ctrl[i].mux_reg_offset;
        unsigned int mux_val = gpio_ctrl[i].mux_value;
        himm_write(mux_reg, mux_val);
        //printf("mux_reg: %#x, mux_val: %#x\n");
    }

    return 0;
}

unsigned int gpio_read(int group, int channel)
{
    unsigned int group_addr = GPIO0_BASE + 0x10000 * group;
    int dir_addr = group_addr+GPIO_DIR_OFFSET;
    unsigned int val = himm_read(dir_addr);
    val &= ~(1 << channel); //set direction
    himm_write(dir_addr, val);

    int offset = (1 << (channel+2));
    int data_addr = group_addr + offset;
    val = himm_read(data_addr);
    //printf("val: %#x\n", val);
    val = (val >> channel) & 0x1;

    return val;
}

unsigned int gpio_write(int group, int channel, int value)
{
    assert(value == 0 || value == 1);

    unsigned int group_addr = GPIO0_BASE + 0x10000 * group;
    int dir_addr = group_addr+GPIO_DIR_OFFSET;
    unsigned int val = himm_read(dir_addr);
    val |= (1 << channel); //set direction
    himm_write(dir_addr, val);

    int offset = (1 << (channel+2));
    int data_addr = group_addr + offset;
    unsigned int new_value = (value << channel);
    //printf("new_value: %#x\n", new_value);
    himm_write(data_addr, new_value);

    return 0;
}






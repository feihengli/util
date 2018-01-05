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

#include "sal_lsadc.h"
#include "sal_himm.h"
#include "sal_debug.h"

#define MUXCTRL_BASE_REG (0x200f0000)            //ch0 管脚复用寄存器
#define MUXCTRL_ADC_CH0 (MUXCTRL_BASE_REG+0xf8)  //SAR_ADC_CH0/GPIO7_6 管脚复用控制寄存器

#define LSADC_CRTL_BASE_ADDR (0x200b0000)
#define LSADC_OFFSET_CRTL0 (LSADC_CRTL_BASE_ADDR+0x0000)
#define LSADC_OFFSET_CRTL1 (LSADC_CRTL_BASE_ADDR+0x0004)
#define LSADC_OFFSET_CRTL2 (LSADC_CRTL_BASE_ADDR+0x0008)
#define LSADC_OFFSET_CRTL3 (LSADC_CRTL_BASE_ADDR+0x000c)
#define LSADC_OFFSET_CRTL4 (LSADC_CRTL_BASE_ADDR+0x0010)
#define LSADC_OFFSET_CRTL5 (LSADC_CRTL_BASE_ADDR+0x0014)
#define LSADC_OFFSET_CRTL6 (LSADC_CRTL_BASE_ADDR+0x0018)
#define LSADC_OFFSET_CRTL7 (LSADC_CRTL_BASE_ADDR+0x001c)
#define LSADC_OFFSET_CRTL8 (LSADC_CRTL_BASE_ADDR+0x0020)

int lsadc_init()
{
    himm_write(MUXCTRL_ADC_CH0, 0);
    unsigned int data = 0xff000000; //8 bit lsadc_active_bit
    data &= ~(0 << 13); // 单次扫描
    data |= (1 << 8); // 通道0有效
    himm_write(LSADC_OFFSET_CRTL0, data); //0xff000100 (606-->fc000100)
    //printf("LSADC_OFFSET_CRTL0: %#x\n", data);

    return 0;
}

int lsadc_get_value()
{
    unsigned int data = 0;
    himm_write(LSADC_OFFSET_CRTL7, 0xffffffff);
    while (1)
    {
        data = himm_read(LSADC_OFFSET_CRTL5);
        int ro_int_flag_ina = data & 0x01;
        if (ro_int_flag_ina == 1)
        {
            //DBG("SAR_ADC_CH0 convert OK\n");
            break;
        }
        usleep(1);
    }

    himm_write(LSADC_OFFSET_CRTL6, 0x1);
    data = himm_read(LSADC_OFFSET_CRTL5);
    assert((data & 0x01) == 0);

    data = himm_read(LSADC_OFFSET_CRTL3);
    data &= 0xff;

    return data;
}





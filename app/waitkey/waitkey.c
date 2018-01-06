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
#include "sal_util.h"
#include "sal_debug.h"

#define MUXCTRL_OFFSET_GPIO1_6 (0x94)             //button

int main(int argc, char** argv)
{
    //int ret = -1;
    unsigned int count = 0;

    struct timeval begin = {0, 0};
    util_time_abs(&begin);
    gpio_init();

    while (1)
    {
        int value = gpio_read(1, 6);
        if (value == 0)
        {
            count++;
        }
        else
        {
            count = 0;
        }

        if (count > 1)
        {
            DBG("key has been press down\n");
            return 0;
        }

        int pass_time = util_time_pass(&begin);
        if (pass_time > 20*1000)
        {
            WRN("wait key on time out[%dms]\n", pass_time);
            return -1;
        }

        usleep(500*1000);
    }


    return -1;
}




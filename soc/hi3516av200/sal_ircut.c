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
#include <errno.h>
#include <sys/prctl.h>

#include "sal_gpio.h"
#include "sal_himm.h"
#include "sal_lsadc.h"
#include "sal_isp.h"
#include "sal_ircut.h"
#include "sal_debug.h"
#include "sal_util.h"

typedef struct sal_ircut_args
{
    SAL_IR_MODE ir_mode;
    int adc_threshold;
    int last_stat; // 最后的状态 0 黑白 1 彩色
    struct timeval last_stat_time;

    int running;
    pthread_t pid;
    pthread_mutex_t mutex;
}sal_ircut_args;

static sal_ircut_args* g_ircut_args = NULL;

static int sal_ircut_switch_mode(int color)
{
    int ret = -1;
    sal_image_attr_s image_attr;
    memset(&image_attr, 0, sizeof(image_attr));
    image_attr.saturation = 128;
    image_attr.contrast = 128;
    image_attr.brightness = 128;
    image_attr.sharpness = 128;

    if (color)
    {
        gpio_write(1, 2, 0); //infrared light
        gpio_write(8, 0, 0); //IRCUT
        usleep(350*1000);

        ret = sal_isp_day_night_switch(SAL_ISP_MODE_DAY, &image_attr);
        CHECK(ret == 0, -1, "Error with %#x.\n", ret);
    }
    else
    {
        image_attr.saturation = 0;

        ret = sal_isp_day_night_switch(SAL_ISP_MODE_NIGHT, &image_attr);
        CHECK(ret == 0, -1, "Error with %#x.\n", ret);

        usleep(350*1000);
        gpio_write(1, 2, 1); //infrared light
        gpio_write(8, 0, 1); //IRCUT
    }

    return 0;
}

static void* sal_ircut_thread(void* arg)
{
    prctl(PR_SET_NAME, __FUNCTION__);

    int ret = -1;
    int stat = 1;

    while (g_ircut_args->running)
    {
        pthread_mutex_lock(&g_ircut_args->mutex);

        if (g_ircut_args->ir_mode == SAL_IR_MODE_AUTO)
        {
            sal_exposure_info_s exposure_info;
            ret = sal_isp_exposure_info_get(&exposure_info);
            CHECK(ret == 0, NULL, "Error with %d.\n", ret);

            int adc = lsadc_get_value();
            if (exposure_info.AveLuma <= 11 && adc < g_ircut_args->adc_threshold)
            {
                stat = 0;
            }
            else if (adc >= g_ircut_args->adc_threshold)
            {
                stat = 1;
            }
            //DBG("AveLuma: %d, adc: %d\n", exposure_info.AveLuma, adc);
        }
        else if (g_ircut_args->ir_mode == SAL_IR_MODE_ON)
        {
            stat = 0;
        }
        else if (g_ircut_args->ir_mode == SAL_IR_MODE_OFF)
        {
            stat = 1;
        }

        if (stat != g_ircut_args->last_stat)
        {
            int pass_time = util_time_pass(&g_ircut_args->last_stat_time);
            //DBG("passtime: %dms, stat: %d\n", pass_time, stat);
            if (pass_time > 2000)
            {
                ret = sal_ircut_switch_mode(stat);
                CHECK(ret == 0, NULL, "Error with %#x.\n", ret);
                g_ircut_args->last_stat = stat;
            }
        }
        else
        {
            util_time_abs(&g_ircut_args->last_stat_time);
        }

        pthread_mutex_unlock(&g_ircut_args->mutex);
        usleep(200*1000);
    }

    return NULL;
}

int sal_ircut_init()
{
    CHECK(NULL == g_ircut_args, -1, "reinit error, please exit first.\n");

    g_ircut_args = (sal_ircut_args*)malloc(sizeof(sal_ircut_args));
    CHECK(g_ircut_args != NULL, -1, "failed to malloc %d bytes.\n", sizeof(sal_ircut_args));
    memset(g_ircut_args, 0, sizeof(sal_ircut_args));
    pthread_mutex_init(&g_ircut_args->mutex, NULL);

    int ret = -1;
    g_ircut_args->ir_mode = SAL_IR_MODE_AUTO;
    g_ircut_args->adc_threshold = 80;
    util_time_abs(&g_ircut_args->last_stat_time);

    ret = sal_ircut_switch_mode(1); // init color
    CHECK(ret == 0, -1, "Error with %#x.\n", ret);
    g_ircut_args->last_stat = 1;

    ret = gpio_init();
    CHECK(ret == 0, -1, "Error with %#x.\n", ret);

    ret = lsadc_init();
    CHECK(ret == 0, -1, "Error with %#x.\n", ret);

    g_ircut_args->running = 1;
    ret = pthread_create(&g_ircut_args->pid, NULL, sal_ircut_thread, NULL);
    CHECK(ret == 0, -1, "Error with %s.\n", strerror(errno));

    return 0;
}

int sal_ircut_exit()
{
    CHECK(NULL != g_ircut_args, -1,"moudle is not inited.\n");

    //int ret = -1;

    if (g_ircut_args->running)
    {
        g_ircut_args->running = 0;
        pthread_join(g_ircut_args->pid, NULL);
    }

    pthread_mutex_destroy(&g_ircut_args->mutex);
    free(g_ircut_args);
    g_ircut_args = NULL;

    return 0;
}

int sal_ircut_mode_set(SAL_IR_MODE mode)
{
    CHECK(NULL != g_ircut_args, -1, "moudle is not inited.\n");

    pthread_mutex_lock(&g_ircut_args->mutex);

    g_ircut_args->ir_mode = mode;

    pthread_mutex_unlock(&g_ircut_args->mutex);

    return 0;
}






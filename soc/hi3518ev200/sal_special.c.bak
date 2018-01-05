#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include "hi_comm.h"
#include "sal_isp.h"
#include "sal_md.h"
#include "sal_debug.h"
#include "sal_config.h"
#include "sal_util.h"
#include "sal_statistics.h"
#include "sal_special.h"
#include "sal_ircut.h"
#include "sal_lbr.h"
#include "sal_osd.h"

typedef struct sal_special_args
{
    int disable;

    pthread_t pid;
}sal_special_args;

static sal_special_args g_special_args;

static void* special_init_all(void* arg)
{
    prctl(PR_SET_NAME, __FUNCTION__);
    //pthread_detach(pthread_self());
    sleep(1); //delay

    int ret = 0;

    //config of isp
    sal_isp_s isp_param;
    memset(&isp_param, 0, sizeof(isp_param));
    isp_param.slow_shutter_enable = 0;
    isp_param.image_attr.saturation = 128;
    isp_param.image_attr.contrast = 128;
    isp_param.image_attr.brightness = 128;
    isp_param.image_attr.sharpness = 128;
    ret = sal_isp_init(&isp_param);
    CHECK(ret == 0, NULL, "Error with %#x.\n", ret);

    ret = sal_ircut_init();
    CHECK(ret == 0, NULL, "Error with %#x.\n", ret);

    ret = sal_lbr_init();
    CHECK(ret == 0, NULL, "Error with %#x.\n", ret);

    ret = sal_osd_init();
    CHECK(ret == 0, NULL, "Error with %#x.\n", ret);

    return NULL;
}

static void* special_exit_all(void* arg)
{
    prctl(PR_SET_NAME, __FUNCTION__);
    //pthread_detach(pthread_self());
    //sleep(1);

    int ret = 0;

    ret = sal_osd_exit();
    CHECK(ret == 0, NULL, "Error with %#x.\n", ret);

    ret = sal_lbr_exit();
    CHECK(ret == 0, NULL, "Error with %#x.\n", ret);

    ret = sal_ircut_exit();
    CHECK(ret == 0, NULL, "Error with %#x.\n", ret);

    ret = sal_isp_exit();
    CHECK(ret == 0, NULL, "Error with %#x.\n", ret);

    return NULL;
}

int sal_special_start()
{
    int s32Ret = 0;

    if (!g_special_args.disable)
    {
        s32Ret = pthread_create(&g_special_args.pid, NULL, special_init_all, NULL);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %s.\n", strerror(errno));
    }

    return 0;
}

int sal_special_stop()
{
    int s32Ret = 0;

    if (g_special_args.pid)
    {
        pthread_join(g_special_args.pid, NULL);

        pthread_t pid = 0;
        s32Ret = pthread_create(&pid, NULL, special_exit_all, NULL);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %s.\n", strerror(errno));

        pthread_join(pid, NULL); //wait to all module exit done
    }

    return 0;
}

int sal_special_disable(int disable)
{
    g_special_args.disable = disable;

    return 0;
}



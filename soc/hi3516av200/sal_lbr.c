#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include "hi_comm.h"
#include "sal_av.h"
#include "sal_md.h"
#include "sal_debug.h"
#include "sal_config.h"
#include "sal_util.h"
#include "sal_statistics.h"


#define STATIC_THR 50
#define STATIC_SCALE 0.2

#define SMALL_MOTION_THR 250
#define SMALL_MOTION_SCALE 0.4

#define CHANGE_MODE_DELAY 2000 // 降低码率延时(毫秒)
#define MAX_STREAM_NUM 2

typedef struct sal_lbr_args
{
    int video_chn_num;
    sal_stream_s stream[MAX_STREAM_NUM];
    struct timeval last_mode_time[MAX_STREAM_NUM];
    int last_bitrate[MAX_STREAM_NUM];
    struct timeval current;
    char mode[32];

    int last_bb_stat[MAX_STREAM_NUM]; // 为1表示码率过冲 bb: big bitrate
    struct timeval last_bb_time[MAX_STREAM_NUM];

    unsigned long long keep_time[4];
    struct timeval last_mode_begin;
    char vp_str[32];

    int min_bitrate;

    int block_h;
    int block_v;
    int precent;

    int running;
    pthread_t pid;
    pthread_mutex_t mutex;
}sal_lbr_args;

static sal_lbr_args* g_lbr_args = NULL;

static int lbr_time_sub(struct timeval* pre, struct timeval* cur)
{
    int pass_time = (cur->tv_sec - pre->tv_sec) * 1000 + (cur->tv_usec - pre->tv_usec) / 1000;

    return pass_time;
}

static int lbr_statistics(int index)
{
    static int pre_idx = -1;
    int update = 0;
    int pass_time = 0;

    if (pre_idx != index)
    {
        if (pre_idx != -1)
        {
            pass_time = lbr_time_sub(&g_lbr_args->last_mode_begin, &g_lbr_args->current);
            g_lbr_args->keep_time[pre_idx] += pass_time;
            //DBG("pre_idx: %d, pass_time: %dms\n", pre_idx, pass_time);
        }

        util_time_abs(&g_lbr_args->last_mode_begin);
        update = 1;
        pre_idx = index;
    }
    else // 定时刷新
    {
        pass_time = lbr_time_sub(&g_lbr_args->last_mode_begin, &g_lbr_args->current);
        if (pass_time > (1*60*1000))
        {
            g_lbr_args->keep_time[pre_idx] += pass_time;
            //DBG("pre_idx: %d, pass_time: %dms\n", pre_idx, pass_time);
            util_time_abs(&g_lbr_args->last_mode_begin);
            update = 1;
        }
    }

    if (update)
    {
        unsigned long long total = 0;
        unsigned int i = 0;
        for (i = 0; i < sizeof(g_lbr_args->keep_time)/sizeof(*g_lbr_args->keep_time); i++)
        {
            total += g_lbr_args->keep_time[i];
        }
        total = (0==total) ? 1 : total;
        int precent[4];
        memset(precent, 0, sizeof(precent));
        for (i = 0; i < sizeof(g_lbr_args->keep_time)/sizeof(*g_lbr_args->keep_time); i++)
        {
            precent[i] = g_lbr_args->keep_time[i]*100/total;
        }

        snprintf(g_lbr_args->vp_str, sizeof(g_lbr_args->vp_str), "vp[%02d%%_%02d%%_%02d%%_%02d%%]", precent[0], precent[1], precent[2], precent[3]);
        //DBG("%llu %llu %llu %llu, %s\n", g_lbr_args->keep_time[0], g_lbr_args->keep_time[1], g_lbr_args->keep_time[2], g_lbr_args->keep_time[3], g_lbr_args->vp_str);

        if (total/1000 > (30*24*60*60)) // 30 days
        {
            DBG("total statistics time[%llu], reset it.\n", total);
            memset(g_lbr_args->keep_time, 0, sizeof(g_lbr_args->keep_time));
            pre_idx = -1;
        }
    }

    return 0;
}

static int lbr_bitrate_get(int precent, int max_bitrate)
{
    int target_bitrate = max_bitrate;
    int index = 0;
    if (precent < STATIC_THR)
    {
        target_bitrate *= STATIC_SCALE;
        snprintf(g_lbr_args->mode, sizeof(g_lbr_args->mode), "s[%04d]", precent);
        index = 0;
    }
    else if (precent < SMALL_MOTION_THR)
    {
         target_bitrate *= SMALL_MOTION_SCALE;
         snprintf(g_lbr_args->mode, sizeof(g_lbr_args->mode), "m[%04d]", precent);
         index = 1;
    }
    else
    {
        snprintf(g_lbr_args->mode, sizeof(g_lbr_args->mode), "b[%04d]", precent);
        index = 2;
    }

    lbr_statistics(index);

/*
    if (NIGHT_MODE == ircut_mode_get())
    {
        target_bitrate += max_bitrate * 0.1;
    }
*/

    if (target_bitrate > max_bitrate)
    {
        target_bitrate = max_bitrate;
    }

    if (target_bitrate < g_lbr_args->min_bitrate)
    {
        target_bitrate = g_lbr_args->min_bitrate;
    }

    return target_bitrate;
}

static int lbr_set_bitrate(int stream, int target_bitrate, int max_bitrate)
{
    if (g_lbr_args->last_bitrate[stream] == target_bitrate)
    {
        util_time_abs(&g_lbr_args->last_mode_time[stream]);
        return 0;
    }

    sal_video_qp_s video_qp;
    memset(&video_qp, 0, sizeof(video_qp));
    SAL_BITRATE_CONTROL_E cbr = SAL_BITRATE_CONTROL_CBR;
    int pass_time = lbr_time_sub(&g_lbr_args->last_mode_time[stream], &g_lbr_args->current);
    //DBG("channel: %d, pass time: %d ms\n", stream, pass_time);

    if (target_bitrate < g_lbr_args->last_bitrate[stream] && pass_time < CHANGE_MODE_DELAY)
    {
        // need delay
        return 0;
    }

    int ret = sal_video_lbr_set(stream, cbr, target_bitrate);
    CHECK(ret == 0, -1, "Error with %d.\n", ret);

    ret = sal_video_cbr_qp_get(stream, &video_qp);
    CHECK(ret == 0, -1, "Error with %d.\n", ret);

    video_qp.max_qp = (target_bitrate == max_bitrate) ? 51 : 35;
    video_qp.min_i_qp = 20;

    ret = sal_video_cbr_qp_set(stream, &video_qp);
    CHECK(ret == 0, -1, "Error with %d.\n", ret);

    //DBG("channel: %d, target_bitrate: %d\n", stream, target_bitrate);
    util_time_abs(&g_lbr_args->last_mode_time[stream]);
    g_lbr_args->last_bitrate[stream] = target_bitrate;

    return 0;
}

static int lbr_check_bitrate(int stream, int max_bitrate)
{
    int ret = -1;
    int real_bitrate = sal_statistics_bitrate_get(stream);
    max_bitrate += 500; // 500kbps 波动码率
    int isBigRate = 0;
    if (real_bitrate > max_bitrate)
    {
        isBigRate = 1;
    }

    if (g_lbr_args->last_bb_stat[stream] != isBigRate)
    {
        g_lbr_args->last_bb_stat[stream] = isBigRate;
        util_time_abs(&g_lbr_args->last_bb_time[stream]);
    }

    if (g_lbr_args->last_bb_stat[stream])
    {
        int pass_time = lbr_time_sub(&g_lbr_args->last_bb_time[stream], &g_lbr_args->current);
        //DBG("%s channel: %d, pass time: %d ms\n", __FUNCTION__, stream, pass_time);
        if (pass_time >= CHANGE_MODE_DELAY)
        {
            DBG("channel: %d, real bitrate[%d] more than max[%d], set qp max[51]\n", stream, real_bitrate, max_bitrate);
            sal_video_qp_s video_qp;
            ret = sal_video_cbr_qp_get(stream, &video_qp);
            CHECK(ret == 0, -1, "Error with %d.\n", ret);

            video_qp.max_qp = 51;

            ret = sal_video_cbr_qp_get(stream, &video_qp);
            CHECK(ret == 0, -1, "Error with %d.\n", ret);
            util_time_abs(&g_lbr_args->last_bb_time[stream]);
        }
    }

    return 0;
}

void* lbr_proc(void* args)
{
    prctl(PR_SET_NAME, __FUNCTION__);
    int ret = -1;
    int i = 0;

    while (g_lbr_args->running)
    {
        pthread_mutex_lock(&g_lbr_args->mutex);
        util_time_abs(&g_lbr_args->current);

        for (i = 0; i < g_lbr_args->video_chn_num; i++)
        {
            int precent = g_lbr_args->precent;
            int target_bitrate = lbr_bitrate_get(precent, g_config.lbr[i].bitrate);

            if (g_config.lbr[i].enable)
            {
                ret = lbr_set_bitrate(i, target_bitrate, g_config.lbr[i].bitrate);
                CHECK(ret == 0, NULL, "Error with %d.\n", ret);

                ret = lbr_check_bitrate(i, g_config.lbr[i].bitrate);
                CHECK(ret == 0, NULL, "error with %d.\n", ret);
            }
        }

        pthread_mutex_unlock(&g_lbr_args->mutex);
        usleep(200*1000);
     }

    return NULL;
}

static int lbr_md_cb(int* precent, int num, int pixel_num)
{
    //CHECK(precent != NULL || num <= TP_MAX_BLOCK_NUM, ERROR, "invalid args.\n");

    int i = 0;
    int sum = 0;
    for (i = 0; i < num; i++)
    {
        //g_md->precent[i] = precent[i];
        sum += precent[i];
    }
    g_lbr_args->precent = sum/num;
    //g_md->full_screen_precent = sum/num;
    //g_md->full_screen_precent1 = pixel_num * 10000 / (g_enc_cap.md.widht * g_enc_cap.md.height);

    return 0;
}

int sal_lbr_init()
{
    CHECK(NULL == g_lbr_args, -1, "reinit error, please uninit first.\n");

    int ret = -1;
    g_lbr_args = (sal_lbr_args*)malloc(sizeof(sal_lbr_args));
    CHECK(NULL != g_lbr_args, -1, "malloc %d bytes failed.\n", sizeof(sal_lbr_args));

    memset(g_lbr_args, 0, sizeof(sal_lbr_args));
    pthread_mutex_init(&g_lbr_args->mutex, NULL);

    unsigned int i = 0;
    for (i = 0; i < MAX_STREAM_NUM; i++)
    {
        if (g_config.video[i].enable)
        {
            g_lbr_args->video_chn_num++;
        }
    }
    g_lbr_args->min_bitrate = 200;
    g_lbr_args->block_h = 4;
    g_lbr_args->block_v = 4;

    ret = sal_md_init(g_lbr_args->block_h, g_lbr_args->block_v);
    CHECK(ret == 0, -1, "Error with %#x.\n", ret);

    ret = sal_md_cb_set(lbr_md_cb);
    CHECK(ret == 0, -1, "Error with %#x.\n", ret);

    g_lbr_args->running = 1;
    ret = pthread_create(&g_lbr_args->pid, NULL, lbr_proc, NULL);
    CHECK(ret == 0, -1, "Error with %s.\n", strerror(errno));

    return 0;
}

int sal_lbr_exit()
{
    CHECK(NULL != g_lbr_args, -1,"moudle is not inited.\n");

    int ret = -1;
    if (g_lbr_args->running)
    {
        g_lbr_args->running = 0;
        pthread_join(g_lbr_args->pid, NULL);
    }

    ret = sal_md_exit();
    CHECK(ret == 0, -1, "Error with %#x.\n", ret);

    pthread_mutex_destroy(&g_lbr_args->mutex);
    free(g_lbr_args);
    g_lbr_args = NULL;

    return 0;
}

int sal_lbr_get_str(char* buf, int len)
{
    CHECK(NULL != g_lbr_args, -1,"moudle is not inited.\n");
    CHECK(len > (int)strlen(g_lbr_args->mode), -1,"len[%d] is too small,expect[%d]\n", len, strlen(g_lbr_args->mode));

    pthread_mutex_lock(&g_lbr_args->mutex);

    memset(buf, 0, len);
    strcpy(buf, g_lbr_args->mode);

    pthread_mutex_unlock(&g_lbr_args->mutex);

    return 0;
}

int sal_lbr_get_vpstr(char* buf, int len)
{
    CHECK(NULL != g_lbr_args, -1,"moudle is not inited.\n");
    CHECK(len > (int)strlen(g_lbr_args->vp_str), -1,"len[%d] is too small,expect[%d]\n", len, strlen(g_lbr_args->vp_str));

    pthread_mutex_lock(&g_lbr_args->mutex);

    memset(buf, 0, len);
    strcpy(buf, g_lbr_args->vp_str);

    pthread_mutex_unlock(&g_lbr_args->mutex);

    return 0;
}





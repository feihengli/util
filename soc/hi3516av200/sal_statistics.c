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

#include "sal_av.h"
#include "sal_debug.h"


#define STAT_MAX_STREAM_NUM 4

typedef struct sal_statistics_args
{
    unsigned int frame_seq;
    unsigned int frame_cnt;
    unsigned int data_cnt;
    double last_pts;

    double bitrate;
    double framerate;
    unsigned int last_keyframe;
}sal_statistics_args;

static sal_statistics_args g_stat_args[STAT_MAX_STREAM_NUM];

static int stat_calc(int id, double pts, int len, int keyFrame, int gop, int smooth_frames)
{
    g_stat_args[id].frame_cnt++;
    g_stat_args[id].data_cnt += len;

    if (g_stat_args[id].last_pts == 0.0)
    {
        g_stat_args[id].last_pts = pts;
    }

    /* 记录一个gop的数据总量来计算实际码率和帧率   */
    /* 向前滑动smooth_frames帧数重新估算码率和帧率 */
    /* 实际计算pass_time的时候会跟实际的帧数少一帧 */
    if (g_stat_args[id].frame_cnt >= gop)
    {
        double pass_time = pts - g_stat_args[id].last_pts;
        g_stat_args[id].bitrate = g_stat_args[id].data_cnt*8*1000.0/pass_time;
        g_stat_args[id].framerate = (g_stat_args[id].frame_cnt-1)*1000.0/pass_time;

        double avg_data = 1.0*g_stat_args[id].data_cnt/g_stat_args[id].frame_cnt;
        double avg_time = 1.0*pass_time/(g_stat_args[id].frame_cnt-1);

        g_stat_args[id].frame_cnt -= smooth_frames;
        g_stat_args[id].data_cnt -= avg_data*smooth_frames;
        g_stat_args[id].last_pts += avg_time*smooth_frames;

/*
        if (id == 1)
        {
            DBG("id: %d, bitrate: %.1f, framerate: %.1f, pass_time: %.1f, avg_time: %.1f\n",
                id, g_stat_args[id].bitrate, g_stat_args[id].framerate, pass_time, avg_time);
        }
*/
    }

    return 0;
}

int sal_statistics_proc(int id, double pts, int len, int keyFrame, int gop, int smooth_frames)
{
    g_stat_args[id].frame_seq++;

    if (keyFrame)
    {
        //发生了强制I帧事件，重新调整信息
        if (g_stat_args[id].frame_seq-g_stat_args[id].last_keyframe != gop)
        {
            //DBG("gop is not correct.reset statistics info.\n");
            memset(&g_stat_args[id], 0, sizeof(sal_statistics_args));
        }
        g_stat_args[id].last_keyframe = g_stat_args[id].frame_seq;
    }

    stat_calc(id, pts, len, keyFrame, gop, smooth_frames);

    return 0;
}

int sal_statistics_bitrate_get(int id)
{
    CHECK(id >= 0 && id < STAT_MAX_STREAM_NUM, -1, "Error with %#x.\n", id);

    return g_stat_args[id].bitrate/1000;
}

int sal_statistics_framerate_get(int id)
{
    CHECK(id >= 0 && id < STAT_MAX_STREAM_NUM, -1, "Error with %#x.\n", id);

    return g_stat_args[id].framerate;
}


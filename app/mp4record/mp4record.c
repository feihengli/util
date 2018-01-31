#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "sal_av.h"
#include "sal_audio.h"
#include "sal_jpeg.h"
#include "sal_osd.h"
#include "sal_debug.h"
#include "sal_frame_pool.h"
#include "sal_rtp.h"
#include "sal_mp4record.h"
#include "mp4v2/mp4v2.h"

static int test_exit = 0;
static handle g_hndMp4;

void sigint(int dummy)
{
    printf("got int signal,exit!\n");
    test_exit = 1;
}

void init_signals(void)
{
    struct sigaction sa;

    sa.sa_flags = 0;

    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGTERM);
    sigaddset(&sa.sa_mask, SIGINT);

    sa.sa_handler = sigint;
    sigaction(SIGTERM, &sa, NULL);

    sa.sa_handler = sigint;
    sigaction(SIGINT, &sa, NULL);

    signal(SIGPIPE, SIG_IGN);
}

int get_audio_frame_cb(char *frame, unsigned long len, double timestamp)
{


    return 0;
}

int get_video_frame_cb(int stream, char *frame, unsigned long len, int key, double pts, SAL_ENCODE_TYPE_E encode_type)
{
    int ret = -1;
    FRAME_TYPE_E type = FRAME_TYPE_INVALID;
    if (encode_type == SAL_ENCODE_TYPE_H264)
    {
        type = FRAME_TYPE_H264;
    }
    else if (encode_type == SAL_ENCODE_TYPE_H265)
    {
        type = FRAME_TYPE_H265;
    }
    
    if (1 ==stream)
    {
        unsigned char* pu8Frame = NULL;
        unsigned int u32FrameSize = 0;
        unsigned char* pu8Tmp = (unsigned char*)frame;
        unsigned int u32TmpSize = len;
        int offset;
        do
        {
            offset = rtp_vframe_split(pu8Tmp, u32TmpSize);
            if (offset != -1)
            {
                //DBG("offset=%d", offset);
                pu8Frame = pu8Tmp;
                u32FrameSize = offset;

                pu8Tmp += offset;
                u32TmpSize -= offset;
            }
            else
            {
                //DBG("u32TmpSize=%u", u32TmpSize);
                pu8Frame = pu8Tmp;
                u32FrameSize = u32TmpSize;
            }
            ret = mp4VEncoderWrite(g_hndMp4, pu8Frame, u32FrameSize);
            CHECK(ret == 0, -1, "Error with: %#x\n", ret);
        }
        while (offset != -1);
    }
    
    return 0;
}

int get_jpeg_frame_cb(char *frame, int len)
{
    return 0;
}

int main(int argc, char** argv)
{
    int ret = -1;
    init_signals();

    if (argc > 1)
    {
        if (!strcmp(argv[1], "-v") || !strcmp(argv[1], "--version"))
        {
            char version[512];
            memset(version, 0, sizeof(version));

            ret = sal_sys_version(version, sizeof(version));
            CHECK(ret == 0, -1, "Error with: %#x\n", ret);

            printf("%s\n", version);
            return 0;
        }
    }

    
    g_hndMp4 = mp4EncoderInit("testh264.mp4", 640, 360, 15);
    CHECK(g_hndMp4, -1, "Error with: %#x\n", g_hndMp4);
    
    //config of video
    sal_video_s video;
    memset(&video, 0, sizeof(video));
    video.cb = get_video_frame_cb;
    video.stream[0].enable = 1;
    video.stream[0].width = 3840;
    video.stream[0].height = 2160;
    video.stream[0].framerate = 15;
    video.stream[0].bitrate = 2500;
    video.stream[0].gop = 2 * video.stream[0].framerate;
    video.stream[0].bitrate_ctl = SAL_BITRATE_CONTROL_CBR;
    video.stream[0].encode_type = SAL_ENCODE_TYPE_H264;

    video.stream[1].enable = 1;
    video.stream[1].width = 640;
    video.stream[1].height = 360;
    video.stream[1].framerate = 15;
    video.stream[1].bitrate = 500;
    video.stream[1].gop = 2 * video.stream[1].framerate;
    video.stream[1].bitrate_ctl = SAL_BITRATE_CONTROL_CBR;
    video.stream[1].encode_type = SAL_ENCODE_TYPE_H264;
    ret = sal_sys_init(&video);
    CHECK(ret == 0, -1, "Error with: %#x\n", ret);
    DBG("sys video init done.\n");
    
    while (!test_exit)
    {
        usleep(1);
    }
    
    //sal_audio_exit();
    sal_sys_exit();
    
    mp4Encoderclose(g_hndMp4);


    return 0;
}






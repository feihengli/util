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
#include "sal_rtsp_server.h"
#include "sal_draw_rectangle.h"

static int test_exit = 0;

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
    if (gHndMainFramePool)
    {
        frame_pool_add(gHndMainFramePool, frame, len, FRAME_TYPE_G711A, 1, timestamp);
    }

    if (gHndSubFramePool)
    {
        frame_pool_add(gHndSubFramePool, frame, len, FRAME_TYPE_G711A, 1, timestamp);
    }

    return 0;
}

int get_video_frame_cb(int stream, char *frame, unsigned long len, int key, double pts, SAL_ENCODE_TYPE_E encode_type)
{
    FRAME_TYPE_E type = FRAME_TYPE_INVALID;
    if (encode_type == SAL_ENCODE_TYPE_H264)
    {
        type = FRAME_TYPE_H264;
    }
    else if (encode_type == SAL_ENCODE_TYPE_H265)
    {
        type = FRAME_TYPE_H265;
    }
    
    if (stream == 0 && gHndMainFramePool)
    {
        frame_pool_add(gHndMainFramePool, frame, len, type, key, pts);
    }
    else if (stream == 1 && gHndSubFramePool)
    {
        frame_pool_add(gHndSubFramePool, frame, len, type, key, pts);
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

    gHndMainFramePool = frame_pool_init(30);
    CHECK(gHndMainFramePool, -1, "Error with: %#x\n", gHndMainFramePool);

    gHndSubFramePool = frame_pool_init(30);
    CHECK(gHndSubFramePool, -1, "Error with: %#x\n", gHndSubFramePool);

    //config of video
    sal_video_s video;
    memset(&video, 0, sizeof(video));
    video.cb = get_video_frame_cb;
    video.stream[0].enable = 1;
    video.stream[0].width = 4000;
    video.stream[0].height = 3000;
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
    video.stream[1].encode_type = SAL_ENCODE_TYPE_H265;
    ret = sal_sys_init(&video);
    CHECK(ret == 0, -1, "Error with: %#x\n", ret);
    DBG("sys video init done.\n");

    //config of audio
    /*sal_audio_s audio;
    memset(&audio, 0, sizeof(audio));
    audio.enable = 1;
    strcpy(audio.encType, "G.711A");
    audio.channels = 1;
    audio.bitWidth = 16;
    audio.volume = 100;
    audio.sampleRate = 8000;
    audio.ptNumPerFrm = 320;
    audio.cb = get_audio_frame_cb;
    ret = sal_audio_init(&audio);
    CHECK(ret == 0, -1, "Error with: %#x\n", ret);
    DBG("sys audio init done.\n");*/
    
    
    ret = sal_dr_init();
    CHECK(ret == 0, -1, "Error with: %#x\n", ret);
    
   /* ret = sal_osd_init();
    CHECK(ret == 0, -1, "Error with: %#x\n", ret);*/

    handle hndRtsps = rtsps_init(554);
    CHECK(hndRtsps, -1, "Error with: %#x\n", hndRtsps);
    
    bmp_demo();
    
    while (!test_exit)
    {
        usleep(1);
    }
    
    ret = sal_osd_exit();
    CHECK(ret == 0, -1, "Error with: %#x\n", ret);
    
    ret = sal_jpeg_exit();
    CHECK(ret == 0, -1, "Error with: %#x\n", ret);

    rtsps_destroy(hndRtsps);
    //sal_audio_exit();
    sal_sys_exit();

    frame_pool_destroy(gHndSubFramePool);
    gHndSubFramePool = NULL;
    frame_pool_destroy(gHndMainFramePool);
    gHndMainFramePool = NULL;

    return 0;
}






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
#include "sal_isp.h"
#include "sal_yuv.h"
#include "sal_md.h"
#include "sal_jpeg.h"
#include "sal_lbr.h"
#include "sal_bitmap.h"
#include "sal_osd.h"
#include "sal_ircut.h"
#include "sal_debug.h"
#include "sal_frame_pool.h"

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

handle gHndMainFramePool = NULL;
handle gHndMainReader = NULL;
handle gHndMainReader1 = NULL;

int get_audio_frame_cb(char *frame, unsigned long len, double timestamp)
{
    return 0;
}

int get_video_frame_cb(int stream, char *frame, unsigned long len, int key, double pts)
{
    frame_pool_add(gHndMainFramePool, frame, len, FRAME_TYPE_H264, key, pts);

    return 0;
}





int main(int argc, char** argv)
{
    init_signals();

    if (argc > 1)
    {
        if (!strcmp(argv[1], "-v") || !strcmp(argv[1], "--version"))
        {
            char version[512];
            memset(version, 0, sizeof(version));
            sal_sys_version(version, sizeof(version));
            printf("%s\n", version);
            return 0;
        }
    }

    gHndMainFramePool = frame_pool_init(60);

    //config of video
    sal_video_s video;
    memset(&video, 0, sizeof(video));
    video.cb = get_video_frame_cb;
    video.stream[0].enable = 1;
    video.stream[0].width = 1920;
    video.stream[0].height = 1080;
    video.stream[0].framerate = 15;
    video.stream[0].bitrate = 1000;
    video.stream[0].gop = 4 * video.stream[0].framerate;
    video.stream[0].bitrate_ctl = SAL_BITRATE_CONTROL_VBR;

    video.stream[1].enable = 1;
    video.stream[1].width = 640;
    video.stream[1].height = 360;
    video.stream[1].framerate = 15;
    video.stream[1].bitrate = 500;
    video.stream[1].gop = 4 * video.stream[1].framerate;
    video.stream[1].bitrate_ctl = SAL_BITRATE_CONTROL_VBR;
    sal_sys_init(&video);

    //config of audio
    sal_audio_s audio;
    memset(&audio, 0, sizeof(audio));
    audio.enable = 1;
    strcpy(audio.encType, "LPCM");
    audio.channels = 1;
    audio.bitWidth = 16;
    audio.volume = 80;
    audio.sampleRate = 8000;
    audio.ptNumPerFrm = 320;
    audio.cb = get_audio_frame_cb;
    sal_audio_init(&audio);



    sleep(1);
    gHndMainReader = frame_pool_register(gHndMainFramePool, 1);
    gHndMainReader1 = frame_pool_register(gHndMainFramePool, 0);

    while (!test_exit)
    {

        frame_info_s* frame = frame_pool_get(gHndMainReader);
        if (frame)
        {
            DBG("sequence: %d, len: %d\n", frame->sequence, frame->len);
            frame_pool_release(gHndMainReader, frame);
        }
        else
        {
            DBG("get NULL\n");
        }
        frame_info_s* frame1 = frame_pool_get(gHndMainReader1);
        if (frame1)
        {
            DBG("sequence: %d, len: %d\n", frame1->sequence, frame1->len);
            frame_pool_release(gHndMainReader1, frame1);
        }
        else
        {
            DBG("get NULL\n");
        }
        usleep(1);
    }

    sal_audio_exit();
    sal_sys_exit();

    return 0;
}

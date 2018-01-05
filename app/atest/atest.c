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
#include "sal_debug.h"
//#include "sal_special.h"
#include "sal_util.h"

static int test_exit = 0;
static FILE* fp = NULL;
static int arecord = 0;
static int aplay = 0;

static void sigint(int dummy)
{
    printf("got int signal,exit!\n");
    test_exit = 1;
}

static void init_signals(void)
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
    //DBG("len: %d, timestamp: %f\n", len, timestamp);
    if (arecord && fp)
    {
        fwrite(frame, 1, len, fp);
    }

    return 0;
}

static void usage(char *path)
{
    printf("Usage: %s [-rpf]\n", path);
    printf("\t-r\t [filepath] record audio\n");
    printf("\t-p\t [filepath] play audio\n");
    printf("\t-f\t [type] aenc type [ LPCM G.711U/G.711A ]\n");
    printf("\t-d\t [duration] record duration [ ms ]\n");
}

int main(int argc, char** argv)
{
    init_signals();
    if (argc == 1)
    {
        usage(argv[0]);
        return -1;
    }

    char format[128];
    memset(format, 0, sizeof(format));
    char file[128];
    memset(file, 0, sizeof(file));

    strcpy(format, "LPCM");
    int duration = 10*1000;

    int ch = 0;
    while ((ch = getopt(argc, argv, "r:p:f:d:h")) != -1)
    {
        switch (ch)
        {
            case 'r':
                arecord = 1;
                strcpy(file, optarg);
                break;
            case 'p':
                aplay = 1;
                strcpy(file, optarg);
                break;
            case 'f':
                memset(format, 0, sizeof(format));
                strcpy(format, optarg);
                break;
            case 'd':
                duration = atoi(optarg);
                break;
            default:
                usage(argv[0]);
                return -1;
        }
    }

    CHECK(arecord ^ aplay, -1, "Error with: %s\n", "arecord ^ aplay");
    CHECK(!strcmp(format, "G.711U") || !strcmp(format, "G.711A")
            || !strcmp(format, "LPCM"), -1, "Error with: %s\n", format);


    if (arecord)
    {
        if (!access(file, F_OK))
        {
            remove(file);
        }
        fp = fopen(file, "a+");
        CHECK(fp, -1, "failed to open [%s]: %s\n", file, strerror(errno));
    }
    if (aplay)
    {
        CHECK(!access(file, F_OK), -1, "Error with: %s\n", file);
        fp = fopen(file, "r");
        CHECK(fp, -1, "failed to open [%s]: %s\n", file, strerror(errno));
    }

    DBG("arecord: %d\n", arecord);
    DBG("aplay: %d\n", aplay);
    DBG("file: %s\n", file);
    DBG("format: %s\n", format);


    //do not enable other moudles
    //sal_special_disable(1);

    //config of video
    sal_video_s video;
    memset(&video, 0, sizeof(video));
    video.cb = NULL;
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
    strcpy(audio.encType, format);
    audio.channels = 1;
    audio.bitWidth = 16;
    audio.volume = 80;
    audio.sampleRate = 8000;
    audio.ptNumPerFrm = 320;
    audio.cb = get_audio_frame_cb;
    sal_audio_init(&audio);

    DBG("encType: %s\n", audio.encType);
    DBG("channels: %d\n", audio.channels);
    DBG("bitWidth: %d\n", audio.bitWidth);
    DBG("volume: %d\n", audio.volume);
    DBG("sampleRate: %d\n", audio.sampleRate);
    DBG("ptNumPerFrm: %d\n", audio.ptNumPerFrm);

    struct timeval begin = {0, 0};
    util_time_abs(&begin);

    while (!test_exit)
    {
        if (aplay && fp)
        {
            int frame_size = audio.ptNumPerFrm*audio.bitWidth/8;
            char buffer[1024];
            while (!test_exit && !feof(fp))
            {
                memset(buffer, 0, sizeof(buffer));
                int len = fread(buffer, 1, frame_size, fp);
                if (len == frame_size)
                {
                    //DBG("len: %d\n", len);
                    sal_audio_play(buffer, frame_size);
                }
            }
            break;
        }

        if (arecord && fp)
        {
            int pass_time = util_time_pass(&begin);
            if (pass_time > duration)
            {
               DBG("arecord duration[%dms] done.\n", duration);
               break;
            }
        }

        usleep(1);
    }

    if (aplay)
    {
        sleep(2);
    }

    sal_audio_exit();
    sal_sys_exit();

    if (fp)
    {
        fclose(fp);
        fp = NULL;
    }

    return 0;
}

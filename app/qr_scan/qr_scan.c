#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/prctl.h>
#include <errno.h>
#include "../sal_av.h"
#include "../sal_yuv.h"
#include "../sal_debug.h"
#include "../sal_special.h"

#include "zbar.h"

static int test_exit = 0;
static zbar_image_t *zimg = NULL;
static zbar_image_scanner_t *zscn = NULL;
static char path_wpa[32] = "";
static char path_sequence[32] = "";

static void qr_detect_init()
{
    zscn = zbar_image_scanner_create();
    zbar_image_scanner_set_config(zscn, 0, ZBAR_CFG_ENABLE, 1);
    zbar_image_scanner_set_config(zscn, 0, ZBAR_CFG_X_DENSITY, 1);
    zbar_image_scanner_set_config(zscn, 0, ZBAR_CFG_Y_DENSITY, 1);

    zimg = zbar_image_create();
    zbar_image_set_size(zimg, 640, 360);
    zbar_image_set_format(zimg, zbar_fourcc('G','R','E','Y'));
}

static int qr_str_split(const char* input, const char* begin, const char* end, char* output, int len)
{
    char* find = strstr(input, begin);
    if (find)
    {
        char* find1 = strstr(find, end);
        if (find1)
        {
            int length = find1-find-strlen(begin);
            CHECK(len > length, -1, "error with %d\n", len);
            memcpy(output, find+strlen(begin), length);
        }
    }

    return 0;
}

static int qr_save2file(char* path, char* buffer, int len)
{
    int ret = -1;
    FILE * fp = fopen(path, "wb");
    CHECK(fp, -1, "failed to fopen[%s] with: %s\n", path, strerror(errno));

    ret = fwrite(buffer, 1, len, fp);
    CHECK(ret == len, -1, "Error with: %s\n", strerror(errno));
    fclose(fp);

    return 0;
}

static int qr_detect_img(char *img)
{
    int ret = -1;
    zbar_image_set_data(zimg, img, 640*360, NULL);
    zbar_scan_image(zscn, zimg);

    const zbar_symbol_t *symbol = zbar_image_first_symbol(zimg);

    //for(; symbol; symbol = zbar_symbol_next(symbol))
    if (symbol)
    {
        zbar_symbol_type_t typ = zbar_symbol_get_type(symbol);
        const char *data = zbar_symbol_get_data(symbol);
        DBG("decoded %s symbol \"%s\"\n", zbar_get_symbol_name(typ), data);

        char ssid[128];
        memset(ssid, 0, sizeof(ssid));
        char pwd[128];
        memset(pwd, 0, sizeof(pwd));
        char sequence[128];
        memset(sequence, 0, sizeof(sequence));

        qr_str_split(data, "ssid:", "\n", ssid, sizeof(ssid));
        DBG("ssid: %s\n", ssid);
        qr_str_split(data, "pwd:", "\n", pwd, sizeof(pwd));
        DBG("pwd: %s\n", pwd);
        qr_str_split(data, "sequenceid:", "\n", sequence, sizeof(sequence));
        DBG("sequenceid: %s\n", sequence);

        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));

        if (strlen(ssid) && strlen(pwd))
        {
            memset(buffer, 0, sizeof(buffer));
            sprintf(buffer, "ctrl_interface=/var/run/wpa_supplicant\n"
                            "network={\n"
                            "ssid=\"%s\"\n"
                            "psk=\"%s\"\n"
                            "}\n",
                            ssid,
                            pwd
                            );

            ret= qr_save2file(path_wpa, buffer, strlen(buffer));
            CHECK(ret == 0, -1, "Error with: %#x\n", ret);
        }

        if (strlen(sequence))
        {
            memset(buffer, 0, sizeof(buffer));
            sprintf(buffer, "%s", sequence);

            ret= qr_save2file(path_sequence, buffer, strlen(buffer));
            CHECK(ret == 0, -1, "Error with: %#x\n", ret);
        }

        ret = 0;
    }

    return ret;
}

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

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        printf("Usage: %s [wpa_file] [sequence_file]\n", argv[0]);
        return -1;
    }

    memset(path_wpa, 0, sizeof(path_wpa));
    strcpy(path_wpa, argv[1]);

    memset(path_sequence, 0, sizeof(path_sequence));
    strcpy(path_sequence, argv[2]);
    init_signals();

    //do not enable other moudles
    sal_special_disable(1);

    //config of video
    sal_video_s video;
    memset(&video, 0, sizeof(video));
    video.cb = NULL;
    video.stream[0].enable = 1;
    video.stream[0].width = 1920;
    video.stream[0].height = 1080;
    video.stream[0].framerate = 15;
    video.stream[0].bitrate = 2000;
    video.stream[0].gop = 4 * video.stream[0].framerate;
    video.stream[0].bitrate_ctl = SAL_BITRATE_CONTROL_CBR;

    video.stream[1].enable = 1;
    video.stream[1].width = 640;
    video.stream[1].height = 360;
    video.stream[1].framerate = 15;
    video.stream[1].bitrate = 500;
    video.stream[1].gop = 4 * video.stream[1].framerate;
    video.stream[1].bitrate_ctl = SAL_BITRATE_CONTROL_CBR;
    sal_sys_init(&video);

    qr_detect_init();
    char* buf = malloc(640*360*2);
    if (!buf)
    {
        printf("failed to malloc: %s\n", strerror(errno));
        return -1;
    }

    int ret = -1;
    while (!test_exit)
    {
        ret = sal_yuv_get(1, buf, 640*360*2);
        if (ret <= 0)
        {
            printf("failed to capture yuv.%#x\n", ret);
            usleep(1);
            continue;
        }
        ret = qr_detect_img(buf);
        if (ret == 0)
        {
            break;
        }
        usleep(1);
    }

    free(buf);
    sal_sys_exit();

    return 0;
}


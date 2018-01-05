#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../sal_av.h"
#include "../sal_audio.h"
#include "../sal_isp.h"
#include "../sal_yuv.h"
#include "../sal_md.h"
#include "../sal_jpeg.h"
#include "../sal_lbr.h"
#include "../sal_bitmap.h"
#include "../sal_osd.h"
#include "../sal_ircut.h"
#include "../sal_debug.h"
#include "../sal_upgrade.h"

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
    return 0;
}

int get_video_frame_cb(int stream, char *frame, unsigned long len, int key, double pts)
{
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

    sal_upgrade_init();
	
	version_s version_info;
    memset(&version_info, 0, sizeof(version_info));
    char* http_url = "http://rdtest.myhiott.com:8081/upgrade/ftp_test/lifeiheng/upgrade_cfg.xml";
	
    sal_upgrade_check(http_url, 5000, &version_info);

    sal_upgrade_enable(version_info.bUpdate);
	
    while (!test_exit)
    {
        sleep(1);
    }

    sal_upgrade_exit();

    return 0;
}

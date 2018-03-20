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
#include "ftp_client.h"

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



int main(int argc, char** argv)
{
    int ret = -1;
    init_signals();
    
    //ftp get test
    handle hndFtpc = ftp_client_init("192.168.0.32", 21, "root", "123456");
    CHECK(hndFtpc, -1, "Error with: %#x\n", hndFtpc);
    
    const char* pathFileName = "live555MediaServer";
    ret = ftp_client_startGet(hndFtpc, pathFileName, 10000);
    CHECK(ret == 0, -1, "Error with: %#x\n", ret);
    
    int progress = 0;
    while (!test_exit)
    {
        FTP_STATUS_E status = FTP_STATUS_INVALID;
        ret = ftp_client_Query(hndFtpc, &status);
        CHECK(ret == 0, -1, "Error with: %#x\n", ret);

        if (status == FTP_STATUS_PROGRESS || status == FTP_STATUS_FINISH_OK)
        {
            ret = ftp_client_ProgressGet(hndFtpc, &progress);
            CHECK(ret == 0, -1, "Error with: %#x\n", ret);

            DBG("Progress: %d\n", progress);
            if (progress == 100 && status == FTP_STATUS_FINISH_OK)
            {
                break;
            }
        }
        else
        {
            break;
        }
        usleep(500*1000);
    }
    
    unsigned char* buffer = NULL;
    int len = 0;
    ret = ftp_client_ContentGet(hndFtpc, &buffer, &len);
    CHECK(ret == 0, -1, "Error with: %#x\n", ret);
    
    CHECK(buffer, -1, "Error with: %#x\n", buffer);
    CHECK(len > 0, -1, "Error with: %#x\n", len);
    
    ret = util_file_write(pathFileName, buffer, len);
    CHECK(ret == 0, -1, "Error with: %#x\n", ret);
    
    ret = ftp_client_stop(hndFtpc);
    CHECK(ret == 0, -1, "Error with: %#x\n", ret);

    ret = ftp_client_exit(hndFtpc);
    CHECK(ret == 0, -1, "Error with: %#x\n", ret);
    
    //-------------------------------------------------------------------------
    //ftp put test
    hndFtpc = ftp_client_init("192.168.0.32", 21, "root", "123456");
    CHECK(hndFtpc, -1, "Error with: %#x\n", hndFtpc);
    
    pathFileName = "live555MediaServer";
    ret = util_file_size(pathFileName);
    CHECK(ret > 0, -1, "Error with: %#x\n", ret);
    
    int size = ret;
    buffer = malloc(size);
    CHECK(buffer, -1, "Error with: %#x\n", buffer);
    
    ret = util_file_read(pathFileName, buffer, size);
    CHECK(ret == 0, -1, "Error with: %#x\n", ret);
    
    ret = ftp_client_startPut(hndFtpc, buffer, size, pathFileName, 10000);
    CHECK(ret == 0, -1, "Error with: %#x\n", ret);
    
    progress = 0;
    while (!test_exit)
    {
        FTP_STATUS_E status = FTP_STATUS_INVALID;
        ret = ftp_client_Query(hndFtpc, &status);
        CHECK(ret == 0, -1, "Error with: %#x\n", ret);

        if (status == FTP_STATUS_PROGRESS || status == FTP_STATUS_FINISH_OK)
        {
            ret = ftp_client_ProgressGet(hndFtpc, &progress);
            CHECK(ret == 0, -1, "Error with: %#x\n", ret);

            DBG("Progress: %d\n", progress);
            if (progress == 100 && status == FTP_STATUS_FINISH_OK)
            {
                break;
            }
        }
        else
        {
            break;
        }
        usleep(500*1000);
    }
    
    ret = ftp_client_stop(hndFtpc);
    CHECK(ret == 0, -1, "Error with: %#x\n", ret);
    
    ret = ftp_client_exit(hndFtpc);
    CHECK(ret == 0, -1, "Error with: %#x\n", ret);
    
    while (!test_exit)
    {
        usleep(1);
    }

    return 0;
}






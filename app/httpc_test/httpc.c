#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../sal_debug.h"
#include "../sal_curl.h"
#include "../sal_util.h"
#include "../sal_md5.h"

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

    handle g_hndCurlWrapper = curl_wrapper_init();
    CHECK(g_hndCurlWrapper, -1, "Error with: %#x\n", g_hndCurlWrapper);

    //char* http_url = "http://192.168.0.32:8186/mktech_auto_update_app";
    char* http_url = "http://rdtest.myhiott.com:8081/upgrade/ftp_test/lifeiheng/mktech_auto_update_app";
    ret = curl_wrapper_StartDownload2Mem(g_hndCurlWrapper, http_url, NULL, NULL);
    CHECK(ret == 0, -1, "Error with: %#x\n", ret);

    unsigned char digest[64];
    memset(digest, 0, sizeof(digest));

    md5_filesum("./ntpc", digest);
    DBG("md5: %s\n", digest);

    int size = util_file_size("./ntpc");
    char* bb = malloc(size);
    util_file_read("./ntpc", bb, size);

    memset(digest, 0, sizeof(digest));
    md5_memsum(bb, size, digest);
    DBG("md5: %s\n", digest);

    CURL_STATUS_S stStatus;
    memset(&stStatus, 0, sizeof(stStatus));
    while (!test_exit)
    {
        ret = curl_wrapper_GetProgress(g_hndCurlWrapper, &stStatus);
        CHECK(ret == 0, -1, "Error with: %#x\n", ret);

        DBG("Progress: %d\n", stStatus.u32Progress);
        if (stStatus.u32Progress == 100)
        {
            break;
        }
        usleep(1*1000*1000);
    }

    printf("pu8Recv: %s\n", stStatus.pu8Recv);
    printf("u32RecvHeaderTotal: %d\n", stStatus.u32RecvHeaderTotal);
    printf("u32RecvBodyTotal: %d\n", stStatus.u32RecvBodyTotal);

    char* output = NULL;
    int len = 0;
    ret = curl_wrapper_Download2MemGet(g_hndCurlWrapper, &output, &len);
    CHECK(ret == 0, -1, "Error with: %#x\n", ret);

    memset(digest, 0, sizeof(digest));
    md5_memsum(output, len, digest);
    DBG("111md5: %s\n", digest);

    curl_wrapper_Stop(g_hndCurlWrapper);
    curl_wrapper_destroy(g_hndCurlWrapper);
    g_hndCurlWrapper = NULL;

    return 0;
}






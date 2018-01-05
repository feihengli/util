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
#include "../sal_mtd.h"

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

    const char* path = "uboot_hi18ev200_1.0.3.0.bin";
    FILE* fp = fopen(path, "r");
    CHECK(fp, -1, "Error with %#x: %s\n", fp, strerror(errno));

    struct stat stStat;
    ret = stat(path, &stStat);
    CHECK(ret == 0, -1, "Error with %#x: %s\n", ret, strerror(errno));

    int len = stStat.st_size;
    char* buf = malloc(len);
    CHECK(buf, -1, "Error with %#x: %s\n", buf, strerror(errno));

    int read_len = 0;
    while (read_len != len)
    {
        int tmp = fread(buf + read_len, 1, len-read_len, fp);
        CHECK(tmp >= 0, -1, "Error with %#x: %s\n", tmp, strerror(errno));

        read_len += tmp;
    }

/*
    char* mtd = "/dev/mtd0";

    ret = sal_mtd_erase(mtd);
    CHECK(ret == 0, -1, "Error with: %#x\n", ret);

    ret = sal_mtd_write(buf, len, mtd);
    CHECK(ret == 0, -1, "Error with: %#x\n", ret);

    ret = sal_mtd_verify(buf, len, mtd);
    CHECK(ret == 0, -1, "Error with: %#x\n", ret);
*/

    char* mtd0 = "/dev/mtd0";
    char flag[8];
    memset(flag, 0, sizeof(flag));
    strcpy(flag, "WORK");

    ret = sal_mtd_writeflag(mtd0, flag, strlen(flag));
    CHECK(ret == 0, -1, "Error with: %#x\n", ret);

    memset(flag, 0, sizeof(flag));
    ret = sal_mtd_readflag(mtd0, flag, sizeof(flag));
    CHECK(ret == 0, -1, "Error with: %#x\n", ret);

    DBG("flag: %s\n", flag);

    free(buf);
    return 0;
}







#include "sal_util.h"
#include "sal_debug.h"

static int util_time_clock(clockid_t clk_id, struct timeval* pTime)
{
    struct timespec stTime;
    memset(&stTime, 0, sizeof(struct timespec));
    memset(pTime, 0, sizeof(struct timeval));
    if (!clock_gettime(clk_id, &stTime))
    {
        pTime->tv_sec = stTime.tv_sec;
        pTime->tv_usec = stTime.tv_nsec/1000;
        return 0;
    }

    return -1;
}

int util_time_abs(struct timeval* pTime)
{
    CHECK(NULL != pTime, -1, "invalid parameter with: %#x\n", pTime);

    return util_time_clock(CLOCK_MONOTONIC, pTime);
}

int util_time_local(struct timeval* pTime)
{
    CHECK(NULL != pTime, -1, "invalid parameter with: %#x\n", pTime);

    return util_time_clock(CLOCK_REALTIME, pTime);
}

//单位毫秒
int util_time_sub(struct timeval* pStart, struct timeval* pEnd)
{
    CHECK(NULL != pStart, -1, "invalid parameter with: %#x\n", pStart);
    CHECK(NULL != pEnd, -1, "invalid parameter with: %#x\n", pEnd);

    int IntervalTime = (pEnd->tv_sec -pStart->tv_sec)*1000 + (pEnd->tv_usec - pStart->tv_usec)/1000;

    return IntervalTime;
}

//单位毫秒
int util_time_pass(struct timeval* previous)
{
    CHECK(NULL != previous, -1, "invalid parameter with: %#x\n", previous);

    struct timeval cur = {0, 0};
    util_time_abs(&cur);

    return util_time_sub(previous, &cur);
}

char* util_time_string(void)
{
    static char time[32] = "";
    struct timeval tv = {0, 0};
    gettimeofday(&tv, NULL);
    //strftime(time, sizeof(time), "%F %H:%M:%S", localtime(&tv.tv_sec));
    strftime(time, sizeof(time), "%Y-%m-%d %H:%M:%S", localtime(&tv.tv_sec));
    sprintf(time, "%s.%06ld", time, tv.tv_usec);

    return time;
}

int util_file_size(const char* path)
{
    CHECK(path, -1, "invalid parameter with: %#x\n", path);

    struct stat stStat;
    int ret = stat(path, &stStat);
    CHECK(ret == 0, -1, "error with %#x: %s\n", ret, strerror(errno));

    return stStat.st_size;
}

//读取文件
int util_file_read(const char* path, unsigned char* buf, int len)
{
    CHECK(path, -1, "invalid parameter with: %#x\n", path);
    
    unsigned int filesize = util_file_size(path);
    CHECK(filesize > 0, -1, "error with: %#x\n", filesize);
    
    //buffer过小
    CHECK(len >= filesize, -1, "buffer is too small with: %d.filesize: %d\n", len, filesize);
    
    FILE* fp = fopen(path, "r");
    CHECK(fp, -1, "error with %#x: %s\n", fp, strerror(errno));

    int read_len = 0;
    while (read_len < len && !feof(fp))
    {
        int tmp = fread(buf + read_len, 1, len-read_len, fp);
        CHECK(tmp >= 0, -1, "error with %#x: %s\n", tmp, strerror(errno));
        read_len += tmp;
    }
    
    fclose(fp);
    return read_len;
}

//写指定大小
int util_file_write(const char* path, unsigned char* buf, int len)
{
    CHECK(path, -1, "invalid parameter with: %#x\n", path);
    CHECK(buf, -1, "invalid parameter with: %#x\n", buf);
    CHECK(len > 0, -1, "invalid parameter with: %#x\n", len);

    FILE* fp = fopen(path, "w+");
    CHECK(fp, -1, "error with %#x: path[%s] %s\n", fp, path, strerror(errno));

    int written_len = 0;
    while (written_len != len)
    {
        int tmp = fwrite(buf + written_len, 1, len-written_len, fp);
        CHECK(tmp >= 0, -1, "error with %#x: %s\n", tmp, strerror(errno));
        written_len += tmp;
    }

    fclose(fp);
    sync();
    return 0;
}





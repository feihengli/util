#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>

#include "sal_debug.h"
#include "file_frame_source.h"

typedef int (*video_frame_cb)(int stream, char *frame, unsigned long len, int key, double pts);

typedef struct split_frame_s
{
    unsigned char buffer[1*1024*1024];
    unsigned char dst[1*1024*1024];
    unsigned char idr[1*1024*1024];
    int idr_len;
    unsigned char vps[128]; //only h265
    int vps_len;
    unsigned char sps[128];
    int sps_len;
    unsigned char pps[128];
    int pps_len;
    unsigned char sei[128];
    int sei_len;
    FILE* fp;
    int file_type; // 0->264, 1->265
    video_frame_cb frame_cb;
    double pts;

    int running;
    pthread_t pid;
}split_frame_s;

static split_frame_s g_arg;

int split_frame_264(unsigned char* src, int s_len, int* r_len, unsigned char* dst, int* len)
{
    *r_len = 0;
    *len = 0;
    int i = 0;
    int j = 0;
    int valid = 0;
    for (i = 0; i < s_len; i++)
    {
        if (i+3 < s_len && src[i] == 0 && src[i+1] == 0 && src[i+2] == 0 && src[i+3] == 1)
        {
            //printf("%d %02x %02x %02x %02x %02x %02x\n", __LINE__, src[i], src[i+1], src[i+2], src[i+3], src[i+4], src[i+5]);
            for (j = i+3; j < s_len; j++)
            {
                if (j+3 < s_len && src[j] == 0 && src[j+1] == 0 && src[j+2] == 0 && src[j+3] == 1)
                {
                    //printf("%d %02x %02x %02x %02x %02x %02x\n", __LINE__, src[j], src[j+1], src[j+2], src[j+3], src[j+4], src[j+5]);
                    valid = 1;
                    break;
                }
            }
            break;
        }
    }

    if (valid)
    {
        memcpy(dst, src+i, j-i);
        *len = j -i;
        *r_len = j;
    }

    return 0;
}

/*
src: input sources
s_len: input sources length
r_len: output handled length
dst: destination
len: destination length
*/
int split_frame_265(unsigned char* src, int s_len, int* r_len, unsigned char* dst, int* len)
{
    *r_len = 0;
    *len = 0;
    int i = 0;
    int j = 0;
    int valid = 0;
    for (i = 0; i < s_len; i++)
    {
        if (i+5 < s_len && src[i] == 0 && src[i+1] == 0 && src[i+2] == 0 && src[i+3] == 1 && src[i+5] == 1)
        {
            //printf("%d %02x %02x %02x %02x %02x %02x\n", __LINE__, src[i], src[i+1], src[i+2], src[i+3], src[i+4], src[i+5]);
            for (j = i+3; j < s_len; j++)
            {
                if (j+5 < s_len && src[j] == 0 && src[j+1] == 0 && src[j+2] == 0 && src[j+3] == 1 && src[j+5] == 1)
                {
                    //printf("%d %02x %02x %02x %02x %02x %02x\n", __LINE__, src[j], src[j+1], src[j+2], src[j+3], src[j+4], src[j+5]);
                    valid = 1;
                    break;
                }
            }
            break;
        }
    }

    if (valid)
    {
        memcpy(dst, &src[i], j-i);
        *len = j -i;
        *r_len = j;
    }

    return 0;
}

void* split_frame_proc(void* arg)
{
    int r_len = 0;
    int d_len = 0;
    int writable = 0;

    while (g_arg.running)
    {
        int n = fread(g_arg.buffer+writable, 1, sizeof(g_arg.buffer)-writable, g_arg.fp);
        if (n > 0)
        {
            writable += n;
        }

        if (writable > 0)
        {
            if (g_arg.file_type)
            {
                split_frame_265(g_arg.buffer, writable, &r_len, g_arg.dst, &d_len);
                if (r_len > 0 && d_len > 0)
                {
                    memmove(g_arg.buffer, g_arg.buffer+r_len, sizeof(g_arg.buffer)-r_len);
                    writable = sizeof(g_arg.buffer)-r_len;
                    printf("%d %02x %02x %02x %02x %02x %02x, len: %d\n", __LINE__, g_arg.dst[0], g_arg.dst[1], g_arg.dst[2], g_arg.dst[3], g_arg.dst[4], g_arg.dst[5], d_len);

                    if (g_arg.dst[4] == 0x40)
                    {
                        memset(g_arg.vps, 0, sizeof(g_arg.vps));
                        memcpy(g_arg.vps, g_arg.dst, d_len);
                        g_arg.vps_len = d_len;
                    }
                    if (g_arg.dst[4] == 0x42)
                    {
                        memset(g_arg.sps, 0, sizeof(g_arg.sps));
                        memcpy(g_arg.sps, g_arg.dst, d_len);
                        g_arg.sps_len = d_len;
                    }
                    if (g_arg.dst[4] == 0x44)
                    {
                        memset(g_arg.pps, 0, sizeof(g_arg.pps));
                        memcpy(g_arg.pps, g_arg.dst, d_len);
                        g_arg.pps_len = d_len;
                    }
                    if (g_arg.dst[4] == 0x4e)
                    {
                        memset(g_arg.sei, 0, sizeof(g_arg.sei));
                        memcpy(g_arg.sei, g_arg.dst, d_len);
                        g_arg.sei_len = d_len;
                    }
                    if (g_arg.dst[4] == 0x26)
                    {
                        memset(g_arg.idr, 0, sizeof(g_arg.idr));
                        memcpy(g_arg.idr, g_arg.dst, d_len);
                        g_arg.idr_len = d_len;
                    }
                    if (g_arg.frame_cb && g_arg.dst[4] == 0x26)
                    {
                        memset(g_arg.dst, 0, sizeof(g_arg.dst));
                        int idx = 0;

                        memcpy(g_arg.dst+idx, g_arg.vps, g_arg.vps_len);
                        idx += g_arg.vps_len;
                        printf("%d len: %d\n", __LINE__, idx);
                        memcpy(g_arg.dst+idx, g_arg.sps, g_arg.sps_len);
                        idx += g_arg.sps_len;
                        printf("%d len: %d\n", __LINE__, idx);
                        memcpy(g_arg.dst+idx, g_arg.pps, g_arg.pps_len);
                        idx += g_arg.pps_len;
                        printf("%d len: %d\n", __LINE__, idx);
/*
                        memcpy(g_arg.dst+idx, g_arg.sei, g_arg.sei_len);
                        idx += g_arg.sei_len;
                        printf("%d len: %d\n", __LINE__, idx);
*/
                        memcpy(g_arg.dst+idx, g_arg.idr, g_arg.idr_len);
                        idx += g_arg.idr_len;
                        printf("%d len: %d\n", __LINE__, idx);

                        g_arg.frame_cb(0, (char*)g_arg.dst, idx, 1, g_arg.pts);
                        //g_arg.frame_cb(1, (char*)g_arg.dst, idx, 1, g_arg.pts);
                        g_arg.pts += 40;
                    }
                    else if (g_arg.frame_cb && g_arg.dst[4] == 0x02)
                    {
                        g_arg.frame_cb(0, (char*)g_arg.dst, d_len, 0, g_arg.pts);
                        //g_arg.frame_cb(1, (char*)g_arg.dst, d_len, 0, g_arg.pts);
                        g_arg.pts += 40;
                    }
                }
            }
            else
            {
                split_frame_264(g_arg.buffer, writable, &r_len, g_arg.dst, &d_len);
                if (r_len > 0 && d_len > 0)
                {
                    memmove(g_arg.buffer, g_arg.buffer+r_len, sizeof(g_arg.buffer)-r_len);
                    writable = sizeof(g_arg.buffer)-r_len;
                    printf("%d %02x %02x %02x %02x %02x, len: %d\n", __LINE__, g_arg.dst[0], g_arg.dst[1], g_arg.dst[2], g_arg.dst[3], g_arg.dst[4], d_len);

                    if (g_arg.dst[4] == 0x67)
                    {
                        memset(g_arg.sps, 0, sizeof(g_arg.sps));
                        memcpy(g_arg.sps, g_arg.dst, d_len);
                        g_arg.sps_len = d_len;
                    }
                    if (g_arg.dst[4] == 0x68)
                    {
                        memset(g_arg.pps, 0, sizeof(g_arg.pps));
                        memcpy(g_arg.pps, g_arg.dst, d_len);
                        g_arg.pps_len = d_len;
                    }
                    if (g_arg.dst[4] == 0x06)
                    {
                        memset(g_arg.sei, 0, sizeof(g_arg.sei));
                        memcpy(g_arg.sei, g_arg.dst, d_len);
                        g_arg.sei_len = d_len;
                    }
                    if (g_arg.dst[4] == 0x65)
                    {
                        memset(g_arg.idr, 0, sizeof(g_arg.idr));
                        memcpy(g_arg.idr, g_arg.dst, d_len);
                        g_arg.idr_len = d_len;
                    }
                    if (g_arg.frame_cb && g_arg.dst[4] == 0x65)
                    {
                        memset(g_arg.dst, 0, sizeof(g_arg.dst));
                        int idx = 0;

                        memcpy(g_arg.dst+idx, g_arg.sps, g_arg.sps_len);
                        idx += g_arg.sps_len;
                        printf("%d len: %d\n", __LINE__, idx);
                        memcpy(g_arg.dst+idx, g_arg.pps, g_arg.pps_len);
                        idx += g_arg.pps_len;
                        printf("%d len: %d\n", __LINE__, idx);
                        memcpy(g_arg.dst+idx, g_arg.sei, g_arg.sei_len);
                        idx += g_arg.sei_len;
                        printf("%d len: %d\n", __LINE__, idx);
                        memcpy(g_arg.dst+idx, g_arg.idr, g_arg.idr_len);
                        idx += g_arg.idr_len;
                        printf("%d len: %d\n", __LINE__, idx);

                        g_arg.frame_cb(0, (char*)g_arg.dst, idx, 1, g_arg.pts);
                        //g_arg.frame_cb(1, (char*)g_arg.dst, idx, 1, g_arg.pts);
                        g_arg.pts += 40;
                    }
                    else if (g_arg.frame_cb && g_arg.dst[4] == 0x61)
                    {
                        g_arg.frame_cb(0, (char*)g_arg.dst, d_len, 0, g_arg.pts);
                        //g_arg.frame_cb(1, (char*)g_arg.dst, d_len, 0, g_arg.pts);
                        g_arg.pts += 40;
                    }
                }
            }
        }

        if (feof(g_arg.fp))
        {
            fseek(g_arg.fp , 0L, SEEK_SET);
        }

        usleep(40*1000);
    }

    fclose(g_arg.fp);

    return 0;
}

int split_frame_init(const char* path, video_frame_cb frame_cb)
{
    g_arg.fp = fopen(path, "r");
    assert(g_arg.fp != NULL);

    g_arg.file_type = strstr(path, "265") ? 1 : 0;
    g_arg.frame_cb = frame_cb;

    g_arg.running = 1;
    pthread_create(&g_arg.pid, NULL, split_frame_proc, NULL);

    return 0;
}

int split_frame_exit()
{
    if (g_arg.running)
    {
        g_arg.running = 0;
        pthread_join(g_arg.pid, NULL);
    }

    if (g_arg.fp)
    {
        fclose(g_arg.fp);
    }

    return 0;
}




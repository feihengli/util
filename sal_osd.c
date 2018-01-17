#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/prctl.h>

#include "sal_overlay.h"
#include "sal_util.h"
#include "sal_debug.h"
#include "sal_bitmap.h"
#include "sal_osd.h"
#include "sal_config.h"
#include "sal_statistics.h"
#include "sal_isp.h"
#include "sal_lbr.h"
#include "sal_af.h"

typedef enum OSD_E
{
    OSD_TIME = 0,
    OSD_TITLE = 1,
    OSD_BUTT,
}OSD_E;

typedef enum OSD_STREAM_TYPE_E
{
    OSD_STREAM_TYPE_MAIN = 0,
    OSD_STREAM_TYPE_SUB = 1,
    OSD_STREAM_TYPE_JPEG = 2,
    OSD_STREAM_TYPE_BUTT,
}OSD_STREAM_TYPE_E;

typedef struct sal_osd_args
{
    struct timeval current;

    int language; // 0 中文 1 英文
    sal_osd_item_s item[OSD_STREAM_TYPE_BUTT][OSD_BUTT];
    sal_overlay_s param[OSD_STREAM_TYPE_BUTT][OSD_BUTT];
    const char* path_hzk;
    const char* path_asc;
    int gap; //行间距以及四个角的位置偏移

    int running;
    pthread_t pid;
    pthread_mutex_t mutex;
}sal_osd_args;

static sal_osd_args* g_osd_args = NULL;

static const char *week_day_ch[8]=
{
    "\xD0\xC7\xC6\xDA\xC8\xD5",
    "\xD0\xC7\xC6\xDA\xD2\xBB",
    "\xD0\xC7\xC6\xDA\xB6\xFE",
    "\xD0\xC7\xC6\xDA\xC8\xFD",
    "\xD0\xC7\xC6\xDA\xCB\xC4",
    "\xD0\xC7\xC6\xDA\xCE\xE5",
    "\xD0\xC7\xC6\xDA\xC1\xF9",
    "",
};

static const char *week_day_en[8]=
{
    "Sunday",
    "Monday",
    "Tuesday",
    "Wednesday",
    "Thursday",
    "Friday",
    "Saturday",
    "",
};

/*
static const char *fmt[]=
{
    "yyyy-mm-dd hh:mm:ss",
    "yyyy/mm/dd hh:mm:ss",
    "yy-mm-dd hh:mm:ss",
    "yy/mm/dd hh:mm:ss",
    "hh:mm:ss dd/mm/yyyy",
    "hh:mm:ss dd-mm-yyyy",
    "hh:mm:ss mm/dd/yyyy",
    "hh:mm:ss mm-dd-yyyy",
    NULL
};
*/

static const char *osd_get_weekday(int index)
{
    const char* ret = NULL;
    if (g_osd_args->language)
    {
        ret = week_day_en[index];
    }
    else
    {
        ret = week_day_ch[index];
    }

    return ret;
}

static int osd_time_string(int fmt, char* output)
{
    struct timeval tv;
    struct tm *ptm;

    util_time_local(&tv);
    ptm = localtime(&tv.tv_sec);

    switch(fmt)
    {
        case 0:
            sprintf(output, "%04d-%02d-%02d %s %02d:%02d:%02d",
                (1900 + ptm->tm_year),
                (1 + ptm->tm_mon),
                ptm->tm_mday,
                osd_get_weekday(ptm->tm_wday),
                ptm->tm_hour,
                ptm->tm_min,
                ptm->tm_sec);
            break;
        case 1:
            sprintf(output, "%04d/%02d/%02d %s %02d:%02d:%02d",
                (1900 + ptm->tm_year),
                (1 + ptm->tm_mon),
                ptm->tm_mday,
                osd_get_weekday(ptm->tm_wday),
                ptm->tm_hour,
                ptm->tm_min,
                ptm->tm_sec);
            break;
        case 2:
            sprintf(output, "%02d-%02d-%02d %s %02d:%02d:%02d",
                (1900 + ptm->tm_year - 2000),
                (1 + ptm->tm_mon),
                ptm->tm_mday,
                osd_get_weekday(ptm->tm_wday),
                ptm->tm_hour,
                ptm->tm_min,
                ptm->tm_sec);
            break;
        case 3:
            sprintf(output, "%02d/%02d/%02d %s %02d:%02d:%02d",
                (1900 + ptm->tm_year - 2000),
                (1 + ptm->tm_mon),
                ptm->tm_mday,
                osd_get_weekday(ptm->tm_wday),
                ptm->tm_hour,
                ptm->tm_min,
                ptm->tm_sec);
            break;
        case 4:
            sprintf(output, "%02d:%02d:%02d %02d/%02d/%04d %s",
                ptm->tm_hour,
                ptm->tm_min,
                ptm->tm_sec,
                ptm->tm_mday,
                (1 + ptm->tm_mon),
                (1900 + ptm->tm_year),
                osd_get_weekday(ptm->tm_wday));
            break;
        case 5:
            sprintf(output, "%02d:%02d:%02d %02d-%02d-%04d %s",
                ptm->tm_hour,
                ptm->tm_min,
                ptm->tm_sec,
                ptm->tm_mday,
                (1 + ptm->tm_mon),
                (1900 + ptm->tm_year),
                osd_get_weekday(ptm->tm_wday));
            break;

        case 6:
            sprintf(output, "%02d:%02d:%02d %02d/%02d/%04d %s",
                ptm->tm_hour,
                ptm->tm_min,
                ptm->tm_sec,
                (1 + ptm->tm_mon),
                ptm->tm_mday,
                (1900 + ptm->tm_year),
                osd_get_weekday(ptm->tm_wday));
            break;
        case 7:
            sprintf(output, "%02d:%02d:%02d %02d-%02d-%04d %s",
                ptm->tm_hour,
                ptm->tm_min,
                ptm->tm_sec,
                (1 + ptm->tm_mon),
                ptm->tm_mday,
                (1900 + ptm->tm_year),
                osd_get_weekday(ptm->tm_wday));
            break;

        default:
            sprintf(output, "%04d-%02d-%02d %s %02d:%02d:%02d",
                (1900 + ptm->tm_year),
                (1 + ptm->tm_mon),
                ptm->tm_mday,
                osd_get_weekday(ptm->tm_wday),
                ptm->tm_hour,
                ptm->tm_min,
                ptm->tm_sec);
    }

    return 0;
}

static int osd_char2short(char* src, int row, int col, unsigned short* dst)
{
    int i = 0;
    int j = 0;

    for (i = 0; i < row; i++)
    {
        for (j = 0; j < col; j++)
        {
            int idx = i*col+j;
            char ch = src[idx];
            if (ch == FORE_GROUND)
            {
                dst[idx] = RGB_VALUE_WHITE;
            }
            else if (ch == BACK_GROUND)
            {
                dst[idx] = RGB_VALUE_BG;
            }
            else if (ch == FONT_EDGE)
            {
                dst[idx] = RGB_VALUE_BLACK;
            }
        }
    }

    return 0;
}

static int osd_get_factor(int width, int height)
{
    int factor = 1;
    if(width*height >= 4000*3000)
        factor = 6;
    else if(width*height >= 3000*2000)
        factor = 5;
    else if(width*height >= 2048*1536)
        factor = 4;
    else if(width*height >= 1920*1080)
        factor = 3;
    else if(width*height >= 1280*720)
        factor = 2;

    return factor;
}

static char* osd_resolution_get(int i, int j)
{
    static char resolution[64];
    memset(resolution, 0, sizeof(resolution));

    // 只对实时码率叠加
    if (i <= OSD_STREAM_TYPE_SUB)
    {
        if (!strcmp(g_config.video[i].resolution, "1920x1080"))
        {
            sprintf(resolution, "1080P");
        }
        else if (!strcmp(g_config.video[i].resolution, "1280x720"))
        {
            sprintf(resolution, "720P");
        }
        else if (!strcmp(g_config.video[i].resolution, "640x360"))
        {
            sprintf(resolution, "360P");
        }
        else
        {
            strcpy(resolution, g_config.video[i].resolution);
        }
    }

    return resolution;
}

static int osd_update_content(int i, int j)
{
    int ret = -1;

    if (j == OSD_TIME) // update time string
    {
        memset(g_osd_args->item[i][j].buffer, 0, sizeof(g_osd_args->item[i][j].buffer));
        ret = osd_time_string(g_osd_args->item[i][j].tmformat, g_osd_args->item[i][j].buffer);
        CHECK(ret == 0, -1, "Error with %#x.\n", ret);
    }
    else if (i <= OSD_STREAM_TYPE_SUB && j == OSD_TITLE)
    {
        int add_resolution = 0;
        int add_bitrate = 0;
        if (g_osd_args->item[i][j].type == TITLE_ADD_RESOLUTION
            || g_osd_args->item[i][j].type == TITLE_ADD_ALL)
        {
            add_resolution = 1;
        }
        if (g_osd_args->item[i][j].type == TITLE_ADD_BITRATE
            || g_osd_args->item[i][j].type == TITLE_ADD_ALL)
        {
            add_bitrate = 1;
        }

        memset(g_osd_args->item[i][j].buffer, 0, sizeof(g_osd_args->item[i][j].buffer));
        strcpy(g_osd_args->item[i][j].buffer, g_osd_args->item[i][j].buffer_bak);
        if (add_resolution)
        {
            strcat(g_osd_args->item[i][j].buffer, " ");
            strcat(g_osd_args->item[i][j].buffer, osd_resolution_get(i, j));
        }

        if (add_bitrate)
        {
            strcat(g_osd_args->item[i][j].buffer, " ");
            strcat(g_osd_args->item[i][j].buffer, "xxxK/s");
        }
    }

    return 0;
}

static int osd_str_replace(char* input, int i, int j)
{
    int ret = -1;
    const char* old[] = {
                        "xxxK/s",
                        "ig[xxxx]",
                        "dg[xxxx]",
                        "ag[xxxx]",
                        "t[xxx]",
                        "luma[xxx]",
                        "v[xxxx]",
                        "vp[xxx_xxx_xxx_xxx]",
                        "ct[xxxx]",
                        "f[xxxxx]"
                        };
    char temp[32][32];
    memset(temp, 0, sizeof(temp));

    sal_exposure_info_s exposure_info;
    memset(&exposure_info, 0, sizeof(exposure_info));
    //ret = sal_isp_exposure_info_get(&exposure_info);
    //CHECK(ret == 0, -1, "Error with %d.\n", ret);

    int af_value = 0;
    ret = sal_af_get_fv(&af_value);
    CHECK(ret == 0, -1, "Error with %d.\n", ret);

    unsigned int k = 0;
    sprintf(temp[k++], "%3dK/s", sal_statistics_bitrate_get(i)/8);
    sprintf(temp[k++], "ig[%04.1f]", exposure_info.ISPGain/1024.0);
    sprintf(temp[k++], "dg[%04.1f]", exposure_info.DGain/1024.0);
    sprintf(temp[k++], "ag[%04.1f]", exposure_info.AGain/1024.0);
    sprintf(temp[k++], "t[%03.0f]", (exposure_info.ExpTime/1000.0 + 0.5));
    sprintf(temp[k++], "luma[%03d]", (exposure_info.AveLuma));

    if (g_config.lbr[i].enable)
    {
        ret = sal_lbr_get_str(temp[k], sizeof(temp[k]));
        CHECK(ret == 0, -1, "Error with %d.\n", ret);
        k++;

        ret = sal_lbr_get_vpstr(temp[k], sizeof(temp[k]));
        CHECK(ret == 0, -1, "Error with %d.\n", ret);
        k++;
    }

    sprintf(temp[k++], "ct[%04d]", (exposure_info.ColorTemp));
    sprintf(temp[k++], "f[%05d]", af_value);

    for (k = 0; k < sizeof(old)/sizeof(*old); k++)
    {
        if (strlen(old[k]) != strlen(temp[k]))
        {
            continue;
        }
        char* find = strstr(input, old[k]);
        if (find)
        {
            memcpy(find, temp[k], strlen(temp[k]));
        }
    }

    return 0;
}

static int osd_update_cfg(int i, int j, int reset)
{
    int ret = -1;
    g_osd_args->param[i][j].stream_id = i;
    g_osd_args->param[i][j].handle = i*OSD_BUTT+j;

    result_s result;
    memset(&result, 0, sizeof(result));
    int window_w = (i < 2) ? g_config.video[i].width : g_config.jpeg.widht;
    int window_h = (i < 2) ? g_config.video[i].height : g_config.jpeg.height;
    int font_size = osd_get_factor(window_w, window_h);

    ret = bitmap_alloc((const unsigned char*)g_osd_args->item[i][j].buffer, font_size, 1, g_osd_args->gap, &result);
    CHECK(ret == 0, -1, "Error with %#x.\n", ret);

    g_osd_args->param[i][j].width = result.col;
    g_osd_args->param[i][j].height = result.row;
    g_osd_args->param[i][j].factor = font_size;

    //与边界的空闲，为一个字大小
    int offset_h = g_osd_args->gap*2; // 水平方向偏移量
    int offset_v = g_osd_args->gap*2; // 垂直方向偏移量
    if (g_osd_args->item[i][j].location == OSD_LEFT_TOP)
    {
        g_osd_args->param[i][j].posx = offset_h;
        g_osd_args->param[i][j].posy = offset_v;
    }
    else if (g_osd_args->item[i][j].location == OSD_LEFT_BOTTOM)
    {
        g_osd_args->param[i][j].posx = offset_h;
        g_osd_args->param[i][j].posy = window_h - g_osd_args->param[i][j].height - offset_v;
    }
    else if (g_osd_args->item[i][j].location == OSD_RIGHT_TOP)
    {
        g_osd_args->param[i][j].posx = window_w - g_osd_args->param[i][j].width - offset_h;
        g_osd_args->param[i][j].posy = offset_v;
    }
    else if (g_osd_args->item[i][j].location == OSD_RIGHT_BOTTOM)
    {
        g_osd_args->param[i][j].posx = window_w - g_osd_args->param[i][j].width - offset_h;
        g_osd_args->param[i][j].posy = window_h - g_osd_args->param[i][j].height - offset_v;
    }

    if (reset)
    {
        if (g_osd_args->param[i][j].bitmap_data)
        {
            ret = sal_overlay_destory(&g_osd_args->param[i][j]);
            CHECK(ret == 0, -1, "Error with %#x.\n", ret);
        }
    }

/*
    DBG("stream_id: %d\n", g_osd_args->param[i][j].stream_id);
    DBG("handle: %d\n", g_osd_args->param[i][j].handle);
    DBG("posx: %d\n", g_osd_args->param[i][j].posx);
    DBG("posy: %d\n", g_osd_args->param[i][j].posy);
    DBG("width: %d\n", g_osd_args->param[i][j].width);
    DBG("height: %d\n", g_osd_args->param[i][j].height);
    DBG("factor: %d\n", g_osd_args->param[i][j].factor);
    DBG("bitmap_data: %#x\n", g_osd_args->param[i][j].bitmap_data);
*/

    if (!g_osd_args->param[i][j].bitmap_data)
    {
        ret = sal_overlay_create(&g_osd_args->param[i][j]);
        CHECK(ret == 0, -1, "Error with %#x.\n", ret);
    }

    ret = osd_char2short(result.bit_map, result.row, result.col, (unsigned short*)g_osd_args->param[i][j].bitmap_data);
    CHECK(ret == 0, -1, "Error with %#x.\n", ret);

    ret = bitmap_free(&result);
    CHECK(ret == 0, -1, "Error with %#x.\n", ret);

    return 0;
}

static void* osd_proc(void* args)
{
    prctl(PR_SET_NAME, __FUNCTION__);

    int ret = -1;
    unsigned int i = 0;
    unsigned int j = 0;

    char last_content[OSD_STREAM_TYPE_BUTT][OSD_BUTT][512];
    memset(last_content, 0, sizeof(last_content));

    while (g_osd_args->running)
    {
        pthread_mutex_lock(&g_osd_args->mutex);
        util_time_abs(&g_osd_args->current);

        for (i = OSD_STREAM_TYPE_MAIN; i < OSD_STREAM_TYPE_BUTT; i++)
        {
            for (j = OSD_TIME; j < OSD_BUTT; j++)
            {
                if (g_osd_args->item[i][j].enable)
                {
                    ret = osd_update_content(i, j);
                    CHECK(ret == 0, NULL, "Error with %#x.\n", ret);

                    if (i <= OSD_STREAM_TYPE_SUB)
                    {
                        ret = osd_str_replace(g_osd_args->item[i][j].buffer, i, j);
                        CHECK(ret == 0, NULL, "Error with %#x.\n", ret);
                    }

                    if (strcmp(last_content[i][j], g_osd_args->item[i][j].buffer))
                    {
                        int reset = (strlen(last_content[i][j]) != strlen(g_osd_args->item[i][j].buffer)) ? 1 : 0;
                        ret = osd_update_cfg(i, j, reset);
                        CHECK(ret == 0, NULL, "Error with %#x.\n", ret);
                    }

                    memset(last_content[i][j], 0, sizeof(last_content[i][j]));
                    strcpy(last_content[i][j], g_osd_args->item[i][j].buffer);

                    ret = sal_overlay_refresh_bitmap(&g_osd_args->param[i][j]);
                    CHECK(ret == 0, NULL, "Error with %#x.\n", ret);
                }
            }
        }

        pthread_mutex_unlock(&g_osd_args->mutex);
        usleep(200*1000);
     }

    return NULL;
}

static int osd_load_cfg(char* path, int i, int j)
{
    if (access(path, F_OK))
    {
        return 0;
    }

    int ret = -1;
    int k = 0;
    int size = util_file_size(path);
    if (size > 0)
    {
        char* buffer = malloc(size);
        memset(buffer, 0, size);

        ret = util_file_read(path, buffer, size);
        CHECK(ret == 0, -1, "Error with %#x.\n", ret);

        const char* cmd[] = {"type:", "location:", "content:"};
        char* find = strstr(buffer, cmd[k]);
        if (find)
        {
            g_osd_args->item[i][j].type = (OSD_TITLE_TYPE_E)strtold(find+strlen(cmd[k++]), NULL);
        }
        find = strstr(buffer, cmd[k]);
        if (find)
        {
            g_osd_args->item[i][j].location = (OSD_LOCATION_E)strtold(find+strlen(cmd[k++]), NULL);
        }
        find = strstr(buffer, cmd[k]);
        if (find)
        {
            memset(g_osd_args->item[i][j].buffer, 0, sizeof(g_osd_args->item[i][j].buffer));
            char* begin = find+strlen(cmd[k]);
            memcpy(g_osd_args->item[i][j].buffer, begin, strlen(begin));
            k++;
        }

        free(buffer);
    }

    return 0;
}

int sal_osd_init()
{
    CHECK(NULL == g_osd_args, -1, "reinit error, please uninit first.\n");

    int ret = -1;
    unsigned int i = 0;
    unsigned int j = 0;
    g_osd_args = (sal_osd_args*)malloc(sizeof(sal_osd_args));
    CHECK(NULL != g_osd_args, -1, "malloc %d bytes failed.\n", sizeof(sal_osd_args));

    memset(g_osd_args, 0, sizeof(sal_osd_args));
    pthread_mutex_init(&g_osd_args->mutex, NULL);

    g_osd_args->path_hzk = "gbk.dzk";
    g_osd_args->path_asc = "asc16.dzk";
    g_osd_args->gap = 8;

    ret = bitmap_create(g_osd_args->path_hzk, 1, g_osd_args->path_asc);
    CHECK(ret == 0, -1, "Error with %d.\n", ret);

    g_osd_args->language = 0; // 中文

    for (i = OSD_STREAM_TYPE_MAIN; i < OSD_STREAM_TYPE_BUTT; i++)
    {
        for (j = OSD_TIME; j < OSD_BUTT; j++)
        {
            g_osd_args->item[i][j].enable = (i <= OSD_STREAM_TYPE_SUB) ? g_config.video[i].enable : g_config.jpeg.enable;

            if (j == OSD_TIME)
            {
                g_osd_args->item[i][j].tmformat = 0;
                g_osd_args->item[i][j].location = OSD_LEFT_TOP;
            }
            else if (j == OSD_TITLE)
            {
                g_osd_args->item[i][j].location = OSD_RIGHT_BOTTOM;
                g_osd_args->item[i][j].type = TITLE_ADD_ALL;
                memset(g_osd_args->item[i][j].buffer, 0, sizeof(g_osd_args->item[i][j].buffer));
                strcpy(g_osd_args->item[i][j].buffer, "ipcam");

                ret = osd_load_cfg("/etc/title.cfg", i, j);
                CHECK(ret == 0, -1, "Error with %#x.\n", ret);

                memset(g_osd_args->item[i][j].buffer_bak, 0, sizeof(g_osd_args->item[i][j].buffer_bak));
                strcpy(g_osd_args->item[i][j].buffer_bak, g_osd_args->item[i][j].buffer);
            }
        }
    }

    g_osd_args->running = 1;
    ret = pthread_create(&g_osd_args->pid, NULL, osd_proc, NULL);
    CHECK(ret == 0, -1, "Error with %s.\n", strerror(errno));

    return 0;
}

int sal_osd_exit()
{
    CHECK(NULL != g_osd_args, -1,"moudle is not inited.\n");

    int ret = -1;
    unsigned int i = 0;
    unsigned int j = 0;

    if (g_osd_args->running)
    {
        g_osd_args->running = 0;
        pthread_join(g_osd_args->pid, NULL);
    }

    for (i = OSD_STREAM_TYPE_MAIN; i < OSD_STREAM_TYPE_BUTT; i++)
    {
        for (j = OSD_TIME; j < OSD_BUTT; j++)
        {
            if (g_osd_args->param[i][j].bitmap_data)
            {
                ret = sal_overlay_destory(&g_osd_args->param[i][j]);
                CHECK(ret == 0, -1, "Error with %#x.\n", ret);
            }
        }
    }

    ret = bitmap_destroy();
    CHECK(ret == 0, -1, "Error with %d.\n", ret);

    pthread_mutex_destroy(&g_osd_args->mutex);
    free(g_osd_args);
    g_osd_args = NULL;

    return 0;
}



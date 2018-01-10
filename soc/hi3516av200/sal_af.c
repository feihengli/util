#include "sal_af.h"
#include "sal_debug.h"
#include "hi_comm.h"

//#include "an41908_ctrl.h"

#define PIPE_PATH          "/tmp/myfifo" //共享的文件名
#define BLEND_SHIFT 6
#define ALPHA 64 // 1
#define BELTA 54 // 0.85
#define AF_BLOCK_8X8

#ifdef AF_BLOCK_8X8
static int AFWeight[8][8] =
{
{1,1,1,1,1,1,1,1,},
{1,2,2,2,2,2,2,1,},
{1,2,16,16,16,16,2,1,},
{1,2,16,32,32,16,2,1,},
{1,2,16,32,32,16,2,1,},
{1,2,16,16,16,16,2,1,},
{1,2,2,2,2,2,2,1,},
{1,1,1,1,1,1,1,1,},
};

static ISP_FOCUS_STATISTICS_CFG_S stFocusCfg =
{

    {HI_TRUE, 8, 8, 1920, 1080, ISP_AF_STA_PEAK, ISP_AF_STA_SUM_NORM, {HI_FALSE, 0, 0, 0, 0}, ISP_AF_STATISTICS_YUV, {HI_FALSE, HI_FALSE, 0, 0, BAYER_RGGB},
        {HI_TRUE, 0x9bff}, 0xf0
    },
    {
        HI_TRUE, {HI_TRUE, HI_TRUE, HI_TRUE}, {188, 414, -330, 486, -461, 400, -328}, {7, 0, 3, 1}, {HI_TRUE, 0, 255, 0, 220, 8, 14}, {127, 12, 2047}
    },
    {
        HI_FALSE, {HI_TRUE, HI_TRUE, HI_FALSE}, {200, 200, -110, 461, -415, 0, 0}, {6, 0, 1, 0}, {HI_FALSE, 0, 0, 0, 0, 0, 0}, {15, 12, 2047}
    },

    {
        {20, 16, 0, -16, -20}, {HI_TRUE, 0, 255, 0, 220, 8, 14}, {38, 12, 1800}
    },

    {
        {-12, -24, 0, 24, 12}, {HI_TRUE, 0, 255, 0, 220, 8, 14}, {15, 12, 2047}
    },
    {
        4, {0, 0}, {1, 1}, 0
    },

};

#else

static int AFWeight[15][17] =
{
{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
{1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
{1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
{1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
{1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
{1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
{1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
{1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
{1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
{1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
{1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
{1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
{1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
{1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
};

static ISP_FOCUS_STATISTICS_CFG_S stFocusCfg =
{

    {HI_TRUE, 17, 15, 1920, 1080, ISP_AF_STA_PEAK, ISP_AF_STA_SUM_NORM, {HI_FALSE, 0, 0, 0, 0}, ISP_AF_STATISTICS_YUV, {HI_FALSE, HI_FALSE, 0, 0, BAYER_RGGB},
        {HI_TRUE, 0x9bff}, 0xf0
    },
    {
        HI_TRUE, {HI_TRUE, HI_TRUE, HI_TRUE}, {188, 414, -330, 486, -461, 400, -328}, {7, 0, 3, 1}, {HI_TRUE, 0, 255, 0, 220, 8, 14}, {127, 12, 2047}
    },
    {
        HI_FALSE, {HI_TRUE, HI_TRUE, HI_FALSE}, {200, 200, -110, 461, -415, 0, 0}, {6, 0, 1, 0}, {HI_FALSE, 0, 0, 0, 0, 0, 0}, {15, 12, 2047}
    },

    {
        {20, 16, 0, -16, -20}, {HI_TRUE, 0, 255, 0, 220, 8, 14}, {38, 12, 1800}
    },

    {
        {-12, -24, 0, 24, 12}, {HI_TRUE, 0, 255, 0, 220, 8, 14}, {15, 12, 2047}
    },
    {
        4, {0, 0}, {1, 1}, 0
    },

};


#endif

typedef struct sal_af_args
{
    unsigned int fv1[AF_ZONE_ROW*AF_ZONE_COLUMN];
    unsigned int fv2[AF_ZONE_ROW*AF_ZONE_COLUMN];

    //0: fv1, 1: fv2
    int sum_fv[2];
    int frame_cnt[2];
    int fv_cnt[2];

    int pipe_fd;
    int zoom_enable;
    int focus_enable;

    int running;
    pthread_t tid;
    pthread_mutex_t mutex;
}sal_af_args;

static sal_af_args* g_af_args = NULL;
/*
static unsigned char d_zoomtele[7] = {0xFF, 0x01, 0x00, 0x20, 0x40, 0x40, 0xA0};
static unsigned char d_zoomwide[7] = {0xFF, 0x01, 0x00, 0x40, 0x40, 0x40, 0xC0};
static unsigned char d_focusnear[7] ={0xFF, 0x01, 0x01, 0x0, 0x0, 0x0, 0x01};
static unsigned char d_focusfar[7] = {0xFF, 0x01, 0x0, 0x80, 0x0, 0x0, 0x80};
static unsigned char d_stop[7] = {0xFF, 0x01, 0x00, 0x0, 0x0, 0x00, 0x01};
static unsigned char d_callpreset[7] = {0xFF, 0x01, 0x0, 0x07, 0x0, 0x0, 0x07};
*/
static int af_get_sum(unsigned int* fv, int col, int row)
{
    static int max = 0;
    int i = 0, j = 0;
    int sum = 0;

    for (i = 0; i < row; ++i)
    {
        for (j = 0; j< col; ++j)
        {
            int idx = i*col+j;
            sum += fv[idx];
            //printf("%6u ", fv[idx]);
        }
        //printf("\n");
    }

    if (max < sum)
    {
        max = sum;
    }
    //printf("---------------rowxcol[%dx%d] max[%d] sum[%d]--------------------------------\n", row, col, max, sum);


    return sum;
}

/*
static int af_map_iso(int iso)
{
    int j, i = (iso >= 200);
    i += ( (iso >= (200 << 1)) + (iso >= (400 << 1)) + (iso >= (400 << 2)) + (iso >= (400 << 3)) + (iso >= (400 << 4)) );
    i += ( (iso >= (400 << 5)) + (iso >= (400 << 6)) + (iso >= (400 << 7)) + (iso >= (400 << 8)) + (iso >= (400 << 9)) );
    j = ( (iso > (112 << i)) + (iso > (125 << i)) + (iso > (141 << i)) + (iso >(158 << i)) + (iso > (178 << i)) );
    return (i * 6 + j + (iso >= 25) + (iso >= 50) + (iso >= 100));
}
*/

/*
static int af_check_fmt(char* cmd, int len)
{
    CHECK(len >= 7, HI_FAILURE, "invalid args: len: %d\n", len);

    int ret = -1;
    if (cmd[0] == 0xff && cmd[1] == 0x01)
    {
        char checksum = cmd[1]+cmd[2]+cmd[3]+cmd[4]+cmd[5];
        if (checksum == cmd[6])
        {
            ret = 0;
        }
    }

    return ret;
}
*/

/*
static int af_proc_pelcod(char* cmd, int len)
{
    //DBG("cmd: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], cmd[6]);
    int processed = 7;
    if (af_check_fmt(cmd, len))
    {
        processed = 1;
        DBG("Warning: pelcod format is not correct, discard 1 byte.\n");
        return processed;
    }

    int prefix_len = 4;
    if (!memcmp(cmd, d_zoomtele, prefix_len))
    {
        //DBG("d_zoomtele.\n");
        g_af_args->zoom_enable = 1;
        an41908_zoomtele_focusfar_start();
        //an41908_zoomtele_start();
    }
    else if (!memcmp(cmd, d_zoomwide, prefix_len))
    {
        //DBG("d_zoomwide.\n");
        g_af_args->zoom_enable = 1;
        an41908_zoomwide_focusnear_start();
        //an41908_zoomwide_start();
    }
    else if (!memcmp(cmd, d_focusnear, prefix_len))
    {
        //DBG("d_focusnear.\n");
        g_af_args->focus_enable = 1;
        an41908_focusnear_start();
    }
    else if (!memcmp(cmd, d_focusfar, prefix_len))
    {
        //DBG("d_focusfar.\n");
        g_af_args->focus_enable = 1;
        an41908_focusfar_start();
    }
    else if (!memcmp(cmd, d_stop, prefix_len))
    {
        //DBG("d_stop.\n");
        if (g_af_args->zoom_enable)
        {
            g_af_args->zoom_enable = 0;
            an41908_zoom_focus_stop();
            //an41908_zoom_stop();
        }
        if (g_af_args->focus_enable)
        {
            g_af_args->focus_enable = 0;
            an41908_focus_stop();
        }
    }
    else if (!memcmp(cmd, d_callpreset, prefix_len))
    {
        int preset = cmd[4] + cmd[5];
        //DBG("d_callpreset: %d.\n", preset);
        an41908_call_preset(preset);
    }
    else
    {
        DBG("Unknow cmd: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], cmd[6]);
    }

    return processed;
}

static void* af_proc(void* arg)
{
    THREAD_REGISTER();
    HI_S32 s32Ret = -1;
    char buffer[512];
    memset(buffer, 0, sizeof(buffer));
    char cmd[7]; // pelco d
    memset(cmd, 0, sizeof(cmd));

    unsigned int offset = 0;

    while (g_af_args->running)
    {
        pthread_mutex_lock(&g_af_args->mutex);

        s32Ret = read(g_af_args->pipe_fd, buffer+offset, sizeof(buffer)-offset);
        if (s32Ret > 0)
        {
            offset += s32Ret;
        }
        else
        {
            DBG("Error with %s.\n", strerror(errno));
            if (errno != EAGAIN)
            {
                DBG("exit thread.\n");
                return NULL;
            }
        }

        while (offset >= sizeof(cmd))
        {
            memcpy(cmd, buffer, sizeof(cmd));

            int bytes = af_proc_pelcod(cmd, sizeof(cmd));
            if (bytes > 0)
            {
                memmove(buffer, buffer+bytes, sizeof(buffer)-bytes);
                memset(buffer+sizeof(buffer)-bytes, 0, bytes);
                offset -= bytes;
            }
        }

        pthread_mutex_unlock(&g_af_args->mutex);
        usleep(10*1000);
    }

    THREAD_UNREGISTER();
    return NULL;
}

static HI_S32 af_stop()
{
    HI_S32 s32Ret = -1;

    s32Ret = write(g_af_args->pipe_fd, d_stop, sizeof(d_stop));
    CHECK(s32Ret == sizeof(d_stop), HI_FAILURE, "Error with %s.\n", strerror(errno));

    return 0;
}
*/

static int af_init(void)
{
    CHECK(NULL == g_af_args, HI_FAILURE, "reinit error, please exit first.\n");

    g_af_args = (sal_af_args*)malloc(sizeof(sal_af_args));
    CHECK(g_af_args != NULL, HI_FAILURE, "malloc %d bytes failed.\n", sizeof(sal_af_args));
    memset(g_af_args, 0, sizeof(sal_af_args));

/*
    if (access(PIPE_PATH, F_OK))
    {
        //unlink(PIPE_PATH);
        s32Ret = mkfifo(PIPE_PATH, 0666);
        CHECK(s32Ret == HI_SUCCESS, ERROR, "Error with %s.\n", strerror(errno));
    }

    g_af_args->pipe_fd = open(PIPE_PATH, O_RDWR);
    CHECK(g_af_args->pipe_fd, ERROR, "Error with %s.\n", strerror(errno));
*/

    pthread_mutex_init(&g_af_args->mutex, NULL);
/*
    g_af_args->running = 1;
    s32Ret = pthread_create(&g_af_args->tid, NULL, af_proc, NULL);
    CHECK(s32Ret == HI_SUCCESS, ERROR, "Error with %s.\n", strerror(errno));
*/

    return 0;
}

static int af_exit(void)
{
    CHECK(NULL != g_af_args, HI_FAILURE, "module not inited.\n");
/*
    if (g_af_args->running)
    {
        g_af_args->running = 0;
        af_stop();
        pthread_join(g_af_args->tid, NULL);
    }

    if (g_af_args->pipe_fd > 0)
    {
        close(g_af_args->pipe_fd);
    }
*/

    pthread_mutex_destroy(&g_af_args->mutex);
    free(g_af_args);
    g_af_args = NULL;

    return 0;
}

static HI_S32 af_cb_init(HI_S32 s32Handle, const ISP_AF_PARAM_S *pstAfParam)
{
    HI_S32 s32Ret = -1;
    ISP_DEV IspDev = 0;
    ISP_STATISTICS_CFG_S stIspStaticsCfg;
    memset(&stIspStaticsCfg, 0, sizeof(stIspStaticsCfg));
    ISP_PUB_ATTR_S stPubattr;
    memset(&stPubattr, 0, sizeof(stPubattr));

    s32Ret = HI_MPI_ISP_GetStatisticsConfig(IspDev, &stIspStaticsCfg);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = HI_MPI_ISP_GetPubAttr(IspDev, &stPubattr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

/*
    s32Ret = an41908_init(stPubattr.f32FrameRate);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
*/

    s32Ret = af_init();
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    stFocusCfg.stConfig.u16Vsize = stPubattr.stWndRect.u32Height;
    stFocusCfg.stConfig.u16Hsize = stPubattr.stWndRect.u32Width;
    DBG("Hsize*Vsize, [%dx%d]\n", stFocusCfg.stConfig.u16Hsize, stFocusCfg.stConfig.u16Vsize);

    stIspStaticsCfg.unKey.bit1AfStat = 0x1;

    memcpy(&stIspStaticsCfg.stFocusCfg, &stFocusCfg, sizeof(ISP_FOCUS_STATISTICS_CFG_S));

    s32Ret = HI_MPI_ISP_SetStatisticsConfig(IspDev, &stIspStaticsCfg);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    DBG("done...\n");
    return 0;
}

static HI_S32 af_cb_exit(HI_S32 s32Handle)
{
    HI_S32 s32Ret = -1;

    s32Ret = af_exit();
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

/*
    s32Ret = an41908_exit();
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
*/

    return 0;
}

static HI_S32 af_cb_ctrl(HI_S32 s32Handle, HI_U32 u32Cmd, HI_VOID *pValue)
{
    //DBG("\n");

    return 0;
}

static int af_get_weight_sum()
{
    HI_U32 i = 0, j = 0;
    HI_U32 u32WgtSum = 0;

    for (i = 0; i < stFocusCfg.stConfig.u16Vwnd; i++)
    {
        for (j = 0; j < stFocusCfg.stConfig.u16Hwnd; j++)
        {
            u32WgtSum += AFWeight[i][j];
        }
    }

    return u32WgtSum;
}

/*
static int af_smoothing_calc(int index, int fv)
{
    int fv_val = 0;
    g_af_args->frame_cnt[index]++;
    g_af_args->fv_cnt[index] += fv;

    int fnt = 5;

    if (g_af_args->frame_cnt[index] >= fnt)
    {
        fv_val = g_af_args->fv_cnt[index]/fnt;

        g_af_args->frame_cnt[index] -= 1;
        g_af_args->fv_cnt[index] -= fv_val;
    }
    //printf("%d\n", fv_val);

    return fv_val;
}
*/

static HI_S32 af_cb_run(HI_S32 s32Handle, const ISP_AF_INFO_S *pstAfInfo, ISP_AF_RESULT_S *pstAfResult, HI_S32 s32Rsv)
{
    //HI_S32 s32Ret = -1;
    HI_U32 i = 0, j = 0;
    HI_U32 u32SumFv1 = 0;
    HI_U32 u32SumFv2 = 0;
    HI_U32 u32WgtSum = af_get_weight_sum();
    HI_U32 u32Fv1_n = 0, u32Fv2_n = 0;

    memset(g_af_args->fv1, 0, sizeof(g_af_args->fv1));
    memset(g_af_args->fv2, 0, sizeof(g_af_args->fv2));

    for (i = 0; i < stFocusCfg.stConfig.u16Vwnd; i++)
    {
        for (j = 0; j < stFocusCfg.stConfig.u16Hwnd; j++)
        {
            HI_U32 u32H1 =pstAfInfo->pstAfStat->stZoneMetrics[i][j].u16h1;
            HI_U32 u32H2 =pstAfInfo->pstAfStat->stZoneMetrics[i][j].u16h2;
            HI_U32 u32V1 =pstAfInfo->pstAfStat->stZoneMetrics[i][j].u16v1;
            HI_U32 u32V2 =pstAfInfo->pstAfStat->stZoneMetrics[i][j].u16v2;
            u32Fv1_n = (u32H1 * ALPHA + u32V1 * ((1<<BLEND_SHIFT) - ALPHA)) >>BLEND_SHIFT;
            u32Fv2_n = (u32H2 * BELTA + u32V2 * ((1<<BLEND_SHIFT) - BELTA)) >>BLEND_SHIFT;

            int idx = i*stFocusCfg.stConfig.u16Hwnd+j;
            g_af_args->fv1[idx] = AFWeight[i][j] * u32Fv1_n / u32WgtSum;
            g_af_args->fv2[idx] = AFWeight[i][j] * u32Fv2_n / u32WgtSum;

            u32SumFv1 += AFWeight[i][j] * u32Fv1_n;
            u32SumFv2 += AFWeight[i][j] * u32Fv2_n;
            //printf("%05hu ", pstAfInfo->stAfStat->stZoneMetrics[i][j].u16y);
        }
        //printf("\n");
    }
    //printf("--------------------\n");

    i = 0;
    int sum = 0;

    sum = af_get_sum(g_af_args->fv1, stFocusCfg.stConfig.u16Hwnd, stFocusCfg.stConfig.u16Vwnd);
    g_af_args->sum_fv[i] = sum;
    i++;

    sum = af_get_sum(g_af_args->fv2, stFocusCfg.stConfig.u16Hwnd, stFocusCfg.stConfig.u16Vwnd);
    g_af_args->sum_fv[i] = sum;

    i = 1;

//    DBG("FV2:%d\n", g_af_args->sum_fv[1]);
/*
    if (!access("/mnt/nand/fv1.flag", F_OK))
    {
        i = 0;
    }
*/
/*
    s32Ret = an41908_run_cb(g_af_args->sum_fv[i]);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
*/

    return 0;
}

int sal_af_register_callback(void)
{
    ISP_DEV IspDev = 0;
    HI_S32 s32Ret = -1;
    ALG_LIB_S stLib;
    memset(&stLib, 0, sizeof(stLib));
    ISP_AF_REGISTER_S stRegister;
    memset(&stRegister, 0, sizeof(stRegister));

    stLib.s32Id = 0;
    strncpy(stLib.acLibName, HI_AF_LIB_NAME, sizeof(HI_AF_LIB_NAME));

    stRegister.stAfExpFunc.pfn_af_init = af_cb_init;
    stRegister.stAfExpFunc.pfn_af_run = af_cb_run;
    stRegister.stAfExpFunc.pfn_af_ctrl = af_cb_ctrl;
    stRegister.stAfExpFunc.pfn_af_exit = af_cb_exit;

    s32Ret = HI_MPI_ISP_AFLibRegCallBack(IspDev, &stLib, &stRegister);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return 0;
}

int sal_af_unregister_callback(void)
{
    ISP_DEV IspDev = 0;
    HI_S32 s32Ret = -1;
    ALG_LIB_S stLib;
    memset(&stLib, 0, sizeof(stLib));

    stLib.s32Id = 0;
    strncpy(stLib.acLibName, HI_AF_LIB_NAME, sizeof(HI_AF_LIB_NAME));
    s32Ret = HI_MPI_ISP_AFLibUnRegCallBack(IspDev, &stLib);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return 0;
}

int sal_af_get_fv(int *value)
{
    CHECK(NULL != g_af_args, 0, "module not inited.\n");
    CHECK(NULL != value, 0, "value is null.\n");

    //int sum = g_af_args->sum_fv[0] + g_af_args->sum_fv[1];
    //主要用于图像调试，调试环境都比较理想,因此选择FV2
    *value = g_af_args->sum_fv[1];
    return 0;
}



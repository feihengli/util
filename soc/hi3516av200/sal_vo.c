#include "hi_comm.h"
#include "sal_debug.h"
#include "sal_vo.h"


typedef struct sal_vo_args
{
    int vo_width;
    int vo_height;
    int framerate;
    int vpssChn;

    int running;
    pthread_t tid;
    pthread_mutex_t mutex;
}
sal_vo_args;

static sal_vo_args* g_vo_args = NULL;


static void* proc(void* arg)
{
    int s32Ret = -1;
    VIDEO_FRAME_INFO_S stFrameInfo;
    memset(&stFrameInfo, 0, sizeof(stFrameInfo));
    DBG("----\n");
    
    while (1)
    {
        //s32Ret = HI_MPI_VO_GetChnFrame(0, 0, &stFrameInfo, 100*1000);
        s32Ret = HI_MPI_VO_GetScreenFrame(0, &stFrameInfo, 100*1000);
        if (s32Ret != HI_SUCCESS)
        {
            //DBG("HI_MPI_VPSS_GetChnFrame[%d] time out. Error with %#x.\n", g_pc_args->vpss_chn, s32Ret);
            continue;
        }


        unsigned int u32Size = (stFrameInfo.stVFrame.u32Stride[0]) * (stFrameInfo.stVFrame.u32Height) * 3 / 2;//yuv420
        unsigned char* yuv420sp = (unsigned char*) HI_MPI_SYS_Mmap(stFrameInfo.stVFrame.u32PhyAddr[0], u32Size);
        CHECK(yuv420sp, NULL, "error with %#x.\n", yuv420sp);

        //*((unsigned long long*)yuv420sp) = 0;
        DBG("VO begin HI_MPI_VO_GetScreenFrame: %02x %02x %02x %02x %02x %02x %02x %02x\n", yuv420sp[0], yuv420sp[1], yuv420sp[2], yuv420sp[3], yuv420sp[4], yuv420sp[5], yuv420sp[6], yuv420sp[7]);
        //memset(yuv420sp, 0xff, u32Size);
        //DBG("reset ok\n");

        //pc_save2file("ttt.yuv", &stFrameInfo);

        s32Ret = HI_MPI_SYS_Munmap(yuv420sp, u32Size);
        CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);
    

        //s32Ret = HI_MPI_VENC_SendFrame(g_pc_args->venc_chn, &stFrameInfo, -1);
        //CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);

        s32Ret = HI_MPI_VO_ReleaseScreenFrame(0, &stFrameInfo);
        CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);

        //break;
    }
    return NULL;
}

int vo_init()
{
    CHECK(NULL == g_vo_args, HI_FAILURE, "reinit error, please exit first.\n");
    
    g_vo_args = (sal_vo_args*)malloc(sizeof(sal_vo_args));
    CHECK(g_vo_args != NULL, HI_FAILURE, "malloc %d bytes failed.\n", sizeof(sal_vo_args));

    memset(g_vo_args, 0, sizeof(sal_vo_args));
    pthread_mutex_init(&g_vo_args->mutex, NULL);
    
    g_vo_args->vo_width = 1920;
    g_vo_args->vo_height = 1080;//pal 576 ntsc 480
    g_vo_args->framerate = 30;
    g_vo_args->vpssChn = 0;
    DBG("resolution [%dx%d], fps[%d], vpssChn[%d]\n", g_vo_args->vo_width, g_vo_args->vo_height, g_vo_args->framerate, g_vo_args->vpssChn);
    
    HI_S32  s32Ret = HI_FAILURE;
    VO_PUB_ATTR_S stVoPubAttr;
    memset(&stVoPubAttr, 0, sizeof(stVoPubAttr));
    VO_DEV VoDev = 0;

    s32Ret = HI_MPI_VO_GetPubAttr(VoDev, &stVoPubAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    stVoPubAttr.u32BgColor = 0x000000ff; //0x000000CC;
    stVoPubAttr.enIntfType = VO_INTF_BT1120; //VO_INTF_CVBS;
    stVoPubAttr.enIntfSync = VO_OUTPUT_1080P30; //VO_OUTPUT_PAL;

    s32Ret = HI_MPI_VO_SetPubAttr(VoDev, &stVoPubAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    
    s32Ret = HI_MPI_VO_Enable(VoDev);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    
    himm_write(0x12010044, 0x47f4);
    DBG("0x12010044: %#x\n", himm_read(0x12010044));
    
    VO_LAYER VoLayer = 0;
    VO_VIDEO_LAYER_ATTR_S stVoLayerAttr;
    memset(&stVoLayerAttr, 0, sizeof(stVoLayerAttr));
    s32Ret = HI_MPI_VO_GetVideoLayerAttr(VoLayer, &stVoLayerAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    
    stVoLayerAttr.stDispRect.s32X = 0;
    stVoLayerAttr.stDispRect.s32Y = 0;
    stVoLayerAttr.bClusterMode = HI_FALSE;
    stVoLayerAttr.bDoubleFrame = HI_FALSE;
    stVoLayerAttr.stDispRect.u32Width = g_vo_args->vo_width;
    stVoLayerAttr.stDispRect.u32Height = g_vo_args->vo_height;
    stVoLayerAttr.stImageSize.u32Width = g_vo_args->vo_width;
    stVoLayerAttr.stImageSize.u32Height = g_vo_args->vo_height;
    stVoLayerAttr.u32DispFrmRt = g_vo_args->framerate;
    stVoLayerAttr.enPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_422; //PIXEL_FORMAT_YUV_SEMIPLANAR_422;
    
    s32Ret = HI_MPI_VO_SetDispBufLen(VoLayer, 3);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    
    s32Ret = HI_MPI_VO_SetVideoLayerAttr(VoLayer, &stVoLayerAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    
    s32Ret = HI_MPI_VO_EnableVideoLayer(VoLayer);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    
    VO_CHN VoChn = 0;
    VO_CHN_ATTR_S stVoChnAttr;
    memset(&stVoLayerAttr, 0, sizeof(stVoLayerAttr));
    s32Ret = HI_MPI_VO_GetChnAttr(VoLayer, VoChn, &stVoChnAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    stVoChnAttr.u32Priority = 0;
    stVoChnAttr.stRect.s32X = 0;
    stVoChnAttr.stRect.s32Y = 0;
    stVoChnAttr.stRect.u32Width = g_vo_args->vo_width;
    stVoChnAttr.stRect.u32Height = g_vo_args->vo_height;
    stVoChnAttr.bDeflicker = HI_FALSE;
    s32Ret = HI_MPI_VO_SetChnAttr(VoLayer, VoChn, &stVoChnAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    
    s32Ret = HI_MPI_VO_EnableChn(VoLayer, VoChn);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    
    //MPP_CHN_S stSrcChn;
    //memset(&stSrcChn, 0, sizeof(stSrcChn));
    //MPP_CHN_S stDestChn;
    //memset(&stDestChn, 0, sizeof(stDestChn));

    //stSrcChn.enModId = HI_ID_VIU; //HI_ID_VPSS;
    //stSrcChn.s32DevId = 0;
    //stSrcChn.s32ChnId = g_vo_args->vpssChn;

    //stDestChn.enModId = HI_ID_VOU;
    //stDestChn.s32DevId = 0;
    //stDestChn.s32ChnId = VoChn;

    //s32Ret = HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
    //CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    
    pthread_t tid = 0;
    //pthread_create(&tid, NULL, proc, NULL);
    
    DBG("done...\n");
    return 0;
}

int vo_exit()
{
    CHECK(NULL != g_vo_args, HI_FAILURE, "module not inited.\n");
    
    HI_S32  s32Ret = HI_FAILURE;
    VO_DEV VoDev = 0;
    VO_CHN VoChn = 0;
    
    MPP_CHN_S stSrcChn;
    memset(&stSrcChn, 0, sizeof(stSrcChn));
    MPP_CHN_S stDestChn;
    memset(&stDestChn, 0, sizeof(stDestChn));

    stSrcChn.enModId = HI_ID_VPSS;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = g_vo_args->vpssChn;

    stDestChn.enModId = HI_ID_VOU;
    stDestChn.s32DevId = 0;
    stDestChn.s32ChnId = VoChn;

    s32Ret = HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    
    s32Ret = HI_MPI_VO_DisableChn(VoDev, VoChn);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = HI_MPI_VO_DisableVideoLayer(VoDev);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = HI_MPI_VO_Disable(VoDev);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = HI_MPI_VO_CloseFd();
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    
    pthread_mutex_destroy(&g_vo_args->mutex);
    free(g_vo_args);
    g_vo_args = NULL;
    
    return 0;
}



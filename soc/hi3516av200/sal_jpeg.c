
#include "sal_av.h"
#include "sal_jpeg.h"
#include "sal_debug.h"
#include "hi_comm.h"
#include "sal_config.h"

#define JPEG_LOCAL_TEST 0

typedef struct sal_jpeg_args
{
    int channel;
    int widht;
    int height;
    int fps;
    sal_jpeg_cb cb;
    
    int onestream_enable;//单包模式/多包模式
    char* multiple_buffer; //多包模式下需要使用buffer来拼合一个帧

    int running;
    pthread_t tid;
    pthread_mutex_t mutex;
}sal_jpeg_args;

static sal_jpeg_args* g_jpeg_args = NULL;

static HI_S32 jpeg_create_chn()
{
    HI_S32 s32Ret = HI_FAILURE;
    VENC_CHN_ATTR_S stVencChnAttr;
    memset(&stVencChnAttr, 0, sizeof(stVencChnAttr));
    VENC_ATTR_JPEG_S stJpegAttr;
    memset(&stJpegAttr, 0, sizeof(stJpegAttr));

    stVencChnAttr.stVeAttr.enType = PT_JPEG;
    stJpegAttr.u32MaxPicWidth  = g_jpeg_args->widht;
    stJpegAttr.u32MaxPicHeight = g_jpeg_args->height;
    stJpegAttr.u32PicWidth  = g_jpeg_args->widht;
    stJpegAttr.u32PicHeight = g_jpeg_args->height;
    stJpegAttr.u32BufSize = g_jpeg_args->widht*g_jpeg_args->height*2;
    stJpegAttr.bByFrame = HI_TRUE;
    stJpegAttr.bSupportDCF = HI_FALSE;
    memcpy(&stVencChnAttr.stVeAttr.stAttrJpege, &stJpegAttr, sizeof(VENC_ATTR_JPEG_S));

    s32Ret = HI_MPI_VENC_CreateChn(g_jpeg_args->channel, &stVencChnAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return HI_SUCCESS;
}

static int jpeg_destroy_chn()
{
    HI_S32 s32Ret;

    s32Ret = HI_MPI_VENC_DestroyChn(g_jpeg_args->channel);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return HI_SUCCESS;
}

static HI_S32 jpeg_venc_bind_vpss()
{
    HI_S32 s32Ret = HI_SUCCESS;
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;

    stSrcChn.enModId = HI_ID_VPSS;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = g_jpeg_args->channel;

    stDestChn.enModId = HI_ID_VENC;
    stDestChn.s32DevId = 0;
    stDestChn.s32ChnId = g_jpeg_args->channel;

    s32Ret = HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return s32Ret;
}

static int jpeg_venc_unbind_vpss()
{
    HI_S32 s32Ret = HI_SUCCESS;
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;

    stSrcChn.enModId = HI_ID_VPSS;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = g_jpeg_args->channel;

    stDestChn.enModId = HI_ID_VENC;
    stDestChn.s32DevId = 0;
    stDestChn.s32ChnId = g_jpeg_args->channel;

    s32Ret = HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return s32Ret;
}

static HI_S32 jpeg_set_Qfactor()
{
    HI_S32 s32Ret = HI_FAILURE;

    VENC_PARAM_JPEG_S stJpegParam;
    memset(&stJpegParam, 0, sizeof(stJpegParam));

    s32Ret = HI_MPI_VENC_GetJpegParam(g_jpeg_args->channel, &stJpegParam);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    stJpegParam.u32Qfactor = 50;
    DBG("u32Qfactor: %u\n", stJpegParam.u32Qfactor);

    s32Ret = HI_MPI_VENC_SetJpegParam(g_jpeg_args->channel, &stJpegParam);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return HI_SUCCESS;
}

static HI_S32 jpeg_enable_chn()
{
    HI_S32 s32Ret = HI_FAILURE;
    VPSS_CHN_ATTR_S stVpssChnAttr;
    memset(&stVpssChnAttr, 0, sizeof(stVpssChnAttr));
    VPSS_CHN_MODE_S stVpssChnMode;
    memset(&stVpssChnAttr, 0, sizeof(stVpssChnAttr));
    VPSS_GRP VpssGrp = 0;

    sal_stream_s stream;
    memset(&stream, 0, sizeof(stream));
    s32Ret = sal_video_args_get(0, &stream);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    stVpssChnMode.enChnMode     = VPSS_CHN_MODE_USER;
    stVpssChnMode.bDouble       = HI_FALSE;
    stVpssChnMode.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    stVpssChnMode.u32Width      = g_jpeg_args->widht;
    stVpssChnMode.u32Height     = g_jpeg_args->height;
    stVpssChnMode.enCompressMode = COMPRESS_MODE_NONE;
    stVpssChnAttr.s32SrcFrameRate = stream.framerate; // vin framerate
    stVpssChnAttr.s32DstFrameRate = (g_jpeg_args->fps > stream.framerate) ? stream.framerate : g_jpeg_args->fps;

    s32Ret = HI_MPI_VPSS_SetChnAttr(VpssGrp, g_jpeg_args->channel, &stVpssChnAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    ROTATE_E enRotate = ROTATE_NONE;
    s32Ret = HI_MPI_VPSS_GetRotate(0, 0, &enRotate); //get vpss chn0 rotate attr
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    if (enRotate == ROTATE_90 || enRotate == ROTATE_270)
    {
        HI_U32 temp = stVpssChnMode.u32Width;
        stVpssChnMode.u32Width = stVpssChnMode.u32Height;
        stVpssChnMode.u32Height = temp;
    }

    s32Ret = HI_MPI_VPSS_SetChnMode(VpssGrp, g_jpeg_args->channel, &stVpssChnMode);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    if (ROTATE_NONE != enRotate && g_jpeg_args->channel < VPSS_MAX_PHY_CHN_NUM)
    {
        s32Ret = HI_MPI_VPSS_SetRotate(VpssGrp, g_jpeg_args->channel, enRotate);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    }

    s32Ret = HI_MPI_VPSS_EnableChn(VpssGrp, g_jpeg_args->channel);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return HI_SUCCESS;
}

static int jpeg_disable_chn()
{
    HI_S32 s32Ret;

    s32Ret = HI_MPI_VPSS_DisableChn(0, g_jpeg_args->channel);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return HI_SUCCESS;
}

static int jpeg_write_frame(char* addr, int size)
{
    static FILE* fp = NULL;
    static int count = 0;

    char name[32] = "";
    sprintf(name, "snap_%d.jpg", count++);
    if (!access(name, F_OK))
        remove(name);
    fp = fopen(name, "wb+");
    if (fp== NULL)
        DBG("failed to open [%s]: %s\n", name, strerror(errno));

    if (fp)
    {
        fwrite(addr, 1, size, fp);
        fclose(fp);
    }

    return 0;
}

static int jpeg_one_pack(VENC_CHN i, VENC_STREAM_S* pstStream, VENC_CHN_STAT_S* pstStat)
{
    HI_S32 s32Ret = HI_FAILURE;
    VENC_PACK_S pack;
    memset(&pack, 0, sizeof(pack));

    CHECK(1 == pstStat->u32CurPacks, HI_FAILURE, "Error with %#x.\n", s32Ret);
    pstStream->pstPack = &pack;
    pstStream->u32PackCount = pstStat->u32CurPacks;

    int timeout = 0; // 非阻塞
    s32Ret = HI_MPI_VENC_GetStream(i, pstStream, timeout);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    HI_U8* frame_addr = pstStream->pstPack->pu8Addr + pstStream->pstPack->u32Offset;
    HI_U32 frame_len = pstStream->pstPack->u32Len - pstStream->pstPack->u32Offset;
    int isKey = (pstStream->pstPack->DataType.enH264EType == H264E_NALU_ISLICE) ? 1 : 0;

    //s32Ret = venc_write_cb(i, pstStream->pstPack->u64PTS, (char*)frame_addr, frame_len, isKey);
    //CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    if (g_jpeg_args->cb)
    {
        g_jpeg_args->cb((char*)frame_addr, frame_len);
    }

    if (JPEG_LOCAL_TEST)
    {
        s32Ret = jpeg_write_frame((char*)frame_addr, frame_len);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    }

    s32Ret = HI_MPI_VENC_ReleaseStream(i, pstStream);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return 0;
}

static int jpeg_multiple_pack(VENC_CHN i, VENC_STREAM_S* pstStream, VENC_CHN_STAT_S* pstStat)
{
    HI_S32 s32Ret = HI_FAILURE;
    char* buffer = g_jpeg_args->multiple_buffer;

    //DBG("u32CurPacks: %u\n", pstStat->u32CurPacks);
    pstStream->pstPack = (VENC_PACK_S*)malloc(sizeof(VENC_PACK_S) * pstStat->u32CurPacks);
    CHECK(pstStream->pstPack, HI_FAILURE, "malloc %d bytes failed.\n", sizeof(VENC_PACK_S) * pstStat->u32CurPacks);

    pstStream->u32PackCount = pstStat->u32CurPacks;

    s32Ret = HI_MPI_VENC_GetStream(i, pstStream, HI_TRUE);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    int isKey = 0;
    int buffer_offset = 0;
    int j = 0;
    for (j = 0; j < pstStream->u32PackCount; j++)
    {
        HI_U8* frame_addr = pstStream->pstPack[j].pu8Addr + pstStream->pstPack[j].u32Offset;
        HI_U32 frame_len = pstStream->pstPack[j].u32Len - pstStream->pstPack[j].u32Offset;
        isKey |= (pstStream->pstPack[j].DataType.enH264EType == H264E_NALU_IDRSLICE) ? 1 : 0;
        //DBG("stream: %d type: %d isKey: %d\n", i, pstStream->pstPack[j].DataType.enH264EType, isKey);

        memcpy(buffer+buffer_offset, frame_addr, frame_len);
        buffer_offset += frame_len;
    }

    //DBG("stream: %d, len: %d isKey: %d, u64PTS: %llu\n", i, buffer_offset, isKey, pstStream->pstPack[j-1].u64PTS);
    if (g_jpeg_args->cb)
    {
        g_jpeg_args->cb((char*)buffer, buffer_offset);
    }

    if (JPEG_LOCAL_TEST)
    {
        s32Ret = jpeg_write_frame((char*)buffer, buffer_offset);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    }

    s32Ret = HI_MPI_VENC_ReleaseStream(i, pstStream);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    free(pstStream->pstPack);
    pstStream->pstPack = NULL;

    return 0;
}

static HI_S32 jpeg_get_one_picture()
{
    HI_S32 s32Ret = HI_FAILURE;
    VENC_RECV_PIC_PARAM_S stRecvParam;
    memset(&stRecvParam, 0, sizeof(stRecvParam));
    VENC_CHN VencChn = g_jpeg_args->channel;

    stRecvParam.s32RecvPicNum = 1;
    s32Ret = HI_MPI_VENC_StartRecvPicEx(VencChn, &stRecvParam);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    HI_S32 s32VencFd = HI_MPI_VENC_GetFd(VencChn);
    CHECK(s32VencFd > 0, HI_FAILURE, "Error with %#x.\n", s32Ret);

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(s32VencFd, &read_fds);

    struct timeval TimeoutVal = {1, 0};
    s32Ret = select(s32VencFd + 1, &read_fds, NULL, NULL, &TimeoutVal);
    if (s32Ret < 0)
    {
        DBG("snap select failed!\n");
        return HI_FAILURE;
    }
    else if (0 == s32Ret)
    {
        DBG("snap time out!\n");
    }
    else
    {
        if (FD_ISSET(s32VencFd, &read_fds))
        {
            VENC_CHN_STAT_S stStat;
            s32Ret = HI_MPI_VENC_Query(VencChn, &stStat);
            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

            if (0 == stStat.u32CurPacks)
            {
                DBG("NOTE: Current  frame is NULL!\n");
                return HI_SUCCESS;
            }
            //DBG("u32CurPacks: %u\n", stStat.u32CurPacks);
            
            VENC_STREAM_S stStream;
            memset(&stStream, 0, sizeof(stStream));
            if (g_jpeg_args->onestream_enable)
            {
                s32Ret = jpeg_one_pack(VencChn, &stStream, &stStat);
                CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
            }
            else
            {
                s32Ret = jpeg_multiple_pack(VencChn, &stStream, &stStat);
                CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
            }
        }
    }

    s32Ret = HI_MPI_VENC_StopRecvPic(VencChn);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return HI_SUCCESS;
}

static void* jpeg_get_stream_thr(void *p)
{
    prctl(PR_SET_NAME, __FUNCTION__);
    HI_S32 s32Ret = HI_FAILURE;
    int interval = 1*1000*1000/g_jpeg_args->fps;

    while (g_jpeg_args->running)
    {
        s32Ret = jpeg_get_one_picture();
        CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);

        usleep(interval);
    }

    return NULL;
}

int sal_jpeg_init(sal_jpeg_cb cb)
{
    CHECK(NULL == g_jpeg_args, HI_FAILURE, "reinit error, please exit first.\n");

    HI_S32 s32Ret = HI_FAILURE;
    g_jpeg_args = (sal_jpeg_args*)malloc(sizeof(sal_jpeg_args));
    CHECK(g_jpeg_args != NULL, HI_FAILURE, "malloc %d bytes failed.\n", sizeof(sal_jpeg_args));

    memset(g_jpeg_args, 0, sizeof(sal_jpeg_args));
    pthread_mutex_init(&g_jpeg_args->mutex, NULL);
    g_jpeg_args->channel = g_config.jpeg.channel;
    g_jpeg_args->widht = g_config.jpeg.widht;
    g_jpeg_args->height = g_config.jpeg.height;
    g_jpeg_args->cb = cb;
    g_jpeg_args->fps = 5;
    
    g_jpeg_args->onestream_enable = 0;
    if (!g_jpeg_args->onestream_enable)
    {
        int size = g_jpeg_args->widht*g_jpeg_args->height/3;
        g_jpeg_args->multiple_buffer = malloc(size);
        CHECK(g_jpeg_args->multiple_buffer, HI_FAILURE, "malloc %d bytes failed.\n", size);
    }

    sal_stream_s stream;
    memset(&stream, 0, sizeof(stream));
    s32Ret = sal_video_args_get(0, &stream);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    CHECK((g_jpeg_args->widht <= stream.width) && (g_jpeg_args->height <= stream.height), HI_FAILURE,
            "jpeg[%dx%d] is too big than main stream[%dx%d].\n", g_jpeg_args->widht, g_jpeg_args->height, stream.width, stream.height);

    s32Ret = jpeg_create_chn();
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = jpeg_enable_chn();
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = jpeg_set_Qfactor();
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = jpeg_venc_bind_vpss();
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    g_jpeg_args->running = 1;
    s32Ret = pthread_create(&g_jpeg_args->tid, NULL, jpeg_get_stream_thr, NULL);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %s.\n", strerror(errno));

    return HI_SUCCESS;
}

int sal_jpeg_exit()
{
    CHECK(NULL != g_jpeg_args, HI_FAILURE, "module not inited.\n");
    HI_S32 s32Ret = HI_FAILURE;

    if (g_jpeg_args->running)
    {
        g_jpeg_args->running = 0;
        pthread_join(g_jpeg_args->tid, NULL);
    }

    s32Ret = jpeg_venc_unbind_vpss();
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = jpeg_destroy_chn();
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

     s32Ret = jpeg_disable_chn();
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    
    if (!g_jpeg_args->onestream_enable && g_jpeg_args->multiple_buffer)
    {
        free(g_jpeg_args->multiple_buffer);
        g_jpeg_args->multiple_buffer = NULL;
    }
    
    pthread_mutex_destroy(&g_jpeg_args->mutex);
    free(g_jpeg_args);
    g_jpeg_args = NULL;

    return 0;
}



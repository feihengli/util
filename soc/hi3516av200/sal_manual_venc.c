
#include "sal_av.h"
#include "sal_jpeg.h"
#include "sal_debug.h"
#include "hi_comm.h"
#include "sal_config.h"

#define VENC1_LOCAL_TEST 1

typedef struct sal_mv_args
{
    sal_stream_s stream;
    sal_video_frame_cb cb;
    int venc_chn;

    int running;
    pthread_t pid;
    pthread_mutex_t mutex;
}sal_mv_args;

static sal_mv_args* g_mv_args = NULL;

int venc1_create_chn(int channel)
{
    HI_S32 s32Ret = -1;
    VENC_CHN_ATTR_S stVencChnAttr;
    memset(&stVencChnAttr, 0, sizeof(stVencChnAttr));
    VENC_ATTR_H264_S stH264Attr;
    memset(&stH264Attr, 0, sizeof(stH264Attr));
    VENC_ATTR_H264_CBR_S    stH264Cbr;
    memset(&stH264Cbr, 0, sizeof(stH264Cbr));
    VENC_ATTR_H264_VBR_S    stH264Vbr;
    memset(&stH264Vbr, 0, sizeof(stH264Vbr));

    sal_stream_s* stream = &g_mv_args->stream;
    stVencChnAttr.stVeAttr.enType = PT_H264;

    stH264Attr.u32MaxPicWidth = stream->width;
    stH264Attr.u32MaxPicHeight = stream->height;
    stH264Attr.u32PicWidth = stream->width;
    stH264Attr.u32PicHeight = stream->height;
    stH264Attr.u32BufSize  = stream->width * stream->height*2;
    stH264Attr.u32Profile  = 2;/*0: baseline; 1:MP; 2:HP;  3:svc_t */
    stH264Attr.bByFrame = HI_TRUE;
    DBG("res[%dx%d], u32Profile: %d\n", stream->width, stream->height, stH264Attr.u32Profile);
    //stH264Attr.u32BFrameNum = 0;
    //stH264Attr.u32RefNum = 1;
    memcpy(&stVencChnAttr.stVeAttr.stAttrH264e, &stH264Attr, sizeof(VENC_ATTR_H264_S));

    stVencChnAttr.stGopAttr.enGopMode = VENC_GOPMODE_NORMALP;
    stVencChnAttr.stGopAttr.stNormalP.s32IPQpDelta = 4;//IP帧QP差值，用于调节呼吸效应。建议值：[2, 6]

    if (stream->bitrate_ctl == SAL_BITRATE_CONTROL_CBR)
    {
        stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
        stH264Cbr.u32Gop            = stream->gop;
        stH264Cbr.u32StatTime       = stream->gop / stream->framerate;
        stH264Cbr.u32SrcFrmRate  = stream->framerate;
        stH264Cbr.fr32DstFrmRate = stream->framerate;
        stH264Cbr.u32BitRate = stream->bitrate;
        stH264Cbr.u32FluctuateLevel = 1;
        memcpy(&stVencChnAttr.stRcAttr.stAttrH264Cbr, &stH264Cbr, sizeof(VENC_ATTR_H264_CBR_S));
        DBG("u32SrcFrmRate: %u\n", stH264Cbr.u32SrcFrmRate);
    }
    else if (stream->bitrate_ctl == SAL_BITRATE_CONTROL_VBR)
    {
        stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264VBR;
        stH264Vbr.u32Gop = stream->gop;
        stH264Vbr.u32StatTime = stream->gop / stream->framerate;
        stH264Vbr.u32SrcFrmRate = stream->framerate;
        stH264Vbr.fr32DstFrmRate = stream->framerate;
        stH264Vbr.u32MinQp = 25;
        stH264Vbr.u32MinIQp = 25;
        stH264Vbr.u32MaxQp = 51;
        stH264Vbr.u32MaxBitRate = stream->bitrate;
        memcpy(&stVencChnAttr.stRcAttr.stAttrH264Vbr, &stH264Vbr, sizeof(VENC_ATTR_H264_VBR_S));
    }
    else
    {
        CHECK(0, HI_FAILURE, "Error with %#x.\n", -1);
    }

    s32Ret = HI_MPI_VENC_CreateChn(channel, &stVencChnAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    
    s32Ret = HI_MPI_VENC_StartRecvPic(channel);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    
    s32Ret = HI_MPI_VENC_StartRecvPic(channel);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return HI_SUCCESS;
}

int venc1_destroy_chn(int channel)
{
    HI_S32 s32Ret;
    
    s32Ret = HI_MPI_VENC_StopRecvPic(channel);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    
    s32Ret = HI_MPI_VENC_DestroyChn(channel);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return HI_SUCCESS;
}

static int venc1_file_write(int stream_id, const char* file, const void* addr, int size)
{
    static FILE* fp[] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

    if (fp[stream_id] == NULL)
    {
        char name[32] = "";
        sprintf(name, "%s_%d", file, stream_id);
        if (!access(name, F_OK))
            remove(name);
        fp[stream_id] = fopen(name, "a+");
        if (fp[stream_id] == NULL)
            DBG("failed to open [%s]: %s\n", name, strerror(errno));
    }

    if (fp[stream_id])
    {
        fwrite(addr, 1, size, fp[stream_id]);
    }

    return 0;
}

static int venc1_one_pack(VENC_CHN i, VENC_STREAM_S* pstStream, VENC_CHN_STAT_S* pstStat)
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
    
    sal_stream_s* stream = &g_mv_args->stream;
    HI_U8* frame_addr = pstStream->pstPack->pu8Addr + pstStream->pstPack->u32Offset;
    HI_U32 frame_len = pstStream->pstPack->u32Len - pstStream->pstPack->u32Offset;
    int isKey = (pstStream->pstPack->DataType.enH264EType == H264E_NALU_ISLICE) ? 1 : 0;
    
    //DBG("stream: %d, len: %d isKey: %d, u64PTS: %llu\n", i, frame_len, isKey, pstStream->pstPack->u64PTS);
    if (g_mv_args->cb)
    {   
        double timestamp = pstStream->pstPack->u64PTS/1000;
        s32Ret = g_mv_args->cb(i, frame_addr, frame_len, isKey, timestamp, stream->encode_type);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    }

    if (VENC1_LOCAL_TEST)
    {   
        const char* path = (SAL_ENCODE_TYPE_H264 == stream->encode_type) ? "stream_264" : "stream_265";
        venc1_file_write(i, path, frame_addr, frame_len);
    }

    s32Ret = HI_MPI_VENC_ReleaseStream(i, pstStream);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return 0;
}

static int venc1_multiple_pack(VENC_CHN i, VENC_STREAM_S* pstStream, VENC_CHN_STAT_S* pstStat)
{
    HI_S32 s32Ret = HI_FAILURE;
    static unsigned char* buffer = NULL;
    static int buffer_size = 0;
    if (buffer == NULL)
    {
        buffer = malloc(1024*1024);
        CHECK(buffer, -1, "Error with %#x\n", buffer);
        buffer_size = 1024*1024;
    }

    //DBG("u32CurPacks: %u\n", pstStat->u32CurPacks);
    pstStream->pstPack = (VENC_PACK_S*)malloc(sizeof(VENC_PACK_S) * pstStat->u32CurPacks);
    CHECK(pstStream->pstPack, HI_FAILURE, "malloc %d bytes failed.\n", sizeof(VENC_PACK_S) * pstStat->u32CurPacks);

    pstStream->u32PackCount = pstStat->u32CurPacks;

    s32Ret = HI_MPI_VENC_GetStream(i, pstStream, HI_TRUE);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    int isKey = 0;
    int buffer_offset = 0;
    int j = 0;
    sal_stream_s* stream = &g_mv_args->stream;
    for (j = 0; j < pstStream->u32PackCount; j++)
    {
        HI_U8* frame_addr = pstStream->pstPack[j].pu8Addr + pstStream->pstPack[j].u32Offset;
        HI_U32 frame_len = pstStream->pstPack[j].u32Len - pstStream->pstPack[j].u32Offset;
        if (SAL_ENCODE_TYPE_H264 == stream->encode_type)
        {
            isKey |= (pstStream->pstPack[j].DataType.enH264EType == H264E_NALU_IDRSLICE) ? 1 : 0;
            //DBG("stream: %d type: %d isKey: %d\n", i, pstStream->pstPack[j].DataType.enH264EType, isKey);
        }
        else if (SAL_ENCODE_TYPE_H265 == stream->encode_type)
        {
            isKey |= (pstStream->pstPack[j].DataType.enH265EType == H265E_NALU_IDRSLICE) ? 1 : 0;
        }
        else
        {
            CHECK(0, HI_FAILURE, "Error with %#x.\n", -1);
        }
        
        CHECK((buffer_offset+frame_len) <= buffer_size, -1, "Error with %#x\n", buffer_size);
        memcpy(buffer+buffer_offset, frame_addr, frame_len);
        buffer_offset += frame_len;
    }

    //DBG("stream: %d, len: %d isKey: %d, u64PTS: %llu\n", i, buffer_offset, isKey, pstStream->pstPack[j-1].u64PTS);
    if (g_mv_args->cb)
    {   
        double timestamp = pstStream->pstPack[j-1].u64PTS/1000;
        s32Ret = g_mv_args->cb(i, buffer, buffer_offset, isKey, timestamp, stream->encode_type);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    }

    if (VENC1_LOCAL_TEST)
    {   
        const char* path = (SAL_ENCODE_TYPE_H264 == stream->encode_type) ? "stream_264" : "stream_265";
        venc1_file_write(i, path, buffer, buffer_offset);
    }

    s32Ret = HI_MPI_VENC_ReleaseStream(i, pstStream);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    free(pstStream->pstPack);
    pstStream->pstPack = NULL;

    return 0;
}

void* venc1_stream_thread(void* arg)
{
    sal_mv_args* p_mv_args = (sal_mv_args*)arg;
    HI_S32 s32Ret = HI_FAILURE;
    VENC_RECV_PIC_PARAM_S stRecvParam;
    memset(&stRecvParam, 0, sizeof(stRecvParam));
    VENC_CHN VencChn = p_mv_args->venc_chn;
    
    //DBG("channel: %d\n", channel);
    //stRecvParam.s32RecvPicNum = 1;
    //s32Ret = HI_MPI_VENC_StartRecvPicEx(VencChn, &stRecvParam);
    //CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    HI_S32 s32VencFd = HI_MPI_VENC_GetFd(VencChn);
    CHECK(s32VencFd > 0, NULL, "Error with %#x.\n", s32Ret);
    
    while (p_mv_args->running)
    {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(s32VencFd, &read_fds);

        struct timeval TimeoutVal = {0, 100*1000};
        s32Ret = select(s32VencFd + 1, &read_fds, NULL, NULL, &TimeoutVal);
        if (s32Ret < 0)
        {
            DBG("venc1 select failed!\n");
            return NULL;
        }
        else if (0 == s32Ret)
        {
            //DBG("venc1 time out!\n");
            continue;
        }
        else
        {
            if (FD_ISSET(s32VencFd, &read_fds))
            {
                VENC_CHN_STAT_S stStat;
                s32Ret = HI_MPI_VENC_Query(VencChn, &stStat);
                CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);

                if (0 == stStat.u32CurPacks)
                {
                    DBG("NOTE: Current  frame is NULL!\n");
                    return NULL;
                }
                //DBG("u32CurPacks: %u\n", stStat.u32CurPacks);
                
                VENC_STREAM_S stStream;
                memset(&stStream, 0, sizeof(stStream));
                if (stStat.u32CurPacks > 0 && stStat.u32CurPacks == 1)
                {
                    s32Ret = venc1_one_pack(VencChn, &stStream, &stStat);
                    CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);
                }
                else if (stStat.u32CurPacks > 0 && stStat.u32CurPacks > 1)
                {
                    s32Ret = venc1_multiple_pack(VencChn, &stStream, &stStat);
                    CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);
                }
            }
        }
    }

    //s32Ret = HI_MPI_VENC_StopRecvPic(VencChn);
    //CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return HI_SUCCESS;
}


int venc1_init()
{
    CHECK(NULL == g_mv_args, -1, "reinit error, please uninit first.\n");

    int ret = -1;

    g_mv_args = (sal_mv_args*)malloc(sizeof(sal_mv_args));
    CHECK(NULL != g_mv_args, -1, "malloc %d bytes failed.\n", sizeof(sal_mv_args));

    memset(g_mv_args, 0, sizeof(sal_mv_args));
    pthread_mutex_init(&g_mv_args->mutex, NULL);

    g_mv_args->venc_chn = 2;
    g_mv_args->stream.bitrate = 1000;
    g_mv_args->stream.bitrate_ctl = SAL_BITRATE_CONTROL_CBR;
    g_mv_args->stream.enable = 1;
    g_mv_args->stream.encode_type = SAL_ENCODE_TYPE_H264;
    g_mv_args->stream.framerate = 30;
    g_mv_args->stream.gop = 30;
    g_mv_args->stream.width = 640;
    g_mv_args->stream.height = 480;
    
    venc1_create_chn(g_mv_args->venc_chn);
    
    g_mv_args->running = 1;
    ret = pthread_create(&g_mv_args->pid, NULL, venc1_stream_thread, g_mv_args);
    CHECK(ret == 0, -1, "Error with %s.\n", strerror(errno));

    return 0;
}

int venc1_exit()
{


    return 0;
}



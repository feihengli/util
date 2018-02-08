
#include "sal_av.h"
#include "sal_jpeg.h"
#include "sal_debug.h"
#include "hi_comm.h"
#include "sal_config.h"

#define JPEG1_LOCAL_TEST 1

int jpeg1_create_chn(int channel, int widht, int height)
{
    HI_S32 s32Ret = HI_FAILURE;
    VENC_CHN_ATTR_S stVencChnAttr;
    memset(&stVencChnAttr, 0, sizeof(stVencChnAttr));
    VENC_ATTR_JPEG_S stJpegAttr;
    memset(&stJpegAttr, 0, sizeof(stJpegAttr));

    stVencChnAttr.stVeAttr.enType = PT_JPEG;
    stJpegAttr.u32MaxPicWidth  = widht;
    stJpegAttr.u32MaxPicHeight = height;
    stJpegAttr.u32PicWidth  = widht;
    stJpegAttr.u32PicHeight = height;
    stJpegAttr.u32BufSize = widht*height*2;
    stJpegAttr.bByFrame = HI_TRUE;
    stJpegAttr.bSupportDCF = HI_FALSE;
    memcpy(&stVencChnAttr.stVeAttr.stAttrJpege, &stJpegAttr, sizeof(VENC_ATTR_JPEG_S));

    s32Ret = HI_MPI_VENC_CreateChn(channel, &stVencChnAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    
    s32Ret = HI_MPI_VENC_StartRecvPic(channel);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    
    //VENC_RECV_PIC_PARAM_S stRecvParam;
    //memset(&stRecvParam, 0, sizeof(stRecvParam));
    //stRecvParam.s32RecvPicNum = 1;
    //s32Ret = HI_MPI_VENC_StartRecvPicEx(channel, &stRecvParam);
    //CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return HI_SUCCESS;
}

int jpeg1_destroy_chn(int channel)
{
    HI_S32 s32Ret;
    
    s32Ret = HI_MPI_VENC_StopRecvPic(channel);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    
    s32Ret = HI_MPI_VENC_DestroyChn(channel);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return HI_SUCCESS;
}

int jpeg1_set_Qfactor(int channel, int Qfactor)
{
    HI_S32 s32Ret = HI_FAILURE;

    VENC_PARAM_JPEG_S stJpegParam;
    memset(&stJpegParam, 0, sizeof(stJpegParam));

    s32Ret = HI_MPI_VENC_GetJpegParam(channel, &stJpegParam);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    stJpegParam.u32Qfactor = Qfactor;
    DBG("u32Qfactor: %u\n", stJpegParam.u32Qfactor);

    s32Ret = HI_MPI_VENC_SetJpegParam(channel, &stJpegParam);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return HI_SUCCESS;
}

static int jpeg1_write_frame(char* addr, int size)
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

static int jpeg1_one_pack(VENC_CHN i, VENC_STREAM_S* pstStream, VENC_CHN_STAT_S* pstStat, unsigned char** out, int* len)
{
    HI_S32 s32Ret = HI_FAILURE;
    VENC_PACK_S pack;
    memset(&pack, 0, sizeof(pack));

    CHECK(1 == pstStat->u32CurPacks, HI_FAILURE, "Error with %#x.\n", s32Ret);
    pstStream->pstPack = &pack;
    pstStream->u32PackCount = pstStat->u32CurPacks;

    int timeout = 0; // ·Ç×èÈû
    s32Ret = HI_MPI_VENC_GetStream(i, pstStream, timeout);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    HI_U8* frame_addr = pstStream->pstPack->pu8Addr + pstStream->pstPack->u32Offset;
    HI_U32 frame_len = pstStream->pstPack->u32Len - pstStream->pstPack->u32Offset;
    //int isKey = (pstStream->pstPack->DataType.enH264EType == H264E_NALU_ISLICE) ? 1 : 0;

    *out = malloc(frame_len);
    CHECK(*out, -1, "Error with %#x\n", *out);
    *len = frame_len;
    memcpy(*out, frame_addr, frame_len);

    if (JPEG1_LOCAL_TEST)
    {
        s32Ret = jpeg1_write_frame((char*)frame_addr, frame_len);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    }

    s32Ret = HI_MPI_VENC_ReleaseStream(i, pstStream);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return 0;
}

static int jpeg1_multiple_pack(VENC_CHN i, VENC_STREAM_S* pstStream, VENC_CHN_STAT_S* pstStat, unsigned char** out, int* len)
{
    HI_S32 s32Ret = HI_FAILURE;
    static unsigned char* buffer = NULL;
    static int buffer_size = 0;
    if (buffer == NULL)
    {
        buffer = malloc(512*1024);
        CHECK(buffer, -1, "Error with %#x\n", buffer);
        buffer_size = 512*1024;
    }

    //DBG("u32CurPacks: %u\n", pstStat->u32CurPacks);
    pstStream->pstPack = (VENC_PACK_S*)malloc(sizeof(VENC_PACK_S) * pstStat->u32CurPacks);
    CHECK(pstStream->pstPack, HI_FAILURE, "malloc %d bytes failed.\n", sizeof(VENC_PACK_S) * pstStat->u32CurPacks);

    pstStream->u32PackCount = pstStat->u32CurPacks;

    s32Ret = HI_MPI_VENC_GetStream(i, pstStream, HI_TRUE);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    //int isKey = 0;
    int buffer_offset = 0;
    int j = 0;
    for (j = 0; j < pstStream->u32PackCount; j++)
    {
        HI_U8* frame_addr = pstStream->pstPack[j].pu8Addr + pstStream->pstPack[j].u32Offset;
        HI_U32 frame_len = pstStream->pstPack[j].u32Len - pstStream->pstPack[j].u32Offset;
        //isKey = 1;
        //DBG("stream: %d type: %d isKey: %d\n", i, pstStream->pstPack[j].DataType.enJPEGEType, isKey);
        
        CHECK((buffer_offset+frame_len) <= buffer_size, -1, "Error with %#x\n", buffer_size);
        memcpy(buffer+buffer_offset, frame_addr, frame_len);
        buffer_offset += frame_len;
    }

    //DBG("stream: %d, len: %d isKey: %d, u64PTS: %llu\n", i, buffer_offset, isKey, pstStream->pstPack[j-1].u64PTS);
    *out = malloc(buffer_offset);
    CHECK(*out, -1, "Error with %#x\n", *out);
    *len = buffer_offset;
    memcpy(*out, buffer, buffer_offset);

    if (JPEG1_LOCAL_TEST)
    {
        s32Ret = jpeg1_write_frame((char*)buffer, buffer_offset);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    }

    s32Ret = HI_MPI_VENC_ReleaseStream(i, pstStream);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    free(pstStream->pstPack);
    pstStream->pstPack = NULL;

    return 0;
}

int jpeg1_get_picture(int channel, unsigned char** out, int* len)
{
    HI_S32 s32Ret = HI_FAILURE;
    VENC_RECV_PIC_PARAM_S stRecvParam;
    memset(&stRecvParam, 0, sizeof(stRecvParam));
    VENC_CHN VencChn = channel;
    
    //DBG("channel: %d\n", channel);
    //stRecvParam.s32RecvPicNum = 1;
    //s32Ret = HI_MPI_VENC_StartRecvPicEx(VencChn, &stRecvParam);
    //CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    HI_S32 s32VencFd = HI_MPI_VENC_GetFd(VencChn);
    CHECK(s32VencFd > 0, HI_FAILURE, "Error with %#x.\n", s32Ret);

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(s32VencFd, &read_fds);

    struct timeval TimeoutVal = {0, 10*1000};
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
            if (stStat.u32CurPacks > 0 && stStat.u32CurPacks == 1)
            {
                s32Ret = jpeg1_one_pack(VencChn, &stStream, &stStat, out, len);
                CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
            }
            else if (stStat.u32CurPacks > 0 && stStat.u32CurPacks > 1)
            {
                s32Ret = jpeg1_multiple_pack(VencChn, &stStream, &stStat, out, len);
                CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
            }
        }
    }

    //s32Ret = HI_MPI_VENC_StopRecvPic(VencChn);
    //CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return HI_SUCCESS;
}

int jpeg1_release_picture(int channel, unsigned char** in, int* len)
{
    free((*in));
    *len = 0;
    return 0;
}




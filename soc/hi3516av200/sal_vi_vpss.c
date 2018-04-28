#include "hi_comm.h"
#include "sal_debug.h"
#include "sal_util.h"
#include "sal_vgs.h"
#include "sal_vi_vpss.h"
#include "sal_t01.h"
#include "sal_vgs.h"
#include "sal_list.h"
#include "sal_jpeg1.h"
#include "sal_malloc.h"

#define MAX_FRAME_QUEUE 10
#define MAX_FRAME_FILTER 10

typedef struct _frame_queue_s
{
    VIDEO_FRAME_INFO_S astVideoFrame[MAX_FRAME_QUEUE];
    int idx_readable;
    int idx_writable;
    int num_valid;
}frame_queue_s;

typedef struct _yuv420_s
{
    //unsigned int x;
    //unsigned int y;
    //unsigned int width;
    //unsigned int height;
    IngDetObject stIngDetObject;
    unsigned char* buffer;
    unsigned int size;
    unsigned int status; //0 不编码 1 编码使能 2 编码完成
}yuv420_s;

typedef struct _sal_vv_args
{
    unsigned int count;
    int vi_chn;
    int vo_chn;
    VPSS_GRP VpssGrp;
    int enable;
    frame_queue_s frame_queue;
    rectangle_s astArectangle[128];
    int validArectangleNum;

    int running;
    pthread_t pid;
    pthread_mutex_t mutex;
    
    int jpeg_chn;
    handle hndListImageYuv420;
    unsigned int usrPrivateCountLast;
    
    int running1;
    pthread_t pid1;
    pthread_mutex_t mutex1;
}sal_vv_args;

static sal_vv_args* g_vv_args = NULL;

static unsigned int vv_stride(unsigned int u16Width, unsigned char u8Align)
{
    return (u16Width + (u8Align - u16Width%u8Align)%u8Align);
}

static int vv_cutYuv420sp(unsigned char *tarYuv, unsigned char *srcYuv, int startW,  
                   int startH, int cutW, int cutH, int srcW, int srcH)
{  
    int i = 0;  
    int j = 0;  
    int k = 0;  

    unsigned char *tmpY = tarYuv;  
    unsigned char *tmpUV = tarYuv+cutW*cutH;  
    for (i=startH; i<cutH+startH; i++) 
    {  
        // 逐行拷贝Y分量，共拷贝cutW*cutH  
        memcpy(tmpY+j*cutW, srcYuv+startW+i*srcW, cutW);  
        j++;  
    }  
    for (i=startH/2; i<(cutH+startH)/2; i++) 
    {  
        //逐行拷贝UV分量，共拷贝cutW*cutH/2  
        memcpy(tmpUV+k*cutW, srcYuv+startW+srcW*srcH+i*srcW, cutW);  
        k++;  
    }  

    return 0;
}

static yuv420_s* vv_find(int id, int type)
{
    yuv420_s* pstYuv420 = list_front(g_vv_args->hndListImageYuv420);
    while (pstYuv420)
    {
        if (pstYuv420->stIngDetObject.id == id && pstYuv420->stIngDetObject.type == type)
        {
            return pstYuv420;
        }
        pstYuv420 = list_next(g_vv_args->hndListImageYuv420, pstYuv420);
    }
    
    return NULL;
}

static int vv_crop2list(VIDEO_FRAME_INFO_S* pstOutFrameInfo, HwDetObj* pstObj)
{
    int s32Ret = -1;
    unsigned int i = 0;
    //unsigned int u32Size = (pstOutFrameInfo->stVFrame.u32Stride[0]) * (pstOutFrameInfo->stVFrame.u32Height) * 3 / 2;//yuv420
    unsigned char* yuv420sp = (unsigned char*)pstOutFrameInfo->stVFrame.pVirAddr[0];
    CHECK(yuv420sp, -1, "error with %#x.\n", yuv420sp);

    //DBG("u32PoolId: %u\n", pstOutFrameInfo->u32PoolId);
    //DBG("u32Width: %u\n", pstOutFrameInfo->stVFrame.u32Width);
    //DBG("u32Height: %u\n", pstOutFrameInfo->stVFrame.u32Height);
    //DBG("u32Stride: %u %u %u\n", pstOutFrameInfo->stVFrame.u32Stride[0], pstOutFrameInfo->stVFrame.u32Stride[1], pstOutFrameInfo->stVFrame.u32Stride[2]);
    
    pthread_mutex_lock(&g_vv_args->mutex1);
    for (i = 0; i < pstObj->valid_obj_num; i++)
    {
        unsigned int startW = vv_stride(pstObj->obj[i].x, 16);
        unsigned int startH = vv_stride(pstObj->obj[i].y, 16);
        if (startW >= pstOutFrameInfo->stVFrame.u32Width || startH >= pstOutFrameInfo->stVFrame.u32Height)
        {
            WRN("invalid startW[%u] startH[%u]\n", startW, startH);
            continue;
        }
        unsigned int cutW = vv_stride(pstObj->obj[i].width, 16);
        unsigned int cutH = vv_stride(pstObj->obj[i].height, 16);
        if (startW+cutW > pstOutFrameInfo->stVFrame.u32Width)
        {
            cutW = pstOutFrameInfo->stVFrame.u32Width - startW;
        }
        if (startH+cutH > pstOutFrameInfo->stVFrame.u32Height)
        {
            cutH = pstOutFrameInfo->stVFrame.u32Height - startH;
        }
        if (cutW%16 != 0 || cutH%16 != 0)
        {
            WRN("invalid startW[%u] startH[%u] cutW[%u] cutH[%u]\n", startW, startH, cutW, cutH);
            continue;
        }

        //DBG("xy[%d %d], wh[%d %d]\n", startW, startH, cutW, cutH);
        
        if (g_vv_args->usrPrivateCountLast == 0)
        {
            g_vv_args->usrPrivateCountLast = pstObj->usrPrivateCount;
        }

        yuv420_s* pstYuv420 = vv_find(pstObj->obj[i].id, pstObj->obj[i].type);
        if (pstYuv420)
        {
            if (pstObj->obj[i].strength > pstYuv420->stIngDetObject.strength)
            {
                int new_size = (cutW * cutH * 3 >> 1);
                if (pstYuv420->size < new_size)
                {
                    mem_free(pstYuv420->buffer);
                    pstYuv420->size = new_size;
                    pstYuv420->buffer = mem_malloc(pstYuv420->size);
                    CHECK(pstYuv420->buffer, -1, "Error with %#x.\n", pstYuv420->buffer);
                }
                else
                {
                    pstYuv420->size = new_size;
                }
                memcpy(&pstYuv420->stIngDetObject, &pstObj->obj[i], sizeof(IngDetObject));
                pstYuv420->stIngDetObject.x = startW;
                pstYuv420->stIngDetObject.y = startH;
                pstYuv420->stIngDetObject.width = cutW;
                pstYuv420->stIngDetObject.height = cutH;
                vv_cutYuv420sp(pstYuv420->buffer, yuv420sp, startW, startH, cutW, cutH, pstOutFrameInfo->stVFrame.u32Stride[0], pstOutFrameInfo->stVFrame.u32Height);
                if ((pstObj->usrPrivateCount - g_vv_args->usrPrivateCountLast) > MAX_FRAME_FILTER)
                {
                    pstYuv420->status = 1;
                    g_vv_args->usrPrivateCountLast = pstObj->usrPrivateCount;
                }
            }
        }
        else
        {
            yuv420_s stYuv420;
            memset(&stYuv420, 0, sizeof(stYuv420));
            memcpy(&stYuv420.stIngDetObject, &pstObj->obj[i], sizeof(IngDetObject));
            stYuv420.stIngDetObject.x = startW;
            stYuv420.stIngDetObject.y = startH;
            stYuv420.stIngDetObject.width = cutW;
            stYuv420.stIngDetObject.height = cutH;
            stYuv420.size = cutW * cutH * 3 >> 1;
            stYuv420.buffer = mem_malloc(stYuv420.size);
            CHECK(stYuv420.buffer, -1, "Error with %#x.\n", stYuv420.buffer);

            vv_cutYuv420sp(stYuv420.buffer, yuv420sp, startW, startH, cutW, cutH, pstOutFrameInfo->stVFrame.u32Stride[0], pstOutFrameInfo->stVFrame.u32Height);
            if ((pstObj->usrPrivateCount - g_vv_args->usrPrivateCountLast) > MAX_FRAME_FILTER)
            {
                stYuv420.status = 1;
                g_vv_args->usrPrivateCountLast = pstObj->usrPrivateCount;
            }
            
            s32Ret = list_push_back(g_vv_args->hndListImageYuv420, &stYuv420, sizeof(stYuv420));
            CHECK(s32Ret == 0, -1, "Error with %#x.\n", s32Ret);
        }
    }
    pthread_mutex_unlock(&g_vv_args->mutex1);
    return 0;
}

static int vv_crop2jpeg(VIDEO_FRAME_INFO_S* pstOutFrameInfo, rectangle_s* pstArectangle, int num)
{
    int s32Ret = -1;
    unsigned int i = 0;
    //unsigned int u32Size = (pstOutFrameInfo->stVFrame.u32Stride[0]) * (pstOutFrameInfo->stVFrame.u32Height) * 3 / 2;//yuv420
    unsigned char* yuv420sp = (unsigned char*)pstOutFrameInfo->stVFrame.pVirAddr[0];
    CHECK(yuv420sp, -1, "error with %#x.\n", yuv420sp);

    //DBG("u32PoolId: %u\n", pstOutFrameInfo->u32PoolId);
    //DBG("u32Width: %u\n", pstOutFrameInfo->stVFrame.u32Width);
    //DBG("u32Height: %u\n", pstOutFrameInfo->stVFrame.u32Height);
    //DBG("u32Stride: %u %u %u\n", pstOutFrameInfo->stVFrame.u32Stride[0], pstOutFrameInfo->stVFrame.u32Stride[1], pstOutFrameInfo->stVFrame.u32Stride[2]);

    for (i = 0; i < num; i++)
    {
        unsigned int startW = vv_stride(pstArectangle[i].x, 16);
        unsigned int startH = vv_stride(pstArectangle[i].y, 16);
        if (startW >= pstOutFrameInfo->stVFrame.u32Width || startH >= pstOutFrameInfo->stVFrame.u32Height)
        {
            WRN("invalid startW[%u] startH[%u]\n", startW, startH);
            continue;
        }
        unsigned int cutW = vv_stride(pstArectangle[i].width, 16);
        unsigned int cutH = vv_stride(pstArectangle[i].height, 16);
        if (startW+cutW > pstOutFrameInfo->stVFrame.u32Width)
        {
            cutW = pstOutFrameInfo->stVFrame.u32Width - startW;
        }
        if (startH+cutH > pstOutFrameInfo->stVFrame.u32Height)
        {
            cutH = pstOutFrameInfo->stVFrame.u32Height - startH;
        }
        if (cutW%16 != 0 || cutH%16 != 0)
        {
            WRN("invalid startW[%u] startH[%u] cutW[%u] cutH[%u]\n", startW, startH, cutW, cutH);
            continue;
        }
        
        //DBG("xy[%d %d], wh[%d %d]\n", startW, startH, cutW, cutH);
        VIDEO_FRAME_INFO_S stFrmInfo;
        memset(&stFrmInfo, 0, sizeof(stFrmInfo));
        unsigned int u32LumaSize = cutW * cutH;
        unsigned int u32ChrmSize = cutW * cutH >> 2;
        unsigned int u32BlkSize  = cutW * cutH * 3 >> 1;
        
        stFrmInfo.u32PoolId =  HI_MPI_VB_CreatePool( u32BlkSize, 1, HI_NULL);
        CHECK(stFrmInfo.u32PoolId != VB_INVALID_POOLID, -1, "Error with %#x.\n", stFrmInfo.u32PoolId);
        
        VB_BLK  hBlock = HI_MPI_VB_GetBlock(stFrmInfo.u32PoolId, u32BlkSize, HI_NULL);
        CHECK(hBlock != VB_INVALID_HANDLE, -1, "Error with %#x.\n", hBlock);
        
        HI_U32 u32PhyAddr = HI_MPI_VB_Handle2PhysAddr(hBlock);
        CHECK(u32PhyAddr > 0, -1, "Error with %#x.\n", u32PhyAddr);
        
        HI_U8* pVirAddr = (HI_U8*)HI_MPI_SYS_Mmap(u32PhyAddr, u32BlkSize);
        CHECK(pVirAddr, -1, "Error with %#x.\n", pVirAddr);
        
        vv_cutYuv420sp(pVirAddr, yuv420sp, startW, startH, cutW, cutH, pstOutFrameInfo->stVFrame.u32Stride[0], pstOutFrameInfo->stVFrame.u32Height);
        stFrmInfo.stVFrame.u32PhyAddr[0] = u32PhyAddr;
        stFrmInfo.stVFrame.u32PhyAddr[1] = stFrmInfo.stVFrame.u32PhyAddr[0] + u32LumaSize;
        stFrmInfo.stVFrame.u32PhyAddr[2] = stFrmInfo.stVFrame.u32PhyAddr[1] + u32ChrmSize;

        stFrmInfo.stVFrame.pVirAddr[0] = pVirAddr;
        stFrmInfo.stVFrame.pVirAddr[1] = (HI_U8*) stFrmInfo.stVFrame.pVirAddr[0] + u32LumaSize;
        stFrmInfo.stVFrame.pVirAddr[2] = (HI_U8*) stFrmInfo.stVFrame.pVirAddr[1] + u32ChrmSize;

        stFrmInfo.stVFrame.u32Width  = cutW;
        stFrmInfo.stVFrame.u32Height = cutH;
        stFrmInfo.stVFrame.u32Stride[0] = cutW;
        stFrmInfo.stVFrame.u32Stride[1] = cutW;
        stFrmInfo.stVFrame.u32Stride[2] = cutW;
        
        stFrmInfo.stVFrame.u32Field = VIDEO_FIELD_FRAME;
        stFrmInfo.stVFrame.enCompressMode = COMPRESS_MODE_NONE;
        stFrmInfo.stVFrame.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
        stFrmInfo.stVFrame.enVideoFormat = VIDEO_FORMAT_LINEAR;

        s32Ret = jpeg1_create_chn(g_vv_args->jpeg_chn, cutW, cutH);
        CHECK(s32Ret == HI_SUCCESS, -1, "Error with %#x.\n", s32Ret);
        
        s32Ret = HI_MPI_VENC_SendFrame(g_vv_args->jpeg_chn, &stFrmInfo, -1);
        CHECK(s32Ret == HI_SUCCESS, -1, "Error with %#x.\n", s32Ret);
        
        unsigned char* out_buffer = NULL;
        int out_size = 0;
        s32Ret = jpeg1_get_picture(g_vv_args->jpeg_chn, &out_buffer, &out_size);
        CHECK(s32Ret == HI_SUCCESS, -1, "Error with %#x.\n", s32Ret);
        
        s32Ret = jpeg1_release_picture(g_vv_args->jpeg_chn, &out_buffer, &out_size);
        CHECK(s32Ret == HI_SUCCESS, -1, "Error with %#x.\n", s32Ret);
        
        s32Ret = jpeg1_destroy_chn(g_vv_args->jpeg_chn);
        CHECK(s32Ret == HI_SUCCESS, -1, "Error with %#x.\n", s32Ret);
        
        s32Ret = HI_MPI_VB_ReleaseBlock(hBlock);
        CHECK(s32Ret == HI_SUCCESS, -1, "Error with %#x.\n", s32Ret);

        s32Ret = HI_MPI_SYS_Munmap((HI_VOID*)stFrmInfo.stVFrame.pVirAddr[0], u32BlkSize);
        CHECK(s32Ret == HI_SUCCESS, -1, "Error with %#x.\n", s32Ret);

        s32Ret = HI_MPI_VB_DestroyPool(stFrmInfo.u32PoolId);
        CHECK(s32Ret == HI_SUCCESS, -1, "Error with %#x.\n", s32Ret);
    }
    
    return 0;
}

static int vv_crop2jpeg1(yuv420_s* pstYuv420Info)
{
    int s32Ret = -1;
    //unsigned int i = 0;

    //DBG("xy[%d %d], wh[%d %d]\n", pstYuv420Info->x, pstYuv420Info->y, pstYuv420Info->width, pstYuv420Info->height);
    VIDEO_FRAME_INFO_S stFrmInfo;
    memset(&stFrmInfo, 0, sizeof(stFrmInfo));
    unsigned int u32LumaSize = pstYuv420Info->stIngDetObject.width * pstYuv420Info->stIngDetObject.height;
    unsigned int u32ChrmSize = pstYuv420Info->stIngDetObject.width * pstYuv420Info->stIngDetObject.height >> 2;
    unsigned int u32BlkSize  = pstYuv420Info->stIngDetObject.width * pstYuv420Info->stIngDetObject.height * 3 >> 1;

    stFrmInfo.u32PoolId =  HI_MPI_VB_CreatePool( u32BlkSize, 1, HI_NULL);
    CHECK(stFrmInfo.u32PoolId != VB_INVALID_POOLID, -1, "Error with %#x.\n", stFrmInfo.u32PoolId);

    VB_BLK  hBlock = HI_MPI_VB_GetBlock(stFrmInfo.u32PoolId, u32BlkSize, HI_NULL);
    CHECK(hBlock != VB_INVALID_HANDLE, -1, "Error with %#x.\n", hBlock);

    HI_U32 u32PhyAddr = HI_MPI_VB_Handle2PhysAddr(hBlock);
    CHECK(u32PhyAddr > 0, -1, "Error with %#x.\n", u32PhyAddr);

    HI_U8* pVirAddr = (HI_U8*)HI_MPI_SYS_Mmap(u32PhyAddr, u32BlkSize);
    CHECK(pVirAddr, -1, "Error with %#x.\n", pVirAddr);

    stFrmInfo.stVFrame.u32PhyAddr[0] = u32PhyAddr;
    stFrmInfo.stVFrame.u32PhyAddr[1] = stFrmInfo.stVFrame.u32PhyAddr[0] + u32LumaSize;
    stFrmInfo.stVFrame.u32PhyAddr[2] = stFrmInfo.stVFrame.u32PhyAddr[1] + u32ChrmSize;
    
    memcpy(pVirAddr, pstYuv420Info->buffer, pstYuv420Info->size);
    stFrmInfo.stVFrame.pVirAddr[0] = pVirAddr;
    stFrmInfo.stVFrame.pVirAddr[1] = (HI_U8*) stFrmInfo.stVFrame.pVirAddr[0] + u32LumaSize;
    stFrmInfo.stVFrame.pVirAddr[2] = (HI_U8*) stFrmInfo.stVFrame.pVirAddr[1] + u32ChrmSize;

    stFrmInfo.stVFrame.u32Width  = pstYuv420Info->stIngDetObject.width;
    stFrmInfo.stVFrame.u32Height = pstYuv420Info->stIngDetObject.height;
    stFrmInfo.stVFrame.u32Stride[0] = pstYuv420Info->stIngDetObject.width;
    stFrmInfo.stVFrame.u32Stride[1] = pstYuv420Info->stIngDetObject.width;
    stFrmInfo.stVFrame.u32Stride[2] = pstYuv420Info->stIngDetObject.width;

    stFrmInfo.stVFrame.u32Field = VIDEO_FIELD_FRAME;
    stFrmInfo.stVFrame.enCompressMode = COMPRESS_MODE_NONE;
    stFrmInfo.stVFrame.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    stFrmInfo.stVFrame.enVideoFormat = VIDEO_FORMAT_LINEAR;

    s32Ret = jpeg1_create_chn(g_vv_args->jpeg_chn, pstYuv420Info->stIngDetObject.width, pstYuv420Info->stIngDetObject.height);
    CHECK(s32Ret == HI_SUCCESS, -1, "Error with %#x.\n", s32Ret);
    
    s32Ret = HI_MPI_VENC_SendFrame(g_vv_args->jpeg_chn, &stFrmInfo, 40);
    CHECK(s32Ret == HI_SUCCESS, -1, "Error with %#x.\n", s32Ret);

    unsigned char* out_buffer = NULL;
    int out_size = 0;
    s32Ret = jpeg1_get_picture(g_vv_args->jpeg_chn, &out_buffer, &out_size);
    CHECK(s32Ret == HI_SUCCESS, -1, "Error with %#x.\n", s32Ret);

    s32Ret = jpeg1_release_picture(g_vv_args->jpeg_chn, &out_buffer, &out_size);
    CHECK(s32Ret == HI_SUCCESS, -1, "Error with %#x.\n", s32Ret);

    s32Ret = jpeg1_destroy_chn(g_vv_args->jpeg_chn);
    CHECK(s32Ret == HI_SUCCESS, -1, "Error with %#x.\n", s32Ret);

    s32Ret = HI_MPI_VB_ReleaseBlock(hBlock);
    CHECK(s32Ret == HI_SUCCESS, -1, "Error with %#x.\n", s32Ret);

    s32Ret = HI_MPI_SYS_Munmap((HI_VOID*)stFrmInfo.stVFrame.pVirAddr[0], u32BlkSize);
    CHECK(s32Ret == HI_SUCCESS, -1, "Error with %#x.\n", s32Ret);

    s32Ret = HI_MPI_VB_DestroyPool(stFrmInfo.u32PoolId);
    CHECK(s32Ret == HI_SUCCESS, -1, "Error with %#x.\n", s32Ret);

    return 0;
}

static int vv_crop2jpeg2(VIDEO_FRAME_INFO_S* pstOutFrameInfo, rectangle_s* pstArectangle, int num)
{
    VIDEO_FRAME_INFO_S stOutFrameInfoBak;
    memcpy(&stOutFrameInfoBak, pstOutFrameInfo, sizeof(stOutFrameInfoBak));
    int s32Ret = -1;
    unsigned int i = 0;
    //unsigned int u32Size = (pstOutFrameInfo->stVFrame.u32Stride[0]) * (pstOutFrameInfo->stVFrame.u32Height) * 3 / 2;//yuv420
    //unsigned char* yuv420sp = (unsigned char*) HI_MPI_SYS_Mmap(pstOutFrameInfo->stVFrame.u32PhyAddr[0], u32Size);
    unsigned char* yuv420sp = (unsigned char*)pstOutFrameInfo->stVFrame.pVirAddr[0];
    CHECK(yuv420sp, -1, "error with %#x.\n", yuv420sp);

    //DBG("u32PoolId: %u\n", pstOutFrameInfo->u32PoolId);
    //DBG("u32Width: %u\n", pstOutFrameInfo->stVFrame.u32Width);
    //DBG("u32Height: %u\n", pstOutFrameInfo->stVFrame.u32Height);
    //DBG("u32Stride: %u %u %u\n", pstOutFrameInfo->stVFrame.u32Stride[0], pstOutFrameInfo->stVFrame.u32Stride[1], pstOutFrameInfo->stVFrame.u32Stride[2]);

    for (i = 0; i < num; i++)
    {
        unsigned int startW = vv_stride(pstArectangle[i].x, 16);
        unsigned int startH = vv_stride(pstArectangle[i].y, 16);
        if (startW >= pstOutFrameInfo->stVFrame.u32Width || startH >= pstOutFrameInfo->stVFrame.u32Height)
        {
            WRN("invalid startW[%u] startH[%u]\n", startW, startH);
            continue;
        }
        unsigned int cutW = vv_stride(pstArectangle[i].width, 16);
        unsigned int cutH = vv_stride(pstArectangle[i].height, 16);
        if (startW+cutW > pstOutFrameInfo->stVFrame.u32Width)
        {
            cutW = pstOutFrameInfo->stVFrame.u32Width - startW;
        }
        if (startH+cutH > pstOutFrameInfo->stVFrame.u32Height)
        {
            cutH = pstOutFrameInfo->stVFrame.u32Height - startH;
        }
        if (cutW%16 != 0 || cutH%16 != 0)
        {
            WRN("invalid startW[%u] startH[%u] cutW[%u] cutH[%u]\n", startW, startH, cutW, cutH);
            continue;
        }

        int size = cutW*cutH*3/2;
        unsigned char* dstBuff = mem_malloc(size*2);
        CHECK(dstBuff, -1, "error with %#x.\n", dstBuff);
        unsigned char* bakBuff = dstBuff + size;

        DBG("xy[%d %d], wh[%d %d]\n", startW, startH, cutW, cutH);
        memcpy(bakBuff, yuv420sp, size);
        vv_cutYuv420sp(dstBuff, yuv420sp, startW, startH, cutW, cutH, pstOutFrameInfo->stVFrame.u32Stride[0], pstOutFrameInfo->stVFrame.u32Height);

        memcpy(yuv420sp, dstBuff, size);
        pstOutFrameInfo->stVFrame.u32Width = cutW;
        pstOutFrameInfo->stVFrame.u32Height = cutH;
        pstOutFrameInfo->stVFrame.u32Stride[0] = cutW;
        pstOutFrameInfo->stVFrame.u32Stride[1] = cutW;
        pstOutFrameInfo->stVFrame.u32Stride[2] = cutW;
        pstOutFrameInfo->stVFrame.u32PhyAddr[0] = pstOutFrameInfo->stVFrame.u32PhyAddr[0];
        pstOutFrameInfo->stVFrame.u32PhyAddr[1] = pstOutFrameInfo->stVFrame.u32PhyAddr[0] + pstOutFrameInfo->stVFrame.u32Stride[0]*pstOutFrameInfo->stVFrame.u32Height;
        pstOutFrameInfo->stVFrame.u32PhyAddr[2] = pstOutFrameInfo->stVFrame.u32PhyAddr[1] + pstOutFrameInfo->stVFrame.u32Stride[1]*pstOutFrameInfo->stVFrame.u32Height/4;

        s32Ret = jpeg1_create_chn(g_vv_args->jpeg_chn, cutW, cutH);
        CHECK(s32Ret == HI_SUCCESS, -1, "Error with %#x.\n", s32Ret);

        s32Ret = HI_MPI_VENC_SendFrame(g_vv_args->jpeg_chn, pstOutFrameInfo, -1);
        CHECK(s32Ret == HI_SUCCESS, -1, "Error with %#x.\n", s32Ret);

        unsigned char* out_buffer = NULL;
        int out_size = 0;
        s32Ret = jpeg1_get_picture(g_vv_args->jpeg_chn, &out_buffer, &out_size);
        CHECK(s32Ret == HI_SUCCESS, -1, "Error with %#x.\n", s32Ret);

        s32Ret = jpeg1_release_picture(g_vv_args->jpeg_chn, &out_buffer, &out_size);
        CHECK(s32Ret == HI_SUCCESS, -1, "Error with %#x.\n", s32Ret);

        s32Ret = jpeg1_destroy_chn(g_vv_args->jpeg_chn);
        CHECK(s32Ret == HI_SUCCESS, -1, "Error with %#x.\n", s32Ret);

        memcpy(yuv420sp, bakBuff, size);
        memcpy(pstOutFrameInfo, &stOutFrameInfoBak, sizeof(stOutFrameInfoBak));
        mem_free(dstBuff);
    }

    //s32Ret = HI_MPI_SYS_Munmap(yuv420sp, u32Size);
    //CHECK(s32Ret == HI_SUCCESS, -1, "Error with %#x.\n", s32Ret);

    return 0;
}

static int vv_overlay(frame_queue_s* pstFrameQueue, HwDetObj* pstObj)
{
    int s32Ret = -1;
    unsigned int i = 0;
    unsigned int j = 0;
    int idxArectangle = 0;
    //memset(g_vv_args->astArectangle, 0, sizeof(g_vv_args->astArectangle));
    //g_vv_args->validArectangleNum = 0;
    
    for (i = 0; i < MAX_FRAME_QUEUE; i++)
    {
        //DBG("usrPrivateCount: %u, u32PrivateData: %u\n", pstObj->usrPrivateCount, pstFrameQueue->astVideoFrame[i].stVFrame.u32PrivateData);
        if (pstObj->usrPrivateCount == pstFrameQueue->astVideoFrame[i].stVFrame.u32PrivateData)
        {
            //DBG("found i=%d, idx_readable=%d usrPrivateCount=%d\n", i, pstFrameQueue->idx_readable, pstObj->usrPrivateCount);

            for (j = 0; j < pstObj->valid_obj_num; j++)
            {
                //g_vv_args->astArectangle[idxArectangle].x = pstObj->obj[j].x;
                //g_vv_args->astArectangle[idxArectangle].y = pstObj->obj[j].y;
                //g_vv_args->astArectangle[idxArectangle].width = pstObj->obj[j].width;
                //g_vv_args->astArectangle[idxArectangle].height = pstObj->obj[j].height;
                //DBG("idxArectangle=%d xy[%d %d], wh[%d %d]\n", idxArectangle, astArectangle[idxArectangle].x, astArectangle[idxArectangle].y, 
                //                                        astArectangle[idxArectangle].width, astArectangle[idxArectangle].height);
                idxArectangle++;
            }
            //g_vv_args->validArectangleNum = idxArectangle;
            
            //if (g_vv_args->validArectangleNum > 0)
            {
                //s32Ret = vv_crop2jpeg(&pstFrameQueue->astVideoFrame[i], g_vv_args->astArectangle, idxArectangle);
                //CHECK(s32Ret == HI_SUCCESS, -1, "Error with %#x.\n", s32Ret);
                
                s32Ret = vv_crop2list(&pstFrameQueue->astVideoFrame[i], pstObj);
                CHECK(s32Ret == HI_SUCCESS, -1, "Error with %#x.\n", s32Ret);
                
                //s32Ret = sal_vgs_draw_rectangle1(&pstFrameQueue->astVideoFrame[i], 2, 0x00FF0000, g_vv_args->astArectangle, g_vv_args->validArectangleNum);
                //CHECK(s32Ret == HI_SUCCESS, -1, "Error with %#x.\n", s32Ret);
            }
        }
    }
    
    return 0;
}

static void* vv_jpeg(void* args)
{
    prctl(PR_SET_NAME, __FUNCTION__);

    int s32Ret = -1;

    while (g_vv_args->running1)
    {
        pthread_mutex_lock(&g_vv_args->mutex1);
        
        yuv420_s* pdelete = NULL;
        yuv420_s* pstYuv420 = list_front(g_vv_args->hndListImageYuv420);
        while (pstYuv420)
        {
            pdelete = NULL;
            if (pstYuv420->status == 1)
            {
                s32Ret = vv_crop2jpeg1(pstYuv420);
                CHECK(s32Ret == 0, NULL, "Error with %#x.\n", s32Ret);
                
                if (pstYuv420->buffer)
                {
                    mem_free(pstYuv420->buffer);
                    pdelete = pstYuv420;
                }
            }
            
            pstYuv420 = list_next(g_vv_args->hndListImageYuv420, pstYuv420);
            if (pdelete)
            {
                s32Ret = list_del(g_vv_args->hndListImageYuv420, pdelete);
                CHECK(s32Ret == 0, NULL, "error with %#x\n", s32Ret);
            }
        }
        
        //DBG("size: %d\n", list_size(g_vv_args->hndListImageYuv420));
        
        pthread_mutex_unlock(&g_vv_args->mutex1);
        usleep(10*1000);
    }

    return NULL;
}

static unsigned int vv_color_get(unsigned int type)
{
    unsigned int color = 0;
    if (type == 0) //hardware face
    {
        color = 0x00ff0000; //red
    }
    else if (type == 1) //hardware head and shoulder(up body)
    {
        color = 0x0000ff00; //green
    }
    else if (type == 2) //hardware full body
    {
        color = 0x000000ff; //blue
    }
    else if (type == 3) //software up body
    {
        color = 0x00ffffff; //white
    }
    else if (type == 4) //software full body
    {
        color = 0x00ffff00; //yellow
    }
    else if (type == 5) //hardware hand
    {
        color = 0x0000ffff; //青色
    }
    else if (type == 6) //software person
    {
        color = 0x00ff80ff; //粉红
    }
    else
    {
        WRN("Unknown type: %d\n", type);
    }
    return color;
}

static void* vv_proc(void* args)
{
    prctl(PR_SET_NAME, __FUNCTION__);
    
    unsigned int clear_count = 0;
    int s32Ret = -1;
    VIDEO_FRAME_INFO_S stFrameInfo;
    memset(&stFrameInfo, 0, sizeof(stFrameInfo));
    unsigned int j = 0;

    HI_U32 u32Depth = 0;
    s32Ret = HI_MPI_VI_GetFrameDepth(g_vv_args->vi_chn, &u32Depth);
    CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);
    
    if (u32Depth < 1)
    {
        u32Depth = 1;
        DBG("vi_chn: %d, u32Depth: %u\n", g_vv_args->vi_chn, u32Depth);
    }
    
    s32Ret = HI_MPI_VI_SetFrameDepth(g_vv_args->vi_chn, u32Depth);
    CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);
    
    frame_queue_s* pstFrameQueue = &g_vv_args->frame_queue;
    HwDetObj stObj;
    memset(&stObj, 0, sizeof(stObj));
    
    while (g_vv_args->running)
    {
        memset(&stFrameInfo, 0, sizeof(stFrameInfo));
        s32Ret = HI_MPI_VI_GetFrame(g_vv_args->vi_chn, &stFrameInfo, 100*1000);
        if (s32Ret != HI_SUCCESS)
        {
            DBG("HI_MPI_VPSS_GetChnFrame[%d] time out. Error with %#x.\n", g_vv_args->vi_chn, s32Ret);
            continue;
        }
        
        if (g_vv_args->enable)
        {
            unsigned int u32Size = (stFrameInfo.stVFrame.u32Stride[0]) * (stFrameInfo.stVFrame.u32Height) * 3 / 2;//yuv420
            unsigned char* yuv420sp = (unsigned char*) HI_MPI_SYS_Mmap(stFrameInfo.stVFrame.u32PhyAddr[0], u32Size);
            CHECK(yuv420sp, NULL, "error with %#x.\n", yuv420sp);
            
            memset(yuv420sp, 0x80, 8);
            unsigned char cc[4] = {0};
            memcpy(cc, &g_vv_args->count, 4);
            
            yuv420sp[0] |= cc[0] >> 4;
            yuv420sp[1] |= cc[0] & 0x0f;
            yuv420sp[2] |= cc[1] >> 4;
            yuv420sp[3] |= cc[1] & 0x0f;
            yuv420sp[4] |= cc[2] >> 4;
            yuv420sp[5] |= cc[2] & 0x0f;
            yuv420sp[6] |= cc[3] >> 4;
            yuv420sp[7] |= cc[3] & 0x0f;
            stFrameInfo.stVFrame.u32PrivateData = g_vv_args->count;
            g_vv_args->count++;
            //DBG("u32PrivateData: %u\n", stFrameInfo.stVFrame.u32PrivateData);
            
            stFrameInfo.stVFrame.pVirAddr[0] = yuv420sp;
        }
        
        s32Ret = HI_MPI_VO_SendFrame(g_vv_args->vo_chn, g_vv_args->vo_chn, &stFrameInfo, 40);
        CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);
        
        s32Ret = sal_t01_HwDetObjGet(&stObj);
        CHECK(s32Ret == 0, NULL, "error with: %#x\n", s32Ret);
        
        if (stObj.valid_obj_num == 0)
        {
            clear_count++;
        }
        else
        {
            clear_count = 0;
        }
        if (clear_count > 10)
        {
            memset(g_vv_args->astArectangle, 0, sizeof(g_vv_args->astArectangle));
            g_vv_args->validArectangleNum = 0;
        }
        
        int idxArectangle = 0;
        if (stObj.valid_obj_num > 0)
        {
            memset(g_vv_args->astArectangle, 0, sizeof(g_vv_args->astArectangle));
            g_vv_args->validArectangleNum = 0;

            for (j = 0; j < stObj.valid_obj_num; j++)
            {
                g_vv_args->astArectangle[idxArectangle].x = stObj.obj[j].x;
                g_vv_args->astArectangle[idxArectangle].y = stObj.obj[j].y;
                g_vv_args->astArectangle[idxArectangle].width = stObj.obj[j].width;
                g_vv_args->astArectangle[idxArectangle].height = stObj.obj[j].height;
                g_vv_args->astArectangle[idxArectangle].color = vv_color_get(stObj.obj[j].type);
                idxArectangle++;
            }
            g_vv_args->validArectangleNum = idxArectangle;
        }
        //DBG("valid_obj_num: %d, validArectangleNum: %d\n", stObj.valid_obj_num, g_vv_args->validArectangleNum);
        if (/*stObj.valid_obj_num > 0 && */g_vv_args->validArectangleNum > 0)
        {
            s32Ret = sal_vgs_draw_rectangle1(&stFrameInfo, 2, 0x00ffffff, g_vv_args->astArectangle, g_vv_args->validArectangleNum);
            CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);
        }
        
        s32Ret = HI_MPI_VPSS_SendFrame(g_vv_args->VpssGrp, &stFrameInfo, 40);
        CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);
        
        if (pstFrameQueue->num_valid < MAX_FRAME_QUEUE)
        {
            memcpy(&pstFrameQueue->astVideoFrame[pstFrameQueue->idx_writable], &stFrameInfo, sizeof(stFrameInfo));
            pstFrameQueue->idx_writable = (pstFrameQueue->idx_writable + 1) % MAX_FRAME_QUEUE;
            pstFrameQueue->num_valid++;
        }
        
        
        if (pstFrameQueue->num_valid == MAX_FRAME_QUEUE)
        {
            if (stObj.valid_obj_num > 0)
            {
                s32Ret = vv_overlay(pstFrameQueue, &stObj);
                CHECK(s32Ret == 0, NULL, "error with: %#x\n", s32Ret);
            }

            unsigned char* pVirAddr = pstFrameQueue->astVideoFrame[pstFrameQueue->idx_readable].stVFrame.pVirAddr[0];
            unsigned int u32Size = (stFrameInfo.stVFrame.u32Stride[0]) * (stFrameInfo.stVFrame.u32Height) * 3 / 2;
            s32Ret = HI_MPI_SYS_Munmap(pVirAddr, u32Size);
            CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);
            
            s32Ret = HI_MPI_VI_ReleaseFrame(g_vv_args->vi_chn, &pstFrameQueue->astVideoFrame[pstFrameQueue->idx_readable]);
            CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);
            pstFrameQueue->idx_readable = (pstFrameQueue->idx_readable + 1) % MAX_FRAME_QUEUE;
            pstFrameQueue->num_valid--;
        }
    }

    while (pstFrameQueue->num_valid > 0)
    {
        unsigned char* pVirAddr = pstFrameQueue->astVideoFrame[pstFrameQueue->idx_readable].stVFrame.pVirAddr[0];
        unsigned int u32Size = (stFrameInfo.stVFrame.u32Stride[0]) * (stFrameInfo.stVFrame.u32Height) * 3 / 2;
        s32Ret = HI_MPI_SYS_Munmap(pVirAddr, u32Size);
        CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);
        
        s32Ret = HI_MPI_VI_ReleaseFrame(g_vv_args->vi_chn, &pstFrameQueue->astVideoFrame[pstFrameQueue->idx_readable]);
        CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);
        
        pstFrameQueue->idx_readable = (pstFrameQueue->idx_readable + 1) % MAX_FRAME_QUEUE;
        pstFrameQueue->num_valid--;
    }

    return NULL;
}


int sal_vv_init()
{
    CHECK(NULL == g_vv_args, -1, "reinit error, please uninit first.\n");
    
    int ret = -1;
    g_vv_args = (sal_vv_args*)mem_malloc(sizeof(sal_vv_args));
    CHECK(NULL != g_vv_args, -1, "malloc %d bytes failed.\n", sizeof(sal_vv_args));

    memset(g_vv_args, 0, sizeof(sal_vv_args));
    pthread_mutex_init(&g_vv_args->mutex, NULL);
    pthread_mutex_init(&g_vv_args->mutex1, NULL);
    
    g_vv_args->vi_chn = 0;
    g_vv_args->vo_chn = 0;
    g_vv_args->jpeg_chn = 15;//[0, 15]
    g_vv_args->VpssGrp = 0;
    g_vv_args->enable = 1;
    
    g_vv_args->hndListImageYuv420 = list_init(sizeof(yuv420_s));
    CHECK(NULL != g_vv_args->hndListImageYuv420, -1, "failed to init list.\n");
    
    g_vv_args->running = 1;
    ret = pthread_create(&g_vv_args->pid, NULL, vv_proc, NULL);
    CHECK(ret == 0, -1, "Error with %s.\n", strerror(errno));
    
    g_vv_args->running1 = 1;
    ret = pthread_create(&g_vv_args->pid1, NULL, vv_jpeg, NULL);
    CHECK(ret == 0, -1, "Error with %s.\n", strerror(errno));

    return 0;
}

int sal_vv_exit()
{
    CHECK(NULL != g_vv_args, HI_FAILURE, "module not inited.\n");
    
    int ret = -1;
    
    if (g_vv_args->running1)
    {
        g_vv_args->running1 = 0;
        pthread_join(g_vv_args->pid1, NULL);
    }
    
    if (g_vv_args->running)
    {
        g_vv_args->running = 0;
        pthread_join(g_vv_args->pid, NULL);
    }
    
    yuv420_s* pstYuv420 = list_front(g_vv_args->hndListImageYuv420);
    while (pstYuv420)
    {
        if (pstYuv420->buffer)
        {
            mem_free(pstYuv420->buffer);
            pstYuv420->buffer = NULL;
        }
        pstYuv420 = list_next(g_vv_args->hndListImageYuv420, pstYuv420);
    }
    ret = list_destroy(g_vv_args->hndListImageYuv420);
    CHECK(ret == 0, -1, "error with %#x\n", ret);

    pthread_mutex_destroy(&g_vv_args->mutex);
    pthread_mutex_destroy(&g_vv_args->mutex1);
    
    mem_free(g_vv_args);
    g_vv_args = NULL;
    
    return 0;
}



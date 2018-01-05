#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include "hi_comm.h"
#include "sal_av.h"
#include "sal_md.h"
#include "sal_debug.h"
#include "sal_config.h"


#define IVE_ALIGN 16
#define MAX_BLOCK_NUM (4*4)
#define MD_IMAGE_NUM 2
#define MMZ_ZONE_NAME "MotionDetect"

typedef struct sal_md_args
{
    int enable;
    int channel;
    int widht;
    int height;
    int rotate;
    int block_h;
    int block_v;
    sal_md_cb report[2];
    int precent[MAX_BLOCK_NUM];
    int pixel_num; // numbers of motion pixel

    IVE_SRC_IMAGE_S astImg[MD_IMAGE_NUM];
    IVE_DST_IMAGE_S stDstImage;
    IVE_DST_MEM_INFO_S stBlob;
    int running;
    pthread_t pid;
    pthread_mutex_t mutex;
}
sal_md_args;

static sal_md_args* g_md_args = NULL;

/*
static int show_result(IVE_CCBLOB_S* pstCCBLOB, unsigned long long sum)
{
    int i = 0;
    DBG("u16CurAreaThr: %hu, s8LabelStatus: %d, u8RegionNum: %d, sum: %llu, motion_precent: %d\n", pstCCBLOB->u16CurAreaThr,
                        pstCCBLOB->s8LabelStatus, pstCCBLOB->u8RegionNum, sum, g_args.motion_precent);

    for (i = 0; i < pstCCBLOB->u8RegionNum; i++)
    {
        DBG("u32Area: %u\n", pstCCBLOB->astRegion[i].u32Area);
        DBG("u16Left: %hu\n", pstCCBLOB->astRegion[i].u16Left);
        DBG("u16Right: %hu\n", pstCCBLOB->astRegion[i].u16Right);
        DBG("u16Top: %hu\n", pstCCBLOB->astRegion[i].u16Top);
        DBG("u16Bottom: %hu\n", pstCCBLOB->astRegion[i].u16Bottom);
    }

    return 0;
}
*/

static int md_calc_block(IVE_SRC_IMAGE_S* pstCur, IVE_CCBLOB_S* pstCCBLOB)
{
    int bound_value_x = pstCur->u16Width / g_md_args->block_h;
    if (pstCur->u16Width % g_md_args->block_h != 0)
    {
        bound_value_x++;
    }
    int bound_value_y = pstCur->u16Height / g_md_args->block_v;
    if (pstCur->u16Height % g_md_args->block_v != 0)
    {
        bound_value_y++;
    }

    int m = 0;
    int k = 0;
    int i = 0;
    int per_block_area[MAX_BLOCK_NUM];
    memset(per_block_area, 0, sizeof(per_block_area));
    g_md_args->pixel_num = 0;

    for(i = 0; i < pstCCBLOB->u8RegionNum; i++)
    {
        g_md_args->pixel_num += pstCCBLOB->astRegion[i].u32Area;
        int index = 0;
        for (k = 0; k < pstCur->u16Height; k += bound_value_y) // 找出连通区域所属的块
        {
            for (m = 0; m < pstCur->u16Width; m += bound_value_x)
            {
                //DBG("%d)i(%d~%d), j(%d~%d)\n", n, k, k+bound_value_y, m, m+bound_value_x);
                // 判断两个矩形是否有重叠区域
                if (((pstCCBLOB->astRegion[i].u16Left > m && pstCCBLOB->astRegion[i].u16Left < (m+bound_value_x))
                    || (pstCCBLOB->astRegion[i].u16Right > m && pstCCBLOB->astRegion[i].u16Right < (m+bound_value_x))
                    || (pstCCBLOB->astRegion[i].u16Left < m && pstCCBLOB->astRegion[i].u16Right > (m+bound_value_x)))
                    &&
                    ((pstCCBLOB->astRegion[i].u16Top > k && pstCCBLOB->astRegion[i].u16Top < (k+bound_value_y))
                    || (pstCCBLOB->astRegion[i].u16Bottom > k && pstCCBLOB->astRegion[i].u16Bottom < (k+bound_value_y))
                    || (pstCCBLOB->astRegion[i].u16Top < k && pstCCBLOB->astRegion[i].u16Bottom > (k+bound_value_y))))
                {
                    // 计算重叠区域面积
                    int max_left = m;
                    if (pstCCBLOB->astRegion[i].u16Left > m)
                    {
                        max_left = pstCCBLOB->astRegion[i].u16Left;
                        //DBG("max_left: %d\n", max_left);
                    }
                    int min_right = m+bound_value_x;
                    if (pstCCBLOB->astRegion[i].u16Right < (m+bound_value_x))
                    {
                        min_right = pstCCBLOB->astRegion[i].u16Right;
                        //DBG("min_right: %d\n", min_right);
                    }
                    int max_top = k;
                    if (pstCCBLOB->astRegion[i].u16Top > k)
                    {
                        max_top = pstCCBLOB->astRegion[i].u16Top;
                        //DBG("max_top: %d\n", max_top);
                    }
                    int min_bottom = k+bound_value_y;
                    if (pstCCBLOB->astRegion[i].u16Bottom < (k+bound_value_y))
                    {
                        min_bottom = pstCCBLOB->astRegion[i].u16Bottom;
                        //DBG("min_bottom: %d\n", min_bottom);
                    }
                    int area = (min_right - max_left)*(min_bottom - max_top);
                    per_block_area[index++] += area;
                    //DBG("max_left: %d, min_right: %d, max_top: %d, min_bottom: %d\n", max_left, min_right, max_top, min_bottom);
                    //DBG("overlap area: w(%d)xh(%d)=%d\n", (min_right - max_left), (min_bottom - max_top), area);
                }
            }
        }
    }

    int per_block_sum_area = (pstCur->u16Width*pstCur->u16Height) / (g_md_args->block_h*g_md_args->block_v);
    for (i = 0; i < MAX_BLOCK_NUM; i++)
    {
        g_md_args->precent[i] = (per_block_area[i]*10000) / per_block_sum_area;
    }

    return 0;
}

static HI_S32 md_dma_image(VIDEO_FRAME_INFO_S *pstFrameInfo,IVE_DST_IMAGE_S *pstDst,HI_BOOL bInstant)
{
    HI_S32 s32Ret;
    IVE_HANDLE hIveHandle;
    IVE_SRC_DATA_S stSrcData;
    IVE_DST_DATA_S stDstData;
    IVE_DMA_CTRL_S stCtrl = {IVE_DMA_MODE_DIRECT_COPY,0};
    HI_BOOL bFinish = HI_FALSE;
    HI_BOOL bBlock = HI_TRUE;

    //fill src
    stSrcData.pu8VirAddr = (HI_U8*)pstFrameInfo->stVFrame.pVirAddr[0];
    stSrcData.u32PhyAddr = pstFrameInfo->stVFrame.u32PhyAddr[0];
    stSrcData.u16Width   = (HI_U16)pstFrameInfo->stVFrame.u32Width;
    stSrcData.u16Height  = (HI_U16)pstFrameInfo->stVFrame.u32Height;
    stSrcData.u16Stride  = (HI_U16)pstFrameInfo->stVFrame.u32Stride[0];

    //fill dst
    stDstData.pu8VirAddr = pstDst->pu8VirAddr[0];
    stDstData.u32PhyAddr = pstDst->u32PhyAddr[0];
    stDstData.u16Width   = pstDst->u16Width;
    stDstData.u16Height  = pstDst->u16Height;
    stDstData.u16Stride  = pstDst->u16Stride[0];

    s32Ret = HI_MPI_IVE_DMA(&hIveHandle,&stSrcData,&stDstData,&stCtrl,bInstant);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    if (HI_TRUE == bInstant)
    {
        s32Ret = HI_MPI_IVE_Query(hIveHandle,&bFinish,bBlock);
        while(HI_ERR_IVE_QUERY_TIMEOUT == s32Ret)
        {
            usleep(100);
            s32Ret = HI_MPI_IVE_Query(hIveHandle,&bFinish,bBlock);
        }
        if (HI_SUCCESS != s32Ret)
        {
            DBG("HI_MPI_IVE_Query fail,Error(%#x)\n",s32Ret);
           return s32Ret;
        }
    }

    return HI_SUCCESS;
}

static HI_VOID * md_proc(HI_VOID * pArgs)
{
    prctl(PR_SET_NAME, __FUNCTION__);
    HI_S32 s32Ret = HI_FAILURE;
    HI_S32 s32GetFrameMilliSec = 2000;
    MD_CHN MdChn = 0;
    HI_BOOL bInstant = HI_TRUE;
    HI_S32 s32CurIdx = 0;
    HI_BOOL bFirstFrm = HI_TRUE;

    VIDEO_FRAME_INFO_S stFrmInfo;
    memset(&stFrmInfo, 0, sizeof(stFrmInfo));
    VPSS_GRP VpssGrp = 0;
    VPSS_CHN VpssChn = g_md_args->channel; //md chn

    while (g_md_args->running)
    {
        s32Ret = HI_MPI_VPSS_GetChnFrame(VpssGrp, VpssChn, &stFrmInfo, s32GetFrameMilliSec);
        if (HI_SUCCESS != s32Ret)
        {
            DBG("HI_MPI_VPSS_GetChnFrame fail,Error(%#x)\n" ,s32Ret);
            continue;
        }

        s32Ret = md_dma_image(&stFrmInfo,&g_md_args->astImg[s32CurIdx],bInstant);
        if (HI_SUCCESS != s32Ret)
        {
            DBG("md_dma_image fail,Error(%#x)\n", s32Ret);
            goto RELEASE;
        }

        if (HI_TRUE != bFirstFrm)
        {
            s32Ret = HI_IVS_MD_Process(MdChn,&g_md_args->astImg[s32CurIdx],&g_md_args->astImg[1 - s32CurIdx], NULL, &g_md_args->stBlob);
            if (HI_SUCCESS != s32Ret)
            {
                DBG("HI_IVS_MD_Process fail,Error(%#x)\n",s32Ret);
                goto RELEASE;
            }
            IVE_CCBLOB_S* pstCCBLOB = (IVE_CCBLOB_S *)g_md_args->stBlob.pu8VirAddr;
            pthread_mutex_lock(&g_md_args->mutex);

            s32Ret = md_calc_block(&g_md_args->astImg[s32CurIdx], pstCCBLOB);
            CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);

            pthread_mutex_unlock(&g_md_args->mutex);

            unsigned i = 0;
            for (i = 0; i < sizeof(g_md_args->report)/sizeof(g_md_args->report[0]); i++)
            {
                if (g_md_args->report[i] != NULL)
                {
                    g_md_args->report[i](g_md_args->precent, g_md_args->block_h*g_md_args->block_v, g_md_args->pixel_num);
                }
            }
        }
        else
        {
            bFirstFrm = HI_FALSE;
        }
        //Change reference and current frame index
        s32CurIdx = 1 - s32CurIdx;

RELEASE:
        s32Ret = HI_MPI_VPSS_ReleaseChnFrame(VpssGrp, VpssChn, &stFrmInfo);
        CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);

     }

     return HI_NULL;
}

static int md_parse_cfg(MD_ATTR_S* pstMdAttr)
{
    FILE* fp = fopen("/mnt/nand/md.cfg", "r");
    if (fp)
    {
        char buf[512];
        memset(buf, 0, sizeof(buf));
        int i = 0;

        fread(buf, sizeof(buf), 1, fp);
        const char* cmd[] = {"SadThr:", "InitAreaThr:"};

        char* find = strstr(buf, cmd[i]);
        if (find)
        {
            pstMdAttr->u16SadThr = strtold(find+strlen(cmd[i++]), NULL);
        }
        find = strstr(buf, cmd[i]);
        if (find)
        {
            pstMdAttr->stCclCtrl.u16InitAreaThr = strtold(find+strlen(cmd[i++]), NULL);
        }

        fclose(fp);
    }

    return 0;
}

static HI_S32 md_start()
{
    HI_S32 s32Ret = HI_SUCCESS;

    MD_CHN MdChn = 0;
    MD_ATTR_S stMdAttr;
    memset(&stMdAttr, 0, sizeof(stMdAttr));
    stMdAttr.enAlgMode = MD_ALG_MODE_BG;
    stMdAttr.enSadMode = IVE_SAD_MODE_MB_4X4;
    stMdAttr.enSadOutCtrl = IVE_SAD_OUT_CTRL_THRESH;
    stMdAttr.u16SadThr = 128;
    stMdAttr.u16Width = g_md_args->widht;
    stMdAttr.u16Height = g_md_args->height;
    stMdAttr.stAddCtrl.u0q16X = 32768;
    stMdAttr.stAddCtrl.u0q16Y = 32768;
    stMdAttr.stCclCtrl.u16InitAreaThr = 4;
    stMdAttr.stCclCtrl.u16Step = 4;

    s32Ret = md_parse_cfg(&stMdAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = HI_IVS_MD_CreateChn(MdChn, &stMdAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    g_md_args->running = 1;
    s32Ret = pthread_create(&g_md_args->pid, NULL, md_proc, NULL);
    CHECK(s32Ret == 0, HI_FAILURE, "Error with %s.\n", strerror(errno));

    return s32Ret;
}

static int md_stop()
{
    HI_S32 s32Ret = HI_SUCCESS;
    MD_CHN MdChn = 0;

    if (g_md_args->running)
    {
        g_md_args->running = 0;
        pthread_join(g_md_args->pid, HI_NULL);
    }

    s32Ret = HI_IVS_MD_DestroyChn(MdChn);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return 0;
}

static int md_vpss_set_chn(VPSS_GRP VpssGrp, VPSS_CHN VpssChn,VPSS_CHN_ATTR_S *pstVpssChnAttr,VPSS_CHN_MODE_S *pstVpssChnMode,VPSS_EXT_CHN_ATTR_S *pstVpssExtChnAttr)
{
    HI_S32 s32Ret;

    if (VpssChn < VPSS_MAX_PHY_CHN_NUM)
    {
        s32Ret = HI_MPI_VPSS_SetChnAttr(VpssGrp, VpssChn, pstVpssChnAttr);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    }
    else
    {
        s32Ret = HI_MPI_VPSS_SetExtChnAttr(VpssGrp, VpssChn, pstVpssExtChnAttr);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    }

    if (VpssChn < VPSS_MAX_PHY_CHN_NUM && HI_NULL != pstVpssChnMode)
    {
        if (VpssChn != 0 && g_md_args->rotate)
        {
            HI_U32 temp = pstVpssChnMode->u32Width;
            pstVpssChnMode->u32Width = pstVpssChnMode->u32Height;
            pstVpssChnMode->u32Height = temp;
        }
        s32Ret = HI_MPI_VPSS_SetChnMode(VpssGrp, VpssChn, pstVpssChnMode);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    }

    if (g_md_args->rotate && VpssChn < VPSS_MAX_PHY_CHN_NUM)
    {
        s32Ret = HI_MPI_VPSS_SetRotate(VpssGrp, VpssChn, ROTATE_90);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    }

    s32Ret = HI_MPI_VPSS_EnableChn(VpssGrp, VpssChn);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return HI_SUCCESS;
}


static int md_enable_chn()
{
    VPSS_GRP VpssGrp = 0;
    HI_S32 s32Ret = HI_SUCCESS;

    VPSS_CHN_MODE_S stVpssChnMode;
    memset(&stVpssChnMode, 0, sizeof(stVpssChnMode));

    stVpssChnMode.enChnMode       = VPSS_CHN_MODE_USER;
    stVpssChnMode.bDouble         = HI_FALSE;
    stVpssChnMode.enPixelFormat   = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    stVpssChnMode.u32Width        = g_md_args->widht;
    stVpssChnMode.u32Height       = g_md_args->height;
    stVpssChnMode.enCompressMode  = COMPRESS_MODE_NONE;

    VPSS_CHN_ATTR_S stVpssChnAttr;
    memset(&stVpssChnAttr, 0, sizeof(stVpssChnAttr));

    sal_stream_s stream;
    memset(&stream, 0, sizeof(stream));
    s32Ret = sal_video_args_get(0, &stream);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    stVpssChnAttr.s32SrcFrameRate = stream.framerate; // vin framerate
    stVpssChnAttr.s32DstFrameRate = (stream.framerate > 5) ? 5 : stream.framerate;

    HI_U32 u32Depth = 1;
    s32Ret = HI_MPI_VPSS_SetDepth(VpssGrp, g_md_args->channel, u32Depth);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = md_vpss_set_chn(VpssGrp, g_md_args->channel, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return s32Ret;
}

static HI_U16 md_calc_stride(HI_U16 u16Width, HI_U8 u8Align)
{
    return (u16Width + (u8Align - u16Width%u8Align)%u8Align);
}

static HI_S32 md_create_image(IVE_IMAGE_S *pstImg,IVE_IMAGE_TYPE_E enType,HI_U16 u16Width,HI_U16 u16Height)
{
    HI_U32 u32Size = 0;
    HI_S32 s32Ret;
    if (NULL == pstImg)
    {
        DBG("pstImg is null\n");
        return HI_FAILURE;
    }

    pstImg->enType = enType;
    pstImg->u16Width = u16Width;
    pstImg->u16Height = u16Height;
    pstImg->u16Stride[0] = md_calc_stride(pstImg->u16Width,IVE_ALIGN);

    switch(enType)
    {
    case IVE_IMAGE_TYPE_U8C1:
    case IVE_IMAGE_TYPE_S8C1:
        {
            u32Size = pstImg->u16Stride[0] * pstImg->u16Height;
            s32Ret = HI_MPI_SYS_MmzAlloc(&pstImg->u32PhyAddr[0], (void**)&pstImg->pu8VirAddr[0], MMZ_ZONE_NAME, HI_NULL, u32Size);
            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
        }
        break;
    case IVE_IMAGE_TYPE_YUV420SP:
        {
            u32Size = pstImg->u16Stride[0] * pstImg->u16Height * 3 / 2;
            s32Ret = HI_MPI_SYS_MmzAlloc(&pstImg->u32PhyAddr[0], (void**)&pstImg->pu8VirAddr[0], MMZ_ZONE_NAME, HI_NULL, u32Size);
            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

            pstImg->u16Stride[1] = pstImg->u16Stride[0];
            pstImg->u32PhyAddr[1] = pstImg->u32PhyAddr[0] + pstImg->u16Stride[0] * pstImg->u16Height;
            pstImg->pu8VirAddr[1] = pstImg->pu8VirAddr[0] + pstImg->u16Stride[0] * pstImg->u16Height;

        }
        break;
    case IVE_IMAGE_TYPE_YUV422SP:
        {
            u32Size = pstImg->u16Stride[0] * pstImg->u16Height * 2;
            s32Ret = HI_MPI_SYS_MmzAlloc(&pstImg->u32PhyAddr[0], (void**)&pstImg->pu8VirAddr[0], MMZ_ZONE_NAME, HI_NULL, u32Size);
            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

            pstImg->u16Stride[1] = pstImg->u16Stride[0];
            pstImg->u32PhyAddr[1] = pstImg->u32PhyAddr[0] + pstImg->u16Stride[0] * pstImg->u16Height;
            pstImg->pu8VirAddr[1] = pstImg->pu8VirAddr[0] + pstImg->u16Stride[0] * pstImg->u16Height;

        }
        break;
    case IVE_IMAGE_TYPE_YUV420P:
        break;
    case IVE_IMAGE_TYPE_YUV422P:
        break;
    case IVE_IMAGE_TYPE_S8C2_PACKAGE:
        break;
    case IVE_IMAGE_TYPE_S8C2_PLANAR:
        break;
    case IVE_IMAGE_TYPE_S16C1:
    case IVE_IMAGE_TYPE_U16C1:
        {

            u32Size = pstImg->u16Stride[0] * pstImg->u16Height * sizeof(HI_U16);
            s32Ret = HI_MPI_SYS_MmzAlloc(&pstImg->u32PhyAddr[0], (void**)&pstImg->pu8VirAddr[0], MMZ_ZONE_NAME, HI_NULL, u32Size);
            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
        }
        break;
    case IVE_IMAGE_TYPE_U8C3_PACKAGE:
        {
            u32Size = pstImg->u16Stride[0] * pstImg->u16Height * 3;
            s32Ret = HI_MPI_SYS_MmzAlloc(&pstImg->u32PhyAddr[0], (void**)&pstImg->pu8VirAddr[0], MMZ_ZONE_NAME, HI_NULL, u32Size);
            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

            pstImg->pu8VirAddr[1] = pstImg->pu8VirAddr[0] +1;
            pstImg->pu8VirAddr[2] = pstImg->pu8VirAddr[1] + 1;
            pstImg->u32PhyAddr[1] = pstImg->u32PhyAddr[0] + 1;
            pstImg->u32PhyAddr[2] = pstImg->u32PhyAddr[1] + 1;
            pstImg->u16Stride[1] = pstImg->u16Stride[0];
            pstImg->u16Stride[2] = pstImg->u16Stride[0];
        }
        break;
    case IVE_IMAGE_TYPE_U8C3_PLANAR:
        break;
    case IVE_IMAGE_TYPE_S32C1:
    case IVE_IMAGE_TYPE_U32C1:
        {
            u32Size = pstImg->u16Stride[0] * pstImg->u16Height * sizeof(HI_U32);
            s32Ret = HI_MPI_SYS_MmzAlloc(&pstImg->u32PhyAddr[0], (void**)&pstImg->pu8VirAddr[0], MMZ_ZONE_NAME, HI_NULL, u32Size);
            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
        }
        break;
    case IVE_IMAGE_TYPE_S64C1:
    case IVE_IMAGE_TYPE_U64C1:
        {
            u32Size = pstImg->u16Stride[0] * pstImg->u16Height * sizeof(HI_U64);
            s32Ret = HI_MPI_SYS_MmzAlloc(&pstImg->u32PhyAddr[0], (void**)&pstImg->pu8VirAddr[0], MMZ_ZONE_NAME, HI_NULL, u32Size);
            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
        }
        break;
    default:
        break;

    }

    return HI_SUCCESS;
}

static HI_S32 md_create_memInfo(IVE_MEM_INFO_S*pstMemInfo,HI_U32 u32Size)
{
    HI_S32 s32Ret;
    pstMemInfo->u32Size = u32Size;

    s32Ret = HI_MPI_SYS_MmzAlloc(&pstMemInfo->u32PhyAddr, (void**)&pstMemInfo->pu8VirAddr, MMZ_ZONE_NAME, HI_NULL, u32Size);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return HI_SUCCESS;
}

static int md_alloc_mem()
{
    HI_S32 s32Ret = HI_SUCCESS;
    int i = 0;

    //alloc mem for MD image
    for (i = 0;i < MD_IMAGE_NUM;i++)
    {
        s32Ret = md_create_image(&g_md_args->astImg[i], IVE_IMAGE_TYPE_U8C1, g_md_args->widht, g_md_args->height);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    }
/*
    s32Ret = md_create_image(&g_md_args->stDstImage, IVE_IMAGE_TYPE_U16C1, stSize.u32Width/4, stSize.u32Height/4);
    CHECK(s32Ret != HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
*/
    //alloc mem for store MD result information
    s32Ret = md_create_memInfo(&g_md_args->stBlob, sizeof(IVE_CCBLOB_S));
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return 0;
}


static HI_S32 md_vpss_disable_chn(VPSS_GRP VpssGrp, VPSS_CHN VpssChn)
{
    HI_S32 s32Ret;

    s32Ret = HI_MPI_VPSS_DisableChn(VpssGrp, VpssChn);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return HI_SUCCESS;
}

int sal_md_init(int block_h, int block_v)
{
    CHECK(NULL == g_md_args, HI_FAILURE, "reinit error, please exit first.\n");
    CHECK((block_h*block_v) <= MAX_BLOCK_NUM, HI_FAILURE, "block_h[%d] * block_v[%d] is invalid.\n", block_h, block_v);

    HI_S32 s32Ret = HI_SUCCESS;

    g_md_args = (sal_md_args*)malloc(sizeof(sal_md_args));
    CHECK(g_md_args != NULL, HI_FAILURE, "malloc %d bytes failed.\n", sizeof(sal_md_args));

    memset(g_md_args, 0, sizeof(sal_md_args));
    pthread_mutex_init(&g_md_args->mutex, NULL);
    g_md_args->enable = g_config.md.enable;
    g_md_args->channel = g_config.md.channel;
    g_md_args->widht = g_config.md.widht;
    g_md_args->height = g_config.md.height;
    g_md_args->block_h = block_h;
    g_md_args->block_v = block_v;

    ROTATE_E enRotate = ROTATE_NONE;
    s32Ret = HI_MPI_VPSS_GetRotate(0, 0, &enRotate); //get vpss chn0 rotate attr
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    g_md_args->rotate = (enRotate == ROTATE_90 || enRotate == ROTATE_270) ? 1 : 0;
    if (g_md_args->rotate)
    {
        int swap = g_md_args->widht;
        g_md_args->widht = g_md_args->height;
        g_md_args->height = swap;
    }

    s32Ret = md_enable_chn();
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = md_alloc_mem();
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = HI_IVS_MD_Init();
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = md_start();
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return s32Ret;
}

int sal_md_exit()
{
    CHECK(NULL != g_md_args, HI_FAILURE, "module not inited.\n");

    HI_S32 i = 0;
    HI_S32 s32Ret = HI_SUCCESS;

    s32Ret = md_stop();
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    DBG("md_stop done.\n");

    s32Ret = md_vpss_disable_chn(0, g_md_args->channel);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    DBG("md_vpss_disable_chn done.\n");

    for (i = 0; i < MD_IMAGE_NUM; i++)
    {
        s32Ret = HI_MPI_SYS_MmzFree(g_md_args->astImg[i].u32PhyAddr[0],g_md_args->astImg[i].pu8VirAddr[0]);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
        DBG("HI_MPI_SYS_MmzFree[%d] done.\n", i);
    }

    //HI_MPI_SYS_MmzFree(g_md_args->stDstImage.u32PhyAddr[0], g_md_args->stDstImage.pu8VirAddr[0]);
    s32Ret = HI_MPI_SYS_MmzFree(g_md_args->stBlob.u32PhyAddr,g_md_args->stBlob.pu8VirAddr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    DBG("HI_MPI_SYS_MmzFree done.\n");

    s32Ret = HI_IVS_MD_Exit();
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    DBG("HI_IVS_MD_Exit done.\n");

    pthread_mutex_destroy(&g_md_args->mutex);
    free(g_md_args);
    g_md_args = NULL;

    return 0;
}

int sal_md_set(int block_h, int block_v)
{
    CHECK(NULL != g_md_args, HI_FAILURE, "module not inited.\n");
    CHECK((block_h*block_v) <= MAX_BLOCK_NUM, HI_FAILURE, "block_h[%d] * block_v[%d] is invalid.\n", block_h, block_v);

    pthread_mutex_lock(&g_md_args->mutex);

    g_md_args->block_h = block_h;
    g_md_args->block_v = block_v;

    pthread_mutex_unlock(&g_md_args->mutex);

    return 0;
}

int sal_md_cb_set(sal_md_cb cb)
{
    CHECK(NULL != g_md_args, HI_FAILURE, "module not inited.\n");
    CHECK(cb, HI_FAILURE, "invalid parameter with: %#x\n", cb);
    CHECK(!g_md_args->report[0] || !g_md_args->report[1], HI_FAILURE, "invalid parameter with: %#x\n", g_md_args->report);

    pthread_mutex_lock(&g_md_args->mutex);

    unsigned i = 0;
    for (i = 0; i < sizeof(g_md_args->report)/sizeof(g_md_args->report[0]); i++)
    {
        if (!g_md_args->report[i])
        {
            g_md_args->report[i] = cb;
            break;
        }
    }

    pthread_mutex_unlock(&g_md_args->mutex);

    return 0;
}


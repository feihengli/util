#include "hi_comm.h"
#include "sal_cover.h"
#include "sal_debug.h"
#include "sal_av.h"

#define ENC_COVER_ALIGN(value, v)\
    ((value)/ (v) * (v))

static int enc_sal_cover_convert(sal_cover_s *param)
{
    int ret = -1;
    VI_CHN_ATTR_S stVIChnAttr;
    ret = HI_MPI_VI_GetChnAttr(0, &stVIChnAttr);
    CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", ret);

    sal_stream_s video;
    ret = sal_video_args_get(0, &video);
    CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", ret);

    DBG("vi resolution[%dx%d], encode resolution[%dx%d]\n", stVIChnAttr.stDestSize.u32Width, stVIChnAttr.stDestSize.u32Height, video.width, video.height);
    if (stVIChnAttr.stDestSize.u32Height*stVIChnAttr.stDestSize.u32Width != (HI_U32)video.height*video.width)
    {
        int x = param->posx*stVIChnAttr.stDestSize.u32Width/video.width;
        int y = param->posy*stVIChnAttr.stDestSize.u32Height/video.height;
        int w = param->width*stVIChnAttr.stDestSize.u32Width/video.width;
        int h = param->height*stVIChnAttr.stDestSize.u32Height/video.height;

        x = ENC_COVER_ALIGN(x, 4);
        y = ENC_COVER_ALIGN(y, 4);
        w = ENC_COVER_ALIGN(w, 4);
        h = ENC_COVER_ALIGN(h, 4);

        DBG("orginal xy[%d %d] wh[%d %d], after convert xy[%d %d] wh[%d %d]\n", param->posx, param->posy, param->width, param->height, x, y, w, h);
        param->posx = x;
        param->posy = y;
        param->width = w;
        param->height = h;
    }

    return 0;
}

int sal_cover_create(sal_cover_s *param)
{
    CHECK(param, HI_FAILURE, "Error with %#x.\n", param);

    int ret = -1;
    RGN_ATTR_S stRgnAttr;
    MPP_CHN_S stChn;
    RGN_CHN_ATTR_S stChnAttr;

    memset(&stRgnAttr, 0, sizeof(stRgnAttr));
    stRgnAttr.enType = COVER_RGN;

    ret = HI_MPI_RGN_Create(param->handle, &stRgnAttr);
    CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", ret);

    ret = enc_sal_cover_convert(param);
    CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", ret);

    memset(&stChn, 0, sizeof(stChn));
    stChn.enModId = HI_ID_VPSS;
    stChn.s32DevId = 0;
    stChn.s32ChnId = 0;

    memset(&stChnAttr, 0, sizeof(stChnAttr));
    stChnAttr.bShow = HI_TRUE;
    stChnAttr.enType = COVER_RGN;
    stChnAttr.unChnAttr.stCoverChn.enCoverType = AREA_RECT;
    stChnAttr.unChnAttr.stCoverChn.stRect.s32X = param->posx;
    stChnAttr.unChnAttr.stCoverChn.stRect.s32Y = param->posy;
    stChnAttr.unChnAttr.stCoverChn.stRect.u32Width = param->width;
    stChnAttr.unChnAttr.stCoverChn.stRect.u32Height = param->height;
    stChnAttr.unChnAttr.stCoverChn.u32Color = 0;
    stChnAttr.unChnAttr.stCoverChn.u32Layer = 0;

    ret = HI_MPI_RGN_AttachToChn(param->handle, &stChn, &stChnAttr);
    CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);

    return HI_SUCCESS;
}


int sal_cover_destory(sal_cover_s *param)
{
    CHECK(param, HI_FAILURE, "Error with %#x.\n", param);
    CHECK(param->handle != INVALID_REGION_HANDLE, HI_FAILURE, "Error with %#x.\n", param->handle);

    int ret = -1;
    MPP_CHN_S stChn;

    memset(&stChn, 0, sizeof(stChn));
    stChn.enModId = HI_ID_VPSS;
    stChn.s32DevId = 0;
    stChn.s32ChnId = 0;

    ret = HI_MPI_RGN_DetachFromChn(param->handle, &stChn);
    CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);

    ret = HI_MPI_RGN_Destroy(param->handle);
    CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);

    return HI_SUCCESS;
}



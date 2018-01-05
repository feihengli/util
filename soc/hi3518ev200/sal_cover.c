#include "hi_comm.h"
#include "sal_cover.h"
#include "sal_debug.h"

int sal_cover_create(sal_cover_s *param)
{
    CHECK(param, HI_FAILURE, "Error with %#x.\n", param);
    CHECK(param->handle != INVALID_REGION_HANDLE, HI_FAILURE, "Error with %#x.\n", param->handle);

    int ret;
    RGN_ATTR_S stRgnAttr;
    MPP_CHN_S stChn;
    RGN_CHN_ATTR_S stChnAttr;

    memset(&stRgnAttr, 0, sizeof(stRgnAttr));
    stRgnAttr.enType = COVER_RGN;

    ret = HI_MPI_RGN_Create(param->handle, &stRgnAttr);
    CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);

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

    int ret;
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



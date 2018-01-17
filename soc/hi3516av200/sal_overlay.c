#include "hi_comm.h"
#include "sal_overlay.h"
#include "sal_debug.h"

int sal_overlay_create(sal_overlay_s *param)
{
    CHECK(param, HI_FAILURE, "Error with %#x.\n", param);
    CHECK(param->handle != INVALID_REGION_HANDLE, HI_FAILURE, "Error with %#x.\n", param->handle);

    int ret = -1;
    RGN_ATTR_S stRgnAttr;
    memset(&stRgnAttr, 0, sizeof(stRgnAttr));
    stRgnAttr.enType = OVERLAY_RGN;
    stRgnAttr.unAttr.stOverlay.enPixelFmt = PIXEL_FORMAT_RGB_1555;
    stRgnAttr.unAttr.stOverlay.u32BgColor = 0x7c00;
    stRgnAttr.unAttr.stOverlay.stSize.u32Width  = param->width;
    stRgnAttr.unAttr.stOverlay.stSize.u32Height = param->height;
    stRgnAttr.unAttr.stOverlay.u32CanvasNum = 2;

    int MMZ = (param->width*param->height)/1024.0 + 0.5;
    MMZ *= 4; // 4k¶ÔÆë
    //DBG("width: %d, height: %d, MMZ: %d KB\n", param->width, param->height,MMZ);
    ret = HI_MPI_RGN_Create(param->handle, &stRgnAttr);
    CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", ret);

    MPP_CHN_S stChn;
    stChn.enModId = HI_ID_VENC;
    stChn.s32DevId = 0;
    stChn.s32ChnId = param->stream_id;

    RGN_CHN_ATTR_S stChnAttr;
    memset(&stChnAttr, 0, sizeof(stChnAttr));
    stChnAttr.bShow = HI_TRUE;
    stChnAttr.enType = OVERLAY_RGN;
    stChnAttr.unChnAttr.stOverlayChn.u32FgAlpha = 128;
    stChnAttr.unChnAttr.stOverlayChn.u32Layer = 0;
    stChnAttr.unChnAttr.stOverlayChn.stQpInfo.bAbsQp = HI_FALSE;
    stChnAttr.unChnAttr.stOverlayChn.stQpInfo.s32Qp  = 0;
    stChnAttr.unChnAttr.stOverlayChn.stQpInfo.bQpDisable = HI_FALSE;
    stChnAttr.unChnAttr.stOverlayChn.stInvertColor.bInvColEn = HI_FALSE;
    stChnAttr.unChnAttr.stOverlayChn.u32BgAlpha = 0;
    stChnAttr.unChnAttr.stOverlayChn.stInvertColor.enChgMod = LESSTHAN_LUM_THRESH;
    stChnAttr.unChnAttr.stOverlayChn.stInvertColor.u32LumThresh = 128;
    stChnAttr.unChnAttr.stOverlayChn.stPoint.s32X = param->posx;
    stChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y = param->posy;
    stChnAttr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.u32Width = 16 * param->factor;
    stChnAttr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.u32Height = 16 * param->factor;
    ret = HI_MPI_RGN_AttachToChn(param->handle, &stChn, &stChnAttr);
    CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", ret);

    param->bitmap_data = malloc(param->width * param->height * 2);
    CHECK(param->bitmap_data, HI_FAILURE, "Error with %#x.\n", param->bitmap_data);

    return HI_SUCCESS;
}

int sal_overlay_ex_create(sal_overlay_s *param)
{
    CHECK(param, HI_FAILURE, "Error with %#x.\n", param);
    CHECK(param->handle != INVALID_REGION_HANDLE, HI_FAILURE, "Error with %#x.\n", param->handle);

    int ret = -1;
    RGN_ATTR_S stRgnAttr;
    memset(&stRgnAttr, 0, sizeof(stRgnAttr));
    stRgnAttr.enType = OVERLAYEX_RGN;
    stRgnAttr.unAttr.stOverlayEx.enPixelFmt = PIXEL_FORMAT_RGB_1555;
    stRgnAttr.unAttr.stOverlayEx.u32BgColor = 0x7c00;
    stRgnAttr.unAttr.stOverlayEx.stSize.u32Width  = param->width;
    stRgnAttr.unAttr.stOverlayEx.stSize.u32Height = param->height;

    int MMZ = (param->width*param->height)/1024.0 + 0.5;
    MMZ *= 4; // 4k¶ÔÆë
    //DBG("width: %d, height: %d, MMZ: %d KB\n", param->width, param->height,MMZ);
    ret = HI_MPI_RGN_Create(param->handle, &stRgnAttr);
    CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", ret);

    MPP_CHN_S stChn;
    stChn.enModId = HI_ID_VENC;
    stChn.s32DevId = 0;
    stChn.s32ChnId = param->stream_id;

    RGN_CHN_ATTR_S stChnAttr;
    memset(&stChnAttr, 0, sizeof(stChnAttr));
    stChnAttr.bShow = HI_TRUE;
    stChnAttr.enType = OVERLAYEX_RGN;
    stChnAttr.unChnAttr.stOverlayExChn.u32FgAlpha = 128;
    stChnAttr.unChnAttr.stOverlayExChn.u32Layer = 0;
    //stChnAttr.unChnAttr.stOverlayExChn.stQpInfo.bAbsQp = HI_FALSE;
    //stChnAttr.unChnAttr.stOverlayExChn.stQpInfo.s32Qp  = 0;
    //stChnAttr.unChnAttr.stOverlayExChn.stQpInfo.bQpDisable = HI_FALSE;
    //stChnAttr.unChnAttr.stOverlayExChn.stInvertColor.bInvColEn = HI_FALSE;
    stChnAttr.unChnAttr.stOverlayExChn.u32BgAlpha = 0;
    //stChnAttr.unChnAttr.stOverlayExChn.stInvertColor.enChgMod = LESSTHAN_LUM_THRESH;
    //stChnAttr.unChnAttr.stOverlayExChn.stInvertColor.u32LumThresh = 128;
    stChnAttr.unChnAttr.stOverlayExChn.stPoint.s32X = param->posx;
    stChnAttr.unChnAttr.stOverlayExChn.stPoint.s32Y = param->posy;
    //stChnAttr.unChnAttr.stOverlayExChn.stInvertColor.stInvColArea.u32Width = 16 * param->factor;
    //stChnAttr.unChnAttr.stOverlayExChn.stInvertColor.stInvColArea.u32Height = 16 * param->factor;
    ret = HI_MPI_RGN_AttachToChn(param->handle, &stChn, &stChnAttr);
    CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", ret);

    param->bitmap_data = malloc(param->width * param->height * 2);
    CHECK(param->bitmap_data, HI_FAILURE, "Error with %#x.\n", param->bitmap_data);

    return HI_SUCCESS;
}

int sal_overlay_destory(sal_overlay_s *param)
{
    CHECK(param, HI_FAILURE, "Error with %#x.\n", param);
    CHECK(INVALID_REGION_HANDLE != param->handle, -1,"invalid handle %#x.\n", param->handle);

    int ret = -1;
    MPP_CHN_S stChn;
    stChn.enModId = HI_ID_VENC;
    stChn.s32DevId = 0;
    stChn.s32ChnId = param->stream_id;
    ret = HI_MPI_RGN_DetachFromChn(param->handle, &stChn);
    CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", ret);

    ret = HI_MPI_RGN_Destroy(param->handle);
    CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", ret);

    param->handle = INVALID_REGION_HANDLE;
    if (param->bitmap_data)
    {
        free(param->bitmap_data);
        param->bitmap_data = NULL;
    }

    return 0;
}

int sal_overlay_refresh_bitmap(sal_overlay_s *param)
{
    CHECK(param, HI_FAILURE, "Error with %#x.\n", param);
    CHECK(INVALID_REGION_HANDLE != param->handle, -1,"invalid handle %#x.\n", param->handle);

    int ret = -1;

    BITMAP_S bitmap;
    bitmap.enPixelFormat = PIXEL_FORMAT_RGB_1555;
    bitmap.u32Width = param->width;
    bitmap.u32Height = param->height;
    bitmap.pData = param->bitmap_data;

    ret = HI_MPI_RGN_SetBitMap(param->handle, &bitmap);
    CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", ret);

    return ret;
}


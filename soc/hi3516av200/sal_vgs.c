#include "hi_comm.h"
#include "sal_debug.h"
#include "sal_vgs.h"

int sal_vgs_draw_rectangle(VIDEO_FRAME_INFO_S* pstOutFrameInfo, unsigned int u32Thick, unsigned int u32Color
                            , int x, int y, int width, int height)
{
    //CHECK(pstOutFrameInfo, HI_FAILURE, "invalid parameter with %#x\n", pstOutFrameInfo);
    
    HI_S32  s32Ret = HI_FAILURE;
    VGS_HANDLE hVgsHandle = -1;
    VGS_TASK_ATTR_S stVgsTaskAttr;
    memset(&stVgsTaskAttr, 0, sizeof(VGS_TASK_ATTR_S));
    VGS_DRAW_LINE_S astDrawLine[4];
    memset(astDrawLine, 0, sizeof(astDrawLine));
    unsigned int i = 0;
    //unsigned int u32Thick = 2; //线宽
    //unsigned int u32Color = 0x0000FF00;//后三个字节分别为RGB
    
    //矩形上边
    astDrawLine[i].stStartPoint.s32X = x;
    astDrawLine[i].stStartPoint.s32Y = y;
    astDrawLine[i].stEndPoint.s32X = x+width;
    astDrawLine[i].stEndPoint.s32Y = y;
    astDrawLine[i].u32Thick = u32Thick;
    astDrawLine[i].u32Color = u32Color;
    i++;
    
    //矩形下边
    astDrawLine[i].stStartPoint.s32X = x;
    astDrawLine[i].stStartPoint.s32Y = y+height;
    astDrawLine[i].stEndPoint.s32X = x+width;
    astDrawLine[i].stEndPoint.s32Y = y+height;
    astDrawLine[i].u32Thick = u32Thick;
    astDrawLine[i].u32Color = u32Color;
    i++;
    
    //矩形左边
    astDrawLine[i].stStartPoint.s32X = x;
    astDrawLine[i].stStartPoint.s32Y = y;
    astDrawLine[i].stEndPoint.s32X = x;
    astDrawLine[i].stEndPoint.s32Y = y+height;
    astDrawLine[i].u32Thick = u32Thick;
    astDrawLine[i].u32Color = u32Color;
    i++;
    
    //矩形右边
    astDrawLine[i].stStartPoint.s32X = x+width;
    astDrawLine[i].stStartPoint.s32Y = y;
    astDrawLine[i].stEndPoint.s32X = x+width;
    astDrawLine[i].stEndPoint.s32Y = y+height;
    astDrawLine[i].u32Thick = u32Thick;
    astDrawLine[i].u32Color = u32Color;
    i++;

    s32Ret = HI_MPI_VGS_BeginJob(&hVgsHandle);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    memcpy(&stVgsTaskAttr.stImgIn, pstOutFrameInfo, sizeof(VIDEO_FRAME_INFO_S));
    memcpy(&stVgsTaskAttr.stImgOut, pstOutFrameInfo, sizeof(VIDEO_FRAME_INFO_S));

    s32Ret = HI_MPI_VGS_AddDrawLineTaskArray(hVgsHandle, &stVgsTaskAttr, astDrawLine, 4);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = HI_MPI_VGS_EndJob(hVgsHandle);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return 0;
}

int sal_vgs_draw_rectangle1(VIDEO_FRAME_INFO_S* pstOutFrameInfo, unsigned int u32Thick, unsigned int u32Color
                           , rectangle_s* pstArectangle, int num)
{
    //CHECK(pstOutFrameInfo, HI_FAILURE, "invalid parameter with %#x\n", pstOutFrameInfo);

    HI_S32  s32Ret = HI_FAILURE;
    VGS_HANDLE hVgsHandle = -1;
    VGS_TASK_ATTR_S stVgsTaskAttr;
    memset(&stVgsTaskAttr, 0, sizeof(VGS_TASK_ATTR_S));
    VGS_DRAW_LINE_S* pstADrawLine = (VGS_DRAW_LINE_S*)malloc(sizeof(VGS_DRAW_LINE_S)*num*4);
    memset(pstADrawLine, 0, sizeof(VGS_DRAW_LINE_S)*num*4);
    unsigned int i = 0;
    unsigned int j = 0;
    for (j = 0; j < num; j++)
    {
        //矩形上边
        pstADrawLine[i].stStartPoint.s32X = pstArectangle[j].x;
        pstADrawLine[i].stStartPoint.s32Y = pstArectangle[j].y;
        pstADrawLine[i].stEndPoint.s32X = pstArectangle[j].x+pstArectangle[j].width;
        pstADrawLine[i].stEndPoint.s32Y = pstArectangle[j].y;
        pstADrawLine[i].u32Thick = u32Thick;
        pstADrawLine[i].u32Color = u32Color;
        i++;

        //矩形下边
        pstADrawLine[i].stStartPoint.s32X = pstArectangle[j].x;
        pstADrawLine[i].stStartPoint.s32Y = pstArectangle[j].y+pstArectangle[j].height;
        pstADrawLine[i].stEndPoint.s32X = pstArectangle[j].x+pstArectangle[j].width;
        pstADrawLine[i].stEndPoint.s32Y = pstArectangle[j].y+pstArectangle[j].height;
        pstADrawLine[i].u32Thick = u32Thick;
        pstADrawLine[i].u32Color = u32Color;
        i++;

        //矩形左边
        pstADrawLine[i].stStartPoint.s32X = pstArectangle[j].x;
        pstADrawLine[i].stStartPoint.s32Y = pstArectangle[j].y;
        pstADrawLine[i].stEndPoint.s32X = pstArectangle[j].x;
        pstADrawLine[i].stEndPoint.s32Y = pstArectangle[j].y+pstArectangle[j].height;
        pstADrawLine[i].u32Thick = u32Thick;
        pstADrawLine[i].u32Color = u32Color;
        i++;

        //矩形右边
        pstADrawLine[i].stStartPoint.s32X = pstArectangle[j].x+pstArectangle[j].width;
        pstADrawLine[i].stStartPoint.s32Y = pstArectangle[j].y;
        pstADrawLine[i].stEndPoint.s32X = pstArectangle[j].x+pstArectangle[j].width;
        pstADrawLine[i].stEndPoint.s32Y = pstArectangle[j].y+pstArectangle[j].height;
        pstADrawLine[i].u32Thick = u32Thick;
        pstADrawLine[i].u32Color = u32Color;
        i++;
    }

    s32Ret = HI_MPI_VGS_BeginJob(&hVgsHandle);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    memcpy(&stVgsTaskAttr.stImgIn, pstOutFrameInfo, sizeof(VIDEO_FRAME_INFO_S));
    memcpy(&stVgsTaskAttr.stImgOut, pstOutFrameInfo, sizeof(VIDEO_FRAME_INFO_S));

    s32Ret = HI_MPI_VGS_AddDrawLineTaskArray(hVgsHandle, &stVgsTaskAttr, pstADrawLine, num*4);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = HI_MPI_VGS_EndJob(hVgsHandle);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    
    free(pstADrawLine);

    return 0;
}


int sal_vgs_draw_osd(VIDEO_FRAME_INFO_S* pstOutFrameInfo, unsigned char* buffer, int width,int height)
{
    //CHECK(pstOutFrameInfo, HI_FAILURE, "invalid parameter with %#x\n", pstOutFrameInfo);

    HI_S32  s32Ret = HI_FAILURE;
    VGS_HANDLE hVgsHandle = -1;
    VGS_TASK_ATTR_S stVgsTaskAttr;
    memset(&stVgsTaskAttr, 0, sizeof(VGS_TASK_ATTR_S));
    VGS_ADD_OSD_S stVgsAddOsd;
    memset(&stVgsAddOsd, 0, sizeof(stVgsAddOsd));
    
    stVgsAddOsd.enPixelFmt = PIXEL_FORMAT_RGB_1555; //PIXEL_FORMAT_RGB_8888;
    stVgsAddOsd.stRect.s32X = pstOutFrameInfo->stVFrame.u32Width - width;
    stVgsAddOsd.stRect.s32Y = (pstOutFrameInfo->stVFrame.u32Height-height)/2;
    stVgsAddOsd.stRect.u32Width = width;
    stVgsAddOsd.stRect.u32Height = height;
    stVgsAddOsd.u32BgColor = 0;
    stVgsAddOsd.u32BgAlpha = 0;
    stVgsAddOsd.u32FgAlpha = 255;
    stVgsAddOsd.u32PhyAddr = 0;
    stVgsAddOsd.u32Stride = width*2; //stVgsAddOsd.stRect.u32Width;
    
    unsigned char* pu8VirAddr = NULL;
    unsigned int u32Size = stVgsAddOsd.u32Stride * stVgsAddOsd.stRect.u32Height;
    s32Ret = HI_MPI_SYS_MmzAlloc(&stVgsAddOsd.u32PhyAddr, (void**)&pu8VirAddr, "vgs_osd", HI_NULL, u32Size);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    
    //CHECK(u32Size == width*height*2, HI_FAILURE, "invalid size %d\n", size);
    memcpy(pu8VirAddr, buffer, u32Size);

    s32Ret = HI_MPI_VGS_BeginJob(&hVgsHandle);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    memcpy(&stVgsTaskAttr.stImgIn, pstOutFrameInfo, sizeof(VIDEO_FRAME_INFO_S));
    memcpy(&stVgsTaskAttr.stImgOut, pstOutFrameInfo, sizeof(VIDEO_FRAME_INFO_S));

    s32Ret = HI_MPI_VGS_AddOsdTask(hVgsHandle, &stVgsTaskAttr, &stVgsAddOsd);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = HI_MPI_VGS_EndJob(hVgsHandle);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    
    s32Ret = HI_MPI_SYS_MmzFree(stVgsAddOsd.u32PhyAddr, pu8VirAddr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return 0;
}


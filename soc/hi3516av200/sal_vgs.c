#include "hi_comm.h"
#include "sal_cover.h"
#include "sal_debug.h"
#include "sal_av.h"

#if 0
int drawFaceRect(VIDEO_FRAME_INFO_S * p_pstOutFrameInfo)
{

    int i, j, x, y, w, h;
    int nDrawLineSum;
    double dHScale, dVScale;
    HI_S32  s32Ret = HI_SUCCESS;
    VGS_HANDLE hVgsHandle;
    VGS_TASK_ATTR_S stVgsTaskAttr;
    VGS_DRAW_LINE_S *pstDrawLine = NULL;

    s32Ret = HI_MPI_VGS_BeginJob(&hVgsHandle);
    if (s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_VGS_BeginJob failed with %#x\n", s32Ret);
        return false;
    }
 
    memset(&stVgsTaskAttr, 0, sizeof(VGS_TASK_ATTR_S));
    memset(pstDrawLine, 0, nDrawLineSum * sizeof(VGS_DRAW_LINE_S));

    memcpy(&stVgsTaskAttr.stImgIn, p_pstOutFrameInfo, sizeof(VIDEO_FRAME_INFO_S));
    memcpy(&stVgsTaskAttr.stImgOut, p_pstOutFrameInfo, sizeof(VIDEO_FRAME_INFO_S));

    dHScale = p_pstOutFrameInfo->stVFrame.u32Width /(double)m_nImageWidth;
    dVScale = p_pstOutFrameInfo->stVFrame.u32Height /(double)m_nImageHeight;




    s32Ret = HI_MPI_VGS_AddDrawLineTaskArray(hVgsHandle, &stVgsTaskAttr, pstDrawLine, nDrawLineSum);
    if (s32Ret != HI_SUCCESS)
        {
        printf("HI_MPI_VGS_AddDrawLineTaskArray failed with %#x\n", s32Ret);
        HI_MPI_VGS_CancelJob(hVgsHandle);
        return false;
        }

    s32Ret = HI_MPI_VGS_EndJob(hVgsHandle);
    if (s32Ret != HI_SUCCESS)
        {
        printf("HI_MPI_VGS_EndJob failed with %#x\n", s32Ret);
        HI_MPI_VGS_CancelJob(hVgsHandle);
        return false;
        }

    delete []pstDrawLine;
    pstDrawLine = NULL;

    return true;
}
#endif



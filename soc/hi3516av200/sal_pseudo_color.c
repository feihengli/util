#include "hi_comm.h"
#include "sal_debug.h"
#include "sal_util.h"
#include "sal_vgs.h"
#include "sal_pseudo_color.h"
#include "colormap.h"
#include "picture_bmp.h"

#define MMZ_PC_ZONE_NAME "PseudoColor"

typedef struct sal_pc_args
{
    struct timeval current;
    int vpss_chn;
    int venc_chn;
    int enable;
    
    //伪彩条
    int enable_osd;
    unsigned char* rgb1555buffer;
    int width;
    int height;
    
    //映射表
    unsigned char u8_y[256];
    unsigned char u8_vu[256*2];

    int running;
    pthread_t pid;
    pthread_mutex_t mutex;
}sal_pc_args;

static sal_pc_args* g_pc_args = NULL;

static int pc_resize(IVE_SRC_IMAGE_S* pstSrc, IVE_SRC_IMAGE_S* pstDst)
{
    int s32Ret = -1;
    IVE_HANDLE IveHandle = -1;
    HI_BOOL bFinish = HI_FALSE;
    IVE_RESIZE_CTRL_S stResizeCtrl;
    memset(&stResizeCtrl, 0, sizeof(stResizeCtrl));
    
    stResizeCtrl.enMode = IVE_RESIZE_MODE_LINEAR;
    stResizeCtrl.u16Num = 1;
    int U8C1_NUM = 1;
    stResizeCtrl.stMem.u32Size = 25*U8C1_NUM + 49 * (stResizeCtrl.u16Num - U8C1_NUM);//copy from hisi document
    
    s32Ret = HI_MPI_SYS_MmzAlloc(&stResizeCtrl.stMem.u32PhyAddr, (void**)&stResizeCtrl.stMem.pu8VirAddr, MMZ_PC_ZONE_NAME, HI_NULL, stResizeCtrl.stMem.u32Size);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    
    s32Ret = HI_MPI_IVE_Resize(&IveHandle, pstSrc, pstDst, &stResizeCtrl, HI_TRUE);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    
    s32Ret = HI_MPI_IVE_Query(IveHandle, &bFinish, HI_TRUE);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    
    s32Ret = HI_MPI_SYS_MmzFree(stResizeCtrl.stMem.u32PhyAddr, stResizeCtrl.stMem.pu8VirAddr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return 0;
}

static int pc_map(IVE_SRC_IMAGE_S* pstSrc, IVE_SRC_MEM_INFO_S* pstMap, IVE_SRC_IMAGE_S* pstDst, IVE_MAP_CTRL_S* pstMapCtrl)
{
    int s32Ret = -1;
    IVE_HANDLE IveHandle = -1;
    HI_BOOL bFinish = HI_FALSE;

    s32Ret = HI_MPI_IVE_Map(&IveHandle, pstSrc, pstMap, pstDst, pstMapCtrl, HI_TRUE);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = HI_MPI_IVE_Query(IveHandle, &bFinish, HI_TRUE);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return 0;
}

static HI_U16 pc_calc_stride(HI_U16 u16Width, HI_U8 u8Align)
{
    return (u16Width + (u8Align - u16Width%u8Align)%u8Align);
}

static HI_S32 pc_alloc_image(IVE_IMAGE_S *pstImg,IVE_IMAGE_TYPE_E enType,HI_U16 u16Width,HI_U16 u16Height)
{
    HI_U32 u32Size = 0;
    HI_S32 s32Ret = -1;

    pstImg->enType = enType;
    pstImg->u16Width = u16Width;
    pstImg->u16Height = u16Height;
    pstImg->u16Stride[0] = pc_calc_stride(pstImg->u16Width,16);

    switch(enType)
    {
    case IVE_IMAGE_TYPE_U8C1:
    case IVE_IMAGE_TYPE_S8C1:
        {
            u32Size = pstImg->u16Stride[0] * pstImg->u16Height;
            s32Ret = HI_MPI_SYS_MmzAlloc(&pstImg->u32PhyAddr[0], (void**)&pstImg->pu8VirAddr[0], MMZ_PC_ZONE_NAME, HI_NULL, u32Size);
            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
        }
        break;
    case IVE_IMAGE_TYPE_YUV420SP:
        {
            u32Size = pstImg->u16Stride[0] * pstImg->u16Height * 3 / 2;
            s32Ret = HI_MPI_SYS_MmzAlloc(&pstImg->u32PhyAddr[0], (void**)&pstImg->pu8VirAddr[0], MMZ_PC_ZONE_NAME, HI_NULL, u32Size);
            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

            pstImg->u16Stride[1] = pstImg->u16Stride[0];
            pstImg->u32PhyAddr[1] = pstImg->u32PhyAddr[0] + pstImg->u16Stride[0] * pstImg->u16Height;
            pstImg->pu8VirAddr[1] = pstImg->pu8VirAddr[0] + pstImg->u16Stride[0] * pstImg->u16Height;
        }
        break;
    default:
        {
            WRN("Unknown type: %d\n", enType);
        }
        break;
    }

    return HI_SUCCESS;
}

static int pc_save2file(const char* path, VIDEO_FRAME_INFO_S* pstVideoFrame)
{
    int s32Ret = -1;
    unsigned int u32Size = (pstVideoFrame->stVFrame.u32Stride[0]) * (pstVideoFrame->stVFrame.u32Height) * 3 / 2;//yuv420
    unsigned char* yuv420sp = (unsigned char*) HI_MPI_SYS_Mmap(pstVideoFrame->stVFrame.u32PhyAddr[0], u32Size);
    
    s32Ret = util_file_write(path, yuv420sp, u32Size);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    
    s32Ret = HI_MPI_SYS_Munmap(yuv420sp, u32Size);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    
    return 0;
}

static int pc_fillYuv420sp(unsigned char *tarYuv, unsigned char *srcYuv, int startW,  
                   int startH, int cutW, int cutH, int srcW, int srcH)   
{  
    int i = 0; 
    unsigned char yValue = 255;

    for (i=0; i<256+0; i++) 
    {
        memset(srcYuv+i*srcW, yValue, cutW);
        yValue--;
    }

    return 0;
}

static void* pc_proc(void* args)
{
    prctl(PR_SET_NAME, __FUNCTION__);

    int s32Ret = -1;
    VPSS_GRP VpssGrp = 0;
    VIDEO_FRAME_INFO_S stFrameInfo;
    memset(&stFrameInfo, 0, sizeof(stFrameInfo));

    IVE_SRC_IMAGE_S stSrc;
    IVE_SRC_IMAGE_S stSrc1;
    IVE_DST_IMAGE_S stDstY;
    IVE_SRC_IMAGE_S stDstUv;

    memset(&stSrc, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&stSrc1, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&stDstY, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&stDstUv, 0, sizeof(IVE_SRC_IMAGE_S));
    
    IVE_SRC_IMAGE_S stResizeSrc;
    memset(&stResizeSrc, 0, sizeof(stResizeSrc));
    IVE_DST_IMAGE_S stResizeDst;
    memset(&stResizeDst, 0, sizeof(stResizeDst));
    
    IVE_MAP_CTRL_S stMapCtrl;
    memset(&stMapCtrl, 0, sizeof(stMapCtrl));
    
    if (g_pc_args->vpss_chn < 4)
    {
        VPSS_CHN_MODE_S stVpssMode;
        memset(&stVpssMode, 0, sizeof(stVpssMode));
        s32Ret = HI_MPI_VPSS_GetChnMode(VpssGrp, g_pc_args->vpss_chn, &stVpssMode);
        CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);
        
        s32Ret = pc_alloc_image(&stResizeDst, IVE_IMAGE_TYPE_U8C1, stVpssMode.u32Width/2, stVpssMode.u32Height/2);
        CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);
    }
    else
    {
        VPSS_EXT_CHN_ATTR_S stExtChnAttr;
        memset(&stExtChnAttr, 0, sizeof(stExtChnAttr));
        s32Ret = HI_MPI_VPSS_GetExtChnAttr(VpssGrp, g_pc_args->vpss_chn, &stExtChnAttr);
        CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);
        
        s32Ret = pc_alloc_image(&stResizeDst, IVE_IMAGE_TYPE_U8C1, stExtChnAttr.u32Width/2, stExtChnAttr.u32Height/2);
        CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);
    }
    
    IVE_SRC_MEM_INFO_S stMap;
    memset(&stMap, 0, sizeof(stMap));
    
    HI_U32 u32Depth = 0;
    s32Ret = HI_MPI_VPSS_GetDepth(VpssGrp, g_pc_args->vpss_chn, &u32Depth);
    CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);
    
    if (u32Depth < 1)
    {
        u32Depth = 1;
        DBG("vpss_chn: %d, u32Depth: %u\n", g_pc_args->vpss_chn, u32Depth);
    }
    
    s32Ret = HI_MPI_VPSS_SetDepth(VpssGrp, g_pc_args->vpss_chn, u32Depth);
    CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);
    
    //sleep(5);
    
    while (g_pc_args->running)
    {
        util_time_abs(&g_pc_args->current);
        
        s32Ret = HI_MPI_VPSS_GetChnFrame(VpssGrp, g_pc_args->vpss_chn, &stFrameInfo, 100*1000);
        if (s32Ret != HI_SUCCESS)
        {
            //DBG("HI_MPI_VPSS_GetChnFrame[%d] time out. Error with %#x.\n", g_pc_args->vpss_chn, s32Ret);
            continue;
        }
        
        if (g_pc_args->enable)
        {
            unsigned int u32Size = (stFrameInfo.stVFrame.u32Stride[0]) * (stFrameInfo.stVFrame.u32Height) * 3 / 2;//yuv420
            unsigned char* yuv420sp = (unsigned char*) HI_MPI_SYS_Mmap(stFrameInfo.stVFrame.u32PhyAddr[0], u32Size);
            CHECK(yuv420sp, NULL, "error with %#x.\n", yuv420sp);
            
            memset(yuv420sp, 0x80, 8);
            static unsigned int count = 0;
            unsigned char cc[4] = {0};
            memcpy(cc, &count, 4);
            count++;
            yuv420sp[0] |= cc[0] >> 4;
            yuv420sp[1] |= cc[0] & 0x0f;
            yuv420sp[2] |= cc[1] >> 4;
            yuv420sp[3] |= cc[1] & 0x0f;
            yuv420sp[4] |= cc[2] >> 4;
            yuv420sp[5] |=cc[2] & 0x0f;
            yuv420sp[6] |= cc[3] >> 4;
            yuv420sp[7] |= cc[3] & 0x0f;
            
            cc[0] |= yuv420sp[0] << 4;
            cc[0] |= yuv420sp[1] & 0x0f;
            cc[1] |= yuv420sp[2] << 4;
            cc[1] |= yuv420sp[3] & 0x0f;
            cc[2] |= yuv420sp[4] << 4;
            cc[2] |= yuv420sp[5] & 0x0f;
            cc[3] |= yuv420sp[6] << 4;
            cc[3] |= yuv420sp[7] & 0x0f;
            unsigned int tt = 0;
            memcpy(&tt, cc, 4);
            //DBG("count: %d, tt: %d\n", count, tt);
            //printf("VPSS begin : %02x %02x %02x %02x %02x %02x %02x %02x\n", yuv420sp[0], yuv420sp[1], yuv420sp[2], yuv420sp[3], yuv420sp[4], yuv420sp[5], yuv420sp[6], yuv420sp[7]);
            //memset(yuv420sp, 0xff, u32Size);
            //DBG("reset ok\n");
            
            char tmp[32];
            static int count1 = 0;
            sprintf(tmp, "ttt_%d.yuv", count1++);
            
            //pc_save2file(tmp, &stFrameInfo);
            
            s32Ret = HI_MPI_SYS_Munmap(yuv420sp, u32Size);
            CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);
        }
        
        //s32Ret = HI_MPI_VENC_SendFrame(g_pc_args->venc_chn, &stFrameInfo, -1);
        //CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);
        
        s32Ret = HI_MPI_VO_SendFrame(0, 0, &stFrameInfo, -1);
        CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);

        s32Ret = HI_MPI_VPSS_ReleaseChnFrame(VpssGrp, g_pc_args->vpss_chn, &stFrameInfo);
        CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);
        
        //break;
    }

    return NULL;
}

static int pc_create_bitmapRGB(unsigned char *rgbout,int width,int height, unsigned char* colormap_yuv)
{
    CHECK(height == 256, -1, "error with: %#x\n", height);
    
    unsigned long idx=0;
    int j = 0;
    int i = 0;
    
    int idx_map = 0;
    unsigned char r,g,b;
    for (j = 0; j < height; j++) 
    {
        idx_map = j*4;;
        idx=j*width*3;
        for (i=0;i<width;i++)
        {
            r = g = b = 0;
            bmp_yuv2rgb(colormap_yuv[idx_map+0], colormap_yuv[idx_map+1], colormap_yuv[idx_map+2], &r, &g, &b);
            
            //打白黑边框，各占一个像素
            if ((i == 0) || (i == width-1) || (j == 0) || (j == height-1))
            {
                r=0xff;
                g=0xff;
                b=0xff;
            }
            if ((i == 1) || (i == width-2) || (j == 1) || (j == height-2))
            {
                r=0;
                g=0;
                b=0;
            }
            if ((j == 1 && i == 0) || (j == 1 && i == width-1) || (j == height-2 && i == 0) || (j == height-2 && i == width-1)
                || (j == 0 && i == 1) || (j == 0 && i == width-2) || (j == height-1 && i == 1) || (j == height-1 && i == width-2))
            {
                r=0xff;
                g=0xff;
                b=0xff;
            }

            rgbout[idx++]=r;
            rgbout[idx++]=g;
            rgbout[idx++]=b;
        }
    }
    
    return 0;
}

int pc_update_colormap(int index)
{
    int ret = -1;
    unsigned char* array_colormap[] = 
                                    {
                                        colormap_hsv, 
                                        colormap_autumn, 
                                        colormap_bone, 
                                        colormap_cool, 
                                        colormap_copper, 
                                        colormap_hot,
                                        colormap_pink, 
                                        colormap_spring, 
                                        colormap_summer, 
                                        colormap_winter, 
                                        colormap_jet, 
                                        colormap_whiteheat,
                                        colormap_rosebengal,
                                        colormap_bone_140311, 
                                        colormap_iron_140321, 
                                        colormap_whitejet_141212, 
                                        colormap_jet_140311, 
                                        colormap_tiehong
                                    };
    unsigned char* colormap = array_colormap[index];
    
    int i = 0;
    int j = 0;
    for (i = 0; i < 1024; i += 4)
    {
        g_pc_args->u8_y[j] = colormap[i];
        j++;
    }
    j = 0;
    for (i = 2; i < 1024; i += 4)
    {
        g_pc_args->u8_vu[j] = colormap[i];//v
        g_pc_args->u8_vu[j+1] = colormap[i-1];//u
        j += 2;
    }
    
    unsigned int size = g_pc_args->width * g_pc_args->height * 3;
    unsigned char* rgbbuffer_dst = (unsigned char*)malloc(size);
    CHECK(rgbbuffer_dst, -1, "failed to malloc %d bytes \n", size);
    memset(rgbbuffer_dst, 0, size);
    
    //size = 24 * 256 * 3;
    //unsigned char* rgbbuffer_src = (unsigned char*)malloc(size);
    //CHECK(rgbbuffer_src, -1, "failed to malloc %d bytes \n", size);
    //memset(rgbbuffer_src, 0, size);
    
    //ret = pc_create_bitmapRGB(rgbbuffer_src, 24, 256, colormap);
    //CHECK(ret == 0, -1, "error with: %#x\n", ret);
    //
    //ret = bmp_copy(rgbbuffer_src, 24, 256, rgbbuffer_dst,g_pc_args->width, g_pc_args->height, 3, 1,1);
    //CHECK(ret == 0, -1, "error with: %#x\n", ret);
    
    ret = pc_create_bitmapRGB(rgbbuffer_dst, g_pc_args->width, g_pc_args->height, colormap);
    CHECK(ret == 0, -1, "error with: %#x\n", ret);
    
    ret = bmp_rgb2rgb1555(rgbbuffer_dst, g_pc_args->rgb1555buffer, g_pc_args->width, g_pc_args->height);
    CHECK(ret == 0, -1, "error with: %#x\n", ret);

    //ret = bmp_rgb2bgr(rgbbuffer_dst, g_pc_args->width, g_pc_args->height);
    //CHECK(ret == 0, -1, "error with: %#x\n", ret);

    //ret = bmp_save2file("weicai.bmp", rgbbuffer_dst, g_pc_args->width, g_pc_args->height);
    //CHECK(ret == 0, -1, "error with: %#x\n", ret);
    
    free(rgbbuffer_dst);
    //free(rgbbuffer_src);
    return 0;
}



int sal_pc_init()
{
    CHECK(NULL == g_pc_args, -1, "reinit error, please uninit first.\n");
    
    int ret = -1;
    g_pc_args = (sal_pc_args*)malloc(sizeof(sal_pc_args));
    CHECK(NULL != g_pc_args, -1, "malloc %d bytes failed.\n", sizeof(sal_pc_args));

    memset(g_pc_args, 0, sizeof(sal_pc_args));
    pthread_mutex_init(&g_pc_args->mutex, NULL);
    
    g_pc_args->vpss_chn = 0;
    g_pc_args->venc_chn = 0;
    g_pc_args->enable = 1;
    g_pc_args->enable_osd = 1;
    g_pc_args->width = 24;
    g_pc_args->height = 256;
    unsigned int size = g_pc_args->width*g_pc_args->height*2; 
    g_pc_args->rgb1555buffer = (unsigned char*)malloc(size);
    CHECK(g_pc_args->rgb1555buffer, -1, "Error with %#x\n", g_pc_args->rgb1555buffer);
    memset(g_pc_args->rgb1555buffer, 0, size);

    ret = pc_update_colormap(12);
    CHECK(ret == 0, -1, "error with %#x\n", ret);
    
    g_pc_args->running = 1;
    ret = pthread_create(&g_pc_args->pid, NULL, pc_proc, NULL);
    CHECK(ret == 0, -1, "Error with %s.\n", strerror(errno));

    return 0;
}

int sal_pc_exit()
{


    return 0;
}



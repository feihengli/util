#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/prctl.h>

#include "hi_comm.h"
#include "sal_overlay.h"
#include "sal_util.h"
#include "sal_debug.h"
#include "sal_bitmap.h"
#include "sal_osd_fullscreen.h"
#include "sal_config.h"
#include "sal_statistics.h"
#include "sal_isp.h"
#include "sal_lbr.h"
#include "sal_af.h"
#include "sal_yuv.h"
#include "libjpeg/jpeglib.h"

#define RGB_VALUE_BLACK     0x8000  //黑
#define RGB_VALUE_WHITE     0xffff  // 白色
#define RGB_VALUE_BG        0x7fff //透明

enum YUV_TYPE
    {
    NV21OR21,
    YUV420SP,
    };
typedef struct sal_osd_fullscreen_args
{
    struct timeval current;
    sal_overlay_s param[100];

    int running;
    pthread_t pid;
    pthread_mutex_t mutex;
}sal_osd_fullscreen_args;

static sal_osd_fullscreen_args* g_osd_fullscreen_args = NULL;

int drawFaceRect(VIDEO_FRAME_INFO_S * p_pstOutFrameInfo)
{

    int i, j, x, y, w, h;
    int nDrawLineSum;
    double dHScale, dVScale;
    HI_S32  s32Ret = HI_SUCCESS;
    VGS_HANDLE hVgsHandle;
    VGS_TASK_ATTR_S stVgsTaskAttr;
    VGS_DRAW_LINE_S astDrawLine[4]=
    {
        {
            {100,100},
            {100,800},
            8,
            0x0000FF00
        },
        {
            {200,200},
            {200,800},
            8,
            0x00FFFFFF
        },
        {
            {300,300},
            {300,800},
            8,
            0x00FF0000
        },
        {
            {400,400},
            {400,800},
            8,
            0x00FF0000
        }
    };

    s32Ret = HI_MPI_VGS_BeginJob(&hVgsHandle);
    if (s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_VGS_BeginJob failed with %#x\n", s32Ret);
        return -1;
    }
    
    //VGS_DRAW_LINE_S astDrawLine = 
    //{
    //    {100,100},
    //    {100,800},
    //    8,
    //    0x00FF0000,
    //};
    //memcpy();
    
    
    memset(&stVgsTaskAttr, 0, sizeof(VGS_TASK_ATTR_S));
    //memset(astDrawLine, 0, sizeof(astDrawLine));
    
    //printf("u32TimeRef: %u\n", p_pstOutFrameInfo->stVFrame.u32TimeRef);
    memcpy(&stVgsTaskAttr.stImgIn, p_pstOutFrameInfo, sizeof(VIDEO_FRAME_INFO_S));
    memcpy(&stVgsTaskAttr.stImgOut, p_pstOutFrameInfo, sizeof(VIDEO_FRAME_INFO_S));






    s32Ret = HI_MPI_VGS_AddDrawLineTaskArray(hVgsHandle, &stVgsTaskAttr, astDrawLine, 4);
    if (s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_VGS_AddDrawLineTaskArray failed with %#x\n", s32Ret);
        HI_MPI_VGS_CancelJob(hVgsHandle);
        return -1;
    }

    s32Ret = HI_MPI_VGS_EndJob(hVgsHandle);
    if (s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_VGS_EndJob failed with %#x\n", s32Ret);
        HI_MPI_VGS_CancelJob(hVgsHandle);
        return -1;
    }



    return 0;
}


static int osd_draw(int posx, int posy, int width, int height)
{
    static int id = 3;
    int ret = -1;
    sal_overlay_s param;
    memset(&param, 0, sizeof(param));
    param.factor = 1;
    param.font_color = 1;
    param.edge_color = 1;
    param.handle = id++;
    param.stream_id = 0;
    param.posx = posx;
    param.posy = posy;
    param.width = width;
    param.height = height;
    param.bitmap_data = NULL;
    
    ret = sal_overlay_ex_create(&param);
    CHECK(ret == 0, -1, "Error with %#x.\n", ret);
    
    if (param.bitmap_data)
    {
        int i = 0;
        int j = 0;
        for (i = 0; i < param.height; i++)
            for (j = 0; j < param.width; j++)
            {
                unsigned short* ptr = param.bitmap_data;
                ptr[i*param.width+j] = RGB_VALUE_WHITE;
            }
    }
    
    ret = sal_overlay_refresh_bitmap(&param);
    CHECK(ret == 0, -1, "Error with %#x.\n", ret);
    
    

    return 0;
}

/******************************************************************************
* function : Get from YUV
******************************************************************************/
HI_S32 SAMPLE_COMM_VI_GetVFrameFromYUV(FILE* pYUVFile, HI_U32 u32Width, HI_U32 u32Height, HI_U32 u32Stride, VIDEO_FRAME_INFO_S* pstVFrameInfo)
    {
    HI_U32             u32LStride;
    HI_U32             u32CStride;
    HI_U32             u32LumaSize;
    HI_U32             u32ChrmSize;
    HI_U32             u32Size;
    VB_BLK VbBlk;
    HI_U32 u32PhyAddr;
    HI_U8* pVirAddr;

    u32LStride  = u32Stride;
    u32CStride  = u32Stride;

    u32LumaSize = (u32LStride * u32Height);
    u32ChrmSize = (u32CStride * u32Height) >> 2;/* YUV 420 */
    u32Size = u32LumaSize + (u32ChrmSize << 1);
    DBG("u32Size: %u, u32LumaSize: %u, u32ChrmSize: %u\n", u32Size,u32LumaSize, u32ChrmSize);

    /* alloc video buffer block ---------------------------------------------------------- */
    VbBlk = HI_MPI_VB_GetBlock(VB_INVALID_POOLID, u32Size, NULL);
    if (VB_INVALID_HANDLE == VbBlk)
        {
        printf("HI_MPI_VB_GetBlock err! size:%d\n", u32Size);
        return -1;
        }
    u32PhyAddr = HI_MPI_VB_Handle2PhysAddr(VbBlk);
    if (0 == u32PhyAddr)
        {
        return -1;
        }

    pVirAddr = (HI_U8*) HI_MPI_SYS_Mmap(u32PhyAddr, u32Size);
    if (NULL == pVirAddr)
        {
        return -1;
        }

    pstVFrameInfo->u32PoolId = HI_MPI_VB_Handle2PoolId(VbBlk);
    if (VB_INVALID_POOLID == pstVFrameInfo->u32PoolId)
        {
        return -1;
        }
    printf("pool id :%d, phyAddr:%x,virAddr:%x\n" , pstVFrameInfo->u32PoolId, u32PhyAddr, (int)pVirAddr);

    pstVFrameInfo->stVFrame.u32PhyAddr[0] = u32PhyAddr;
    pstVFrameInfo->stVFrame.u32PhyAddr[1] = pstVFrameInfo->stVFrame.u32PhyAddr[0] + u32LumaSize;
    pstVFrameInfo->stVFrame.u32PhyAddr[2] = pstVFrameInfo->stVFrame.u32PhyAddr[1] + u32ChrmSize;

    pstVFrameInfo->stVFrame.pVirAddr[0] = pVirAddr;
    pstVFrameInfo->stVFrame.pVirAddr[1] = pstVFrameInfo->stVFrame.pVirAddr[0] + u32LumaSize;
    pstVFrameInfo->stVFrame.pVirAddr[2] = pstVFrameInfo->stVFrame.pVirAddr[1] + u32ChrmSize;

    pstVFrameInfo->stVFrame.u32Width  = u32Width;
    pstVFrameInfo->stVFrame.u32Height = u32Height;
    pstVFrameInfo->stVFrame.u32Stride[0] = u32LStride;
    pstVFrameInfo->stVFrame.u32Stride[1] = u32CStride;
    pstVFrameInfo->stVFrame.u32Stride[2] = u32CStride;
    pstVFrameInfo->stVFrame.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    pstVFrameInfo->stVFrame.u32Field = VIDEO_FIELD_FRAME;/* Intelaced D1,otherwise VIDEO_FIELD_FRAME */

    ///* read Y U V data from file to the addr ----------------------------------------------*/
    //SAMPLE_COMM_VI_ReadFrame(pYUVFile, pstVFrameInfo->stVFrame.pVirAddr[0],
    //    pstVFrameInfo->stVFrame.pVirAddr[1], pstVFrameInfo->stVFrame.pVirAddr[2],
    //    pstVFrameInfo->stVFrame.u32Width, pstVFrameInfo->stVFrame.u32Height,
    //    pstVFrameInfo->stVFrame.u32Stride[0], pstVFrameInfo->stVFrame.u32Stride[1] >> 1 );

    ///* convert planar YUV420 to sem-planar YUV420 -----------------------------------------*/
    //SAMPLE_COMM_VI_PlanToSemi(pstVFrameInfo->stVFrame.pVirAddr[0], pstVFrameInfo->stVFrame.u32Stride[0],
    //    pstVFrameInfo->stVFrame.pVirAddr[1], pstVFrameInfo->stVFrame.u32Stride[1],
    //    pstVFrameInfo->stVFrame.pVirAddr[2], pstVFrameInfo->stVFrame.u32Stride[1],
    //    pstVFrameInfo->stVFrame.u32Width, pstVFrameInfo->stVFrame.u32Height);

    HI_MPI_SYS_Mmap(u32PhyAddr, u32Size);
    return 0;
    }

int yuv420sp_to_jpg11(char *filename, int width, int height, unsigned char *pYUVBuffer)
{
    FILE *fJpg;
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPROW row_pointer[1];
    int row_stride;
    int i = 0, j = 0;
    unsigned char yuvbuf[width * 3];
    unsigned char *pY, *pU, *pV;
    int ulen;

    ulen = width * height / 4;

    if(pYUVBuffer == NULL){
        DBG("pBGRBuffer is NULL!\n");
        return -1;
    }

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    fJpg = fopen(filename, "wb");
    if(fJpg == NULL){
        DBG("Cannot open file %s, %s\n", filename, strerror(errno));
        jpeg_destroy_compress(&cinfo);
        return -1;
    }

    jpeg_stdio_dest(&cinfo, fJpg);
    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_YCbCr;
    cinfo.dct_method = JDCT_ISLOW;
    jpeg_set_defaults(&cinfo);


    jpeg_set_quality(&cinfo, 99, TRUE);

    jpeg_start_compress(&cinfo, TRUE);
    row_stride = cinfo.image_width * 3; /* JSAMPLEs per row in image_buffer */
    
    pY = pYUVBuffer;
    pU = pYUVBuffer + width*height;
    pV = pYUVBuffer + width*height + ulen;
    j = 1;
    while (cinfo.next_scanline < cinfo.image_height) {
        /* jpeg_write_scanlines expects an array of pointers to scanlines.
         * Here the array is only one element long, but you could pass
         * more than one scanline at a time if that's more convenient.
         */

        /*Test yuv buffer serial is : yyyy...uu..vv*/
        if(j % 2 == 1 && j > 1){
            pU = pYUVBuffer + width*height + width / 2 * (j / 2);
            pV = pYUVBuffer + width*height * 5 / 4 + width / 2 *(j / 2);
        }
        for(i = 0; i < width; i += 2){
            yuvbuf[i*3] = *pY++;
            yuvbuf[i*3 + 1] = *pU;
            yuvbuf[i*3 + 2] = *pV;

            yuvbuf[i*3 + 3] = *pY++;
            yuvbuf[i*3 + 4] = *pU++;
            yuvbuf[i*3 + 5] = *pV++;
        }

        row_pointer[0] = yuvbuf;
        (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
        j++;
    }

    jpeg_finish_compress(&cinfo);

    jpeg_destroy_compress(&cinfo);
    fclose(fJpg);

    return 0;
}

int rgb2jpeg(const char * filename, unsigned char* rgbData,int image_width,int image_height,int quality)    
    {    
    struct jpeg_compress_struct jpeg;  //identify a compress object  
    struct jpeg_error_mgr jerr;  //error information  

    jpeg.err = jpeg_std_error(&jerr);    
    jpeg_create_compress(&jpeg);  //init compress object  

    FILE* pFile;  
    pFile = fopen(filename,"wb" );    
    if( !pFile )  return 0;    
    jpeg_stdio_dest(&jpeg, pFile);    

    //compress param set,i just did a simple param set  
    jpeg.client_data=(void*)&pFile;  
    jpeg.image_width = image_width;    
    jpeg.image_height = image_height;    
    jpeg.input_components  = 3;    
    jpeg.in_color_space = JCS_RGB;     
    jpeg_set_defaults(&jpeg);     
    //// 指定亮度及色度质量    
    jpeg.q_scale_factor[0] = jpeg_quality_scaling(100);    
    jpeg.q_scale_factor[1] = jpeg_quality_scaling(100);    
    //// 图像采样率，默认为2 * 2    
    jpeg.comp_info[0].v_samp_factor = 2;    
    jpeg.comp_info[0].h_samp_factor = 2;    
    //// set jpeg compress quality    
    jpeg_set_quality(&jpeg, quality, TRUE);  //100 is the highest  

    //start compress  
    jpeg_start_compress(&jpeg, TRUE);    

    JSAMPROW row_pointer[1];    

    //from up to down ,set every pixel  
    unsigned int i=0;
    for( i=0;i<jpeg.image_height;i++ )    
        {    
        row_pointer[0] = rgbData+i*jpeg.image_width*3;    
        jpeg_write_scanlines( &jpeg,row_pointer,1 );    
        }    
    //stop compress  
    jpeg_finish_compress(&jpeg);    

    fclose( pFile );    
    pFile = NULL;    
    jpeg_destroy_compress(&jpeg);    
    return 0;    
    } 

int yuv420sp_to_jpeg(const char * filename, const char* pdata,int image_width,int image_height, int quality)  
    {     
    struct jpeg_compress_struct cinfo;    
    struct jpeg_error_mgr jerr;    
    cinfo.err = jpeg_std_error(&jerr);    
    jpeg_create_compress(&cinfo);    

    FILE * outfile;    // target file    
    if ((outfile = fopen(filename, "wb")) == NULL) {    
        fprintf(stderr, "can't open %s\n", filename);    
        exit(1);    
        }    
    jpeg_stdio_dest(&cinfo, outfile);    

    cinfo.image_width = image_width;  // image width and height, in pixels    
    cinfo.image_height = image_height;    
    cinfo.input_components = 3;    // # of color components per pixel    
    cinfo.in_color_space = JCS_YCbCr;  //colorspace of input image    
    jpeg_set_defaults(&cinfo);    
    jpeg_set_quality(&cinfo, quality, TRUE );    

    //////////////////////////////    
    //  cinfo.raw_data_in = TRUE;    
    cinfo.jpeg_color_space = JCS_YCbCr;    
    cinfo.comp_info[0].h_samp_factor = 2;    
    cinfo.comp_info[0].v_samp_factor = 2;    
    /////////////////////////    

    jpeg_start_compress(&cinfo, TRUE);    

    JSAMPROW row_pointer[1];  

    unsigned char *yuvbuf;  
    if((yuvbuf=(unsigned char *)malloc(image_width*3))!=NULL)  
        memset(yuvbuf,0,image_width*3);  

    unsigned char *ybase,*ubase;  
    ybase=pdata;  
    ubase=pdata+image_width*image_height;    
    int j=0;  
    int i=0;
    while (cinfo.next_scanline < cinfo.image_height)   
        {  
        int idx=0;  
        for(i=0;i<image_width;i++)  
            {   
            yuvbuf[idx++]=ybase[i + j * image_width];  
            yuvbuf[idx++]=ubase[j/2 * image_width+(i/2)*2+1];  
            yuvbuf[idx++]=ubase[j/2 * image_width+(i/2)*2];      
            }  
        row_pointer[0] = yuvbuf;  
        jpeg_write_scanlines(&cinfo, row_pointer, 1);  
        j++;  
        }  
    jpeg_finish_compress(&cinfo);    
    jpeg_destroy_compress(&cinfo);    
    fclose(outfile);    
    return 0;    
    }  

static void* osd_fullscreen_proc(void* args)
{
    prctl(PR_SET_NAME, __FUNCTION__);

    int ret = -1;
    int s32Ret = -1;
    unsigned int i = 0;
    unsigned int j = 0;
    VIDEO_FRAME_INFO_S stFrame;
    
    //for (i = 0; i < 20; i++)
    //{
    //    DBG("i: %d\n", i);
    //    ret = osd_draw(100*i, 100*i, 100, 100);
    //    CHECK(ret == 0, NULL, "Error with %#x.\n", ret);
    //}
    
    if (HI_MPI_VPSS_SetDepth(0, 3, 1) != 0)
    {
        printf("set depth error!!!\n");
        //HI_S32 s32Ret = HI_MPI_VPSS_DisableChn(0, 4);
        //if (s32Ret != HI_SUCCESS)
        //{
        //    printf("%s[%d] Hi_Mpp failed,Chn[%d] return %#x\n", m_nDisplayChnID, s32Ret);
        //}
        return -1;
    }
    
    //if (HI_MPI_VI_SetFrameDepth(0, 1) != 0)
    //{
    //    printf("set depth error!!!\n");
    //    //HI_S32 s32Ret = HI_MPI_VPSS_DisableChn(0, 4);
    //    //if (s32Ret != HI_SUCCESS)
    //    //{
    //    //    printf("%s[%d] Hi_Mpp failed,Chn[%d] return %#x\n", m_nDisplayChnID, s32Ret);
    //    //}
    //    return -1;
    //}
    
    //if (HI_MPI_VPSS_EnableBackupFrame(0) != 0)
    //{
    //    printf("set HI_MPI_VPSS_EnableBackupFrame error!!!\n");
    //    //HI_S32 s32Ret = HI_MPI_VPSS_DisableChn(0, 4);
    //    //if (s32Ret != HI_SUCCESS)
    //    //{
    //    //    printf("%s[%d] Hi_Mpp failed,Chn[%d] return %#x\n", m_nDisplayChnID, s32Ret);
    //    //}
    //    return -1;
    //}
    
    while (g_osd_fullscreen_args->running)
    {
        //pthread_mutex_lock(&g_osd_fullscreen_args->mutex);
        util_time_abs(&g_osd_fullscreen_args->current);

        //for (i = 0; i < 2; i++)
        //{
        //    DBG("i: %d\n", i);
        //    ret = osd_draw(100*i, 100*i, 100, 100);
        //    CHECK(ret == 0, NULL, "Error with %#x.\n", ret);
        //}
        //} 
        //for(i = 0; i < 5; i++)
        {
            s32Ret = HI_MPI_VPSS_GetChnFrame(0, 3, &stFrame, -1);
            if (s32Ret != HI_SUCCESS)
            {
                //printf("HI_MPI_VPSS_GetChnFrame failed with %#x\n", s32Ret);
            }
            else
            {	
                DBG("get one frame.\n");		
                //drawFaceRect(&stFrame);
                unsigned int u32Size = (stFrame.stVFrame.u32Stride[0]) * (stFrame.stVFrame.u32Height) * 3 / 2;
                char* yuv420sp = (HI_CHAR*) HI_MPI_SYS_Mmap(stFrame.stVFrame.u32PhyAddr[0], u32Size);
                CHECK(yuv420sp, NULL, "error with %#x.\n", yuv420sp);
                
                DBG("--begin--\n");
                yuv420sp_to_jpeg("yuvsp420new.jpg", yuv420sp, 640, 360, 80);
                DBG("--end--\n");
                
                //DBG("yuv capture %dx%d size: %u\n", stFrame.stVFrame.u32Stride[0], stFrame.stVFrame.u32Height, u32Size);
                //ret = util_file_write("yuvsp420new", yuv420sp, u32Size);
                //CHECK(ret == 0, -1, "Error with %d.\n", ret);
                // 
                
                int size1 = 200*200*1.5;
                char* buffer1 = malloc(size1);
                CHECK(NULL != buffer1, NULL, "malloc %d bytes failed.\n", size1);
                cutYuv(NV21OR21, buffer1, yuv420sp, 100, 100, 200, 200, 640, 360);
                
                DBG("--begin--\n");
                yuv420sp_to_jpeg("yuvsp420new1.jpg", buffer1, 200, 200, 80);
                DBG("--end--\n");
                
                {
                    //
                    DBG("pVirAddr: %p %p %p\n", stFrame.stVFrame.pVirAddr[0], stFrame.stVFrame.pVirAddr[1],stFrame.stVFrame.pVirAddr[2]);
                    DBG("u32PoolId: %u\n", stFrame.u32PoolId);
                    DBG("%#x, %#x, %#x\n", stFrame.stVFrame.u32PhyAddr[0], stFrame.stVFrame.u32PhyAddr[1],stFrame.stVFrame.u32PhyAddr[2]);
                    DBG("u32Field: %d\n", stFrame.stVFrame.u32Field);
                    VIDEO_FRAME_INFO_S stFrameNew;
                    memcpy(&stFrameNew, &stFrame, sizeof(stFrame));
                    
                    
                    /*stFrameNew.stVFrame.u32Width = 640;
                    stFrameNew.stVFrame.u32Height = 360;
                    stFrameNew.stVFrame.u32Stride[0] = stFrameNew.stVFrame.u32Width;
                    stFrameNew.stVFrame.u32Stride[1] = stFrameNew.stVFrame.u32Width;
                    stFrameNew.stVFrame.u32Stride[2] = stFrameNew.stVFrame.u32Width;
                    
                    unsigned int u32Size1 = (stFrameNew.stVFrame.u32Stride[0] * stFrameNew.stVFrame.u32Height) * 3 / 2;
                    unsigned int u32LumaSize = (stFrameNew.stVFrame.u32Stride[0] * stFrameNew.stVFrame.u32Height);
                    unsigned int u32ChrmSize = u32LumaSize / 2;
                    
                    stFrameNew.stVFrame.u32PhyAddr[0] = stFrameNew.stVFrame.u32PhyAddr[0];
                    stFrameNew.stVFrame.u32PhyAddr[1] = stFrameNew.stVFrame.u32PhyAddr[0] + u32LumaSize;
                    stFrameNew.stVFrame.u32PhyAddr[2] = stFrameNew.stVFrame.u32PhyAddr[1] + u32ChrmSize;
                    
                    VB_BLK VbBlk = HI_MPI_VB_GetBlock(VB_INVALID_POOLID, u32Size1, HI_NULL);
                    CHECK(VB_INVALID_HANDLE != VbBlk, NULL, "Error with %#x.\n", VbBlk);
                    
                    unsigned int u32PhyAddr = HI_MPI_VB_Handle2PhysAddr(VbBlk);
                    CHECK(0 != u32PhyAddr, NULL, "Error with %#x.\n", u32PhyAddr);
                    
                    stFrameNew.u32PoolId = HI_MPI_VB_Handle2PoolId(VbBlk);
                    CHECK(VB_INVALID_POOLID != stFrameNew.u32PoolId, NULL, "Error with %#x.\n", stFrameNew.u32PoolId);
                    
                    stFrameNew.stVFrame.pVirAddr[0] = NULL;
                    stFrameNew.stVFrame.pVirAddr[1] = NULL;
                    stFrameNew.stVFrame.pVirAddr[2] = NULL;*/
                    
                    //memset(&stFrameNew, 0, sizeof(stFrameNew));
                    //SAMPLE_COMM_VI_GetVFrameFromYUV(NULL, 200, 200, 200, &stFrameNew);
                    //
                    ////memcpy(yuv420sp, buffer1, size1);
                    //memcpy(stFrameNew.stVFrame.pVirAddr[0], buffer1, size1);
                    //
                    //
                    //DBG("stFrameNew.u32PoolId: %u\n", stFrameNew.u32PoolId);
                    //s32Ret = HI_MPI_VENC_SendFrame(3, &stFrameNew, -1);
                    //CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);
                    //DBG("send ok.\n");
                }
                
                
                //ret = util_file_write("yuvsp420new_crop", buffer1, size1);
                //CHECK(ret == 0, NULL, "Error with %d.\n", ret);
                
                HI_MPI_SYS_Munmap(yuv420sp, u32Size);
            }
            HI_MPI_VPSS_ReleaseChnFrame(0, 3, &stFrame);
            break;
        }
        
        //{
        //    s32Ret = HI_MPI_VI_GetFrame(0, &stFrame, 20);
        //    if (s32Ret != HI_SUCCESS)
        //    {
        //        //printf("HI_MPI_VPSS_GetChnFrame failed with %#x\n", s32Ret);
        //    }
        //    else
        //    {	
        //        //DBG("get one frame.\n");		
        //        drawFaceRect(&stFrame);
        //    }
        //    HI_MPI_VI_ReleaseFrame(0, &stFrame);
        //}
        
        //{
        //    s32Ret = HI_MPI_VPSS_GetGrpFrame(0, &stFrame, 0);
        //    if (s32Ret != HI_SUCCESS)
        //    {
        //        //printf("HI_MPI_VPSS_GetChnFrame failed with %#x\n", s32Ret);
        //    }
        //    else
        //    {	
        //        //DBG("get one frame.\n");		
        //        drawFaceRect(&stFrame);
        //    }
        //    HI_MPI_VPSS_ReleaseGrpFrame(0, &stFrame);
        //}

        //pthread_mutex_unlock(&g_osd_fullscreen_args->mutex);
        //usleep(200*1000);
    }

    return NULL;
}



void cutYuv(int yuvType, unsigned char *tarYuv, unsigned char *srcYuv, int startW,  
            int startH, int cutW, int cutH, int srcW, int srcH)   
    {  
    int i;  
    int j = 0;  
    int k = 0;  
    //分配一段内存，用于存储裁剪后的Y分量   
    unsigned char *tmpY = (unsigned char *)malloc(cutW*cutH);  
    //分配一段内存，用于存储裁剪后的UV分量   
    unsigned char *tmpUV = (unsigned char *)malloc(cutW*cutH/2);  
    switch (yuvType) {  
    case NV21OR21:  
        for(i=startH; i<cutH+startH; i++) 
        {  
            // 逐行拷贝Y分量，共拷贝cutW*cutH  
            memcpy(tmpY+j*cutW, srcYuv+startW+i*srcW, cutW);  
            j++;  
        }  
        for(i=startH/2; i<(cutH+startH)/2; i++) 
        {  
            //逐行拷贝UV分量，共拷贝cutW*cutH/2  
            memcpy(tmpUV+k*cutW, srcYuv+startW+srcW*srcH+i*srcW, cutW);  
            k++;  
        }  
        //将拷贝好的Y，UV分量拷贝到目标内存中  
        memcpy(tarYuv, tmpY, cutW*cutH);  
        memcpy(tarYuv+cutW*cutH, tmpUV, cutW*cutH/2);  
        free(tmpY);  
        free(tmpUV);  
        break;  
    case YUV420SP:  
        //Not FInished  
        for(i=startH; i<cutH+startH; i++) 
        {  
            // 逐行拷贝Y分量，共拷贝cutW*cutH  
            memcpy(tmpY+j*cutW, srcYuv+startW+i*srcW, cutW);  
            j++;  
        }  
        for(i=startH/2; i<(cutH+startH)/2; i++) 
        {  
            //逐行拷贝UV分量，共拷贝cutW*cutH/2  
            memcpy(tmpUV+k*cutW, srcYuv+startW+srcW*srcH+i*srcW, cutW);  
            k++;  
        }  
        //将拷贝好的Y，UV分量拷贝到目标内存中  
        memcpy(tarYuv, tmpY, cutW*cutH);  
        memcpy(tarYuv+cutW*cutH, tmpUV, cutW*cutH/2);  
        free(tmpY);  
        free(tmpUV);
        break;  
        }  
    }  


int sal_osd_fullscreen_init()
{
    CHECK(NULL == g_osd_fullscreen_args, -1, "reinit error, please uninit first.\n");

    int ret = -1;
    unsigned int i = 0;
    unsigned int j = 0;
    g_osd_fullscreen_args = (sal_osd_fullscreen_args*)malloc(sizeof(sal_osd_fullscreen_args));
    CHECK(NULL != g_osd_fullscreen_args, -1, "malloc %d bytes failed.\n", sizeof(sal_osd_fullscreen_args));

    memset(g_osd_fullscreen_args, 0, sizeof(sal_osd_fullscreen_args));
    pthread_mutex_init(&g_osd_fullscreen_args->mutex, NULL);

    g_osd_fullscreen_args->running = 1;
    ret = pthread_create(&g_osd_fullscreen_args->pid, NULL, osd_fullscreen_proc, NULL);
    CHECK(ret == 0, -1, "Error with %s.\n", strerror(errno));
    
    //int size = 4000*3000*1.5;
    //char* buffer = malloc(size);
    //CHECK(NULL != buffer, -1, "malloc %d bytes failed.\n", size);
    //
    //ret = sal_yuv_get(1, buffer, size);
    //CHECK(ret > 0, -1, "Error with %d.\n", ret);
    //
    //ret = util_file_write("yuvsp420", buffer, ret);
    //CHECK(ret == 0, -1, "Error with %d.\n", ret);
    
    /*int size1 = 1000*1000*1.5;
    char* buffer1 = malloc(size1);
    CHECK(NULL != buffer1, -1, "malloc %d bytes failed.\n", size1);
    cutYuv(NV21OR21, buffer1, buffer, 1000, 1000, 1000, 1000, 4000, 3000);
    
    ret = util_file_write("yuvsp4201", buffer1, size1);
    CHECK(ret == 0, -1, "Error with %d.\n", ret);*/

    return 0;
}

int sal_osd_fullscreen_exit()
{
    CHECK(NULL != g_osd_fullscreen_args, -1,"moudle is not inited.\n");

    int ret = -1;
    unsigned int i = 0;
    unsigned int j = 0;
    
    
    if (g_osd_fullscreen_args->running)
    {
        g_osd_fullscreen_args->running = 0;
        pthread_join(g_osd_fullscreen_args->pid, NULL);
    }

    pthread_mutex_destroy(&g_osd_fullscreen_args->mutex);
    free(g_osd_fullscreen_args);
    g_osd_fullscreen_args = NULL;

    return 0;
}



#include "hi_comm.h"
#include "sal_debug.h"
#include "sal_util.h"
#include "sal_vgs.h"
#include "sal_draw_rectangle.h"
//#include "libjpeg/jpeglib.h"
#include "libjpeg-turbo/jpeglib.h"

typedef struct sal_dr_args
{
    struct timeval current;
    int vpss_chn;

    int running;
    pthread_t pid;
    pthread_mutex_t mutex;
}sal_dr_args;

static sal_dr_args* g_dr_args = NULL;

int dr_cutYuv420sp(unsigned char *tarYuv, unsigned char *srcYuv, int startW,  
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

int dr_yuv420sp2jpeg(unsigned char** outbuffer, unsigned long* outsize, unsigned char* pdata,int image_width,int image_height, int quality)  
{     
    struct jpeg_compress_struct cinfo;    
    struct jpeg_error_mgr jerr;    
    cinfo.err = jpeg_std_error(&jerr);    
    jpeg_create_compress(&cinfo);    
  
    jpeg_mem_dest(&cinfo, outbuffer, outsize);

    cinfo.image_width = image_width;    // image width and height, in pixels    
    cinfo.image_height = image_height;    
    cinfo.input_components = 3;         // # of color components per pixel    
    cinfo.in_color_space = JCS_YCbCr;   //colorspace of input image    
    jpeg_set_defaults(&cinfo);    
    jpeg_set_quality(&cinfo, quality, TRUE);    

    cinfo.jpeg_color_space = JCS_YCbCr;    
    cinfo.comp_info[0].h_samp_factor = 2;    
    cinfo.comp_info[0].v_samp_factor = 2;    

    jpeg_start_compress(&cinfo, TRUE);    

    JSAMPROW row_pointer[1];  

    unsigned char *yuvbuf = (unsigned char *)malloc(image_width*3);
    CHECK(yuvbuf, -1, "malloc %d bytes failed.\n", image_width*3);
    memset(yuvbuf, 0, image_width*3);  

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
    free(yuvbuf);
    return 0;    
}  

int dr_yuv420sp2jpeg_gray(unsigned char** outbuffer, unsigned long* outsize, unsigned char* pdata,int image_width,int image_height, int quality)  
{     
    struct jpeg_compress_struct cinfo;    
    struct jpeg_error_mgr jerr;    
    cinfo.err = jpeg_std_error(&jerr);    
    jpeg_create_compress(&cinfo);    

    jpeg_mem_dest(&cinfo, outbuffer, outsize);

    cinfo.image_width = image_width;    // image width and height, in pixels    
    cinfo.image_height = image_height;    
    cinfo.input_components = 1;         // # of color components per pixel    
    cinfo.in_color_space = JCS_GRAYSCALE;   //colorspace of input image    
    jpeg_set_defaults(&cinfo);    
    jpeg_set_quality(&cinfo, quality, TRUE);    

    cinfo.jpeg_color_space = JCS_GRAYSCALE;    
    cinfo.comp_info[0].h_samp_factor = 2;    
    cinfo.comp_info[0].v_samp_factor = 2;    
    jpeg_start_compress(&cinfo, TRUE);    

    JSAMPROW row_pointer[1];
    int j = 0;
    while (cinfo.next_scanline < cinfo.image_height)   
    {  
        row_pointer[0] = pdata+j*image_width;  
        jpeg_write_scanlines(&cinfo, row_pointer, 1);  
        j++;  
    }

    jpeg_finish_compress(&cinfo);    
    jpeg_destroy_compress(&cinfo);    

    return 0;
}  

static void* dr_proc(void* args)
{
    prctl(PR_SET_NAME, __FUNCTION__);

    int s32Ret = -1;
    VIDEO_FRAME_INFO_S stFrame;
    memset(&stFrame, 0, sizeof(stFrame));

    VPSS_GRP VpssGrp = 0;
    HI_U32 u32Depth = 0;
    s32Ret = HI_MPI_VPSS_GetDepth(VpssGrp, g_dr_args->vpss_chn, &u32Depth);
    CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);
    
    if (u32Depth < 1)
    {
        u32Depth = 1;
        DBG("vpss_chn: %d, u32Depth: %u\n", g_dr_args->vpss_chn, u32Depth);
    }
    
    s32Ret = HI_MPI_VPSS_SetDepth(VpssGrp, g_dr_args->vpss_chn, u32Depth);
    CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);
    
    while (g_dr_args->running)
    {
        util_time_abs(&g_dr_args->current);
        
        s32Ret = HI_MPI_VPSS_GetChnFrame(VpssGrp, g_dr_args->vpss_chn, &stFrame, 10);
        if (s32Ret != HI_SUCCESS)
        {
            //DBG("HI_MPI_VPSS_GetChnFrame[%d] time out. Error with %#x.\n", g_dr_args->vpss_chn, s32Ret);
            continue;
        }
        else
        {
            unsigned int u32Size = (stFrame.stVFrame.u32Stride[0]) * (stFrame.stVFrame.u32Height) * 3 / 2;//yuv420
            unsigned char* yuv420sp = (unsigned char*) HI_MPI_SYS_Mmap(stFrame.stVFrame.u32PhyAddr[0], u32Size);
            CHECK(yuv420sp, NULL, "error with %#x.\n", yuv420sp);
            
            DBG("u32PoolId: %u\n", stFrame.u32PoolId);
            DBG("u32Width: %u\n", stFrame.stVFrame.u32Width);
            DBG("u32Height: %u\n", stFrame.stVFrame.u32Height);
            DBG("u32Stride: %u %u %u\n", stFrame.stVFrame.u32Stride[0], stFrame.stVFrame.u32Stride[1], stFrame.stVFrame.u32Stride[2]);

            int cutW = 192;
            int cutH = 192;
            int size = cutW*cutH*3/2;
            char* dstBuff = malloc(size);
            dr_cutYuv420sp(dstBuff, yuv420sp, 100, 100, cutW, cutH, stFrame.stVFrame.u32Width, stFrame.stVFrame.u32Height);
            
            char* dstBuff1 = malloc(size);
            dr_cutYuv420sp(dstBuff1, yuv420sp, 200, 100, cutW, cutH, stFrame.stVFrame.u32Width, stFrame.stVFrame.u32Height);
            
            memcpy(yuv420sp, dstBuff, size);
            stFrame.stVFrame.u32Width = cutW;
            stFrame.stVFrame.u32Height = cutH;
            stFrame.stVFrame.u32Stride[0] = cutW;
            stFrame.stVFrame.u32Stride[1] = cutW;
            stFrame.stVFrame.u32Stride[2] = cutW;
            stFrame.stVFrame.u32PhyAddr[0] = stFrame.stVFrame.u32PhyAddr[0];
            stFrame.stVFrame.u32PhyAddr[1] = stFrame.stVFrame.u32PhyAddr[0] + stFrame.stVFrame.u32Stride[0]*stFrame.stVFrame.u32Height;
            stFrame.stVFrame.u32PhyAddr[2] = stFrame.stVFrame.u32PhyAddr[1] + stFrame.stVFrame.u32Stride[1]*stFrame.stVFrame.u32Height/4;

            DBG("begin send ...\n");
            s32Ret = HI_MPI_VENC_SendFrame(3, &stFrame, -1);
            CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);
            //DBG("send ok.\n");

            //jpeg_get_one_picture();
            
            DBG("begin send ...\n");
            s32Ret = HI_MPI_VENC_SendFrame(3, &stFrame, -1);
            CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);
            //DBG("send ok.\n");

            //jpeg_get_one_picture();
            
            ////
            memcpy(yuv420sp, dstBuff1, size);
            stFrame.stVFrame.u32Width = cutW;
            stFrame.stVFrame.u32Height = cutH;
            stFrame.stVFrame.u32Stride[0] = cutW;
            stFrame.stVFrame.u32Stride[1] = cutW;
            stFrame.stVFrame.u32Stride[2] = cutW;
            stFrame.stVFrame.u32PhyAddr[0] = stFrame.stVFrame.u32PhyAddr[0];
            stFrame.stVFrame.u32PhyAddr[1] = stFrame.stVFrame.u32PhyAddr[0] + stFrame.stVFrame.u32Stride[0]*stFrame.stVFrame.u32Height;
            stFrame.stVFrame.u32PhyAddr[2] = stFrame.stVFrame.u32PhyAddr[1] + stFrame.stVFrame.u32Stride[1]*stFrame.stVFrame.u32Height/4;
            
            DBG("u32PoolId: %u\n", stFrame.u32PoolId);
            DBG("u32Width: %u\n", stFrame.stVFrame.u32Width);
            DBG("u32Height: %u\n", stFrame.stVFrame.u32Height);
            DBG("u32Stride: %u %u %u\n", stFrame.stVFrame.u32Stride[0], stFrame.stVFrame.u32Stride[1], stFrame.stVFrame.u32Stride[2]);
            
            DBG("begin send ...\n");
            s32Ret = HI_MPI_VENC_SendFrame(3, &stFrame, -1);
            CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);
            //DBG("send ok.\n");

            //jpeg_get_one_picture();
            
            s32Ret = HI_MPI_SYS_Munmap(yuv420sp, u32Size);
            CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);
        }
        s32Ret = HI_MPI_VPSS_ReleaseChnFrame(VpssGrp, g_dr_args->vpss_chn, &stFrame);
        CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);
        
        break;
    }

    return NULL;
}

static void* dr_proc1(void* args)
{
    prctl(PR_SET_NAME, __FUNCTION__);

    int s32Ret = -1;
    VIDEO_FRAME_INFO_S stFrame;
    memset(&stFrame, 0, sizeof(stFrame));

    VI_CHN ViChn = 0;
    HI_U32 u32Depth = 0;

    s32Ret = HI_MPI_VI_GetFrameDepth(ViChn, &u32Depth);
    CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);

    if (u32Depth < 1)
    {
        u32Depth = 1;
        DBG("vi_chn: %d, u32Depth: %u\n", ViChn, u32Depth);
    }

    s32Ret = HI_MPI_VI_SetFrameDepth(ViChn, u32Depth);
    CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);
    
    while (g_dr_args->running)
    {
        util_time_abs(&g_dr_args->current);
        
        s32Ret = HI_MPI_VI_GetFrame(ViChn, &stFrame, 10);
        if (s32Ret != HI_SUCCESS)
        {
            //DBG("HI_MPI_VPSS_GetChnFrame[%d] time out. Error with %#x.\n", g_dr_args->vpss_chn, s32Ret);
            continue;
        }
        else
        {
            unsigned int u32Size = (stFrame.stVFrame.u32Stride[0]) * (stFrame.stVFrame.u32Height) * 3 / 2;//yuv420
            unsigned char* yuv420sp = (unsigned char*) HI_MPI_SYS_Mmap(stFrame.stVFrame.u32PhyAddr[0], u32Size);
            CHECK(yuv420sp, NULL, "error with %#x.\n", yuv420sp);
            
            DBG("u32PoolId: %u\n", stFrame.u32PoolId);
            DBG("u32Width: %u\n", stFrame.stVFrame.u32Width);
            DBG("u32Height: %u\n", stFrame.stVFrame.u32Height);
            DBG("u32Stride: %u %u %u\n", stFrame.stVFrame.u32Stride[0], stFrame.stVFrame.u32Stride[1], stFrame.stVFrame.u32Stride[2]);
            
            int cutW = 192;
            int cutH = 192;
            int size = cutW*cutH*3/2;
            char* dstBuff = malloc(size);
            dr_cutYuv420sp(dstBuff, yuv420sp, 100, 100, cutW, cutH, stFrame.stVFrame.u32Width, stFrame.stVFrame.u32Height);
            
            memcpy(yuv420sp, dstBuff, size);
            stFrame.stVFrame.u32Width = cutW;
            stFrame.stVFrame.u32Height = cutH;
            stFrame.stVFrame.u32Stride[0] = cutW;
            stFrame.stVFrame.u32Stride[1] = cutW;
            stFrame.stVFrame.u32Stride[2] = cutW;
            stFrame.stVFrame.u32PhyAddr[0] = stFrame.stVFrame.u32PhyAddr[0];
            stFrame.stVFrame.u32PhyAddr[1] = stFrame.stVFrame.u32PhyAddr[0] + stFrame.stVFrame.u32Stride[0]*stFrame.stVFrame.u32Height;
            stFrame.stVFrame.u32PhyAddr[2] = stFrame.stVFrame.u32PhyAddr[1] + stFrame.stVFrame.u32Stride[1]*stFrame.stVFrame.u32Height/4;
            
            
            VIDEO_FRAME_INFO_S stFrameBak;
            memcpy(&stFrameBak, &stFrame, sizeof(stFrame));
            
            DBG("begin send ...\n");
            jpeg1_create_chn(3, cutW, cutH);
            
            s32Ret = HI_MPI_VENC_SendFrame(3, &stFrameBak, -1);
            CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);
            //DBG("send ok.\n");
            unsigned char* out_buffer = NULL;
            int out_size = 0;
            jpeg1_get_picture(3, &out_buffer, &out_size);
            jpeg1_release_picture(3, &out_buffer, &out_size);
            jpeg1_destroy_chn(3);
            
            //DBG("destroy ok.\n");
            jpeg1_create_chn(3, cutW, cutH);
            // 
            //s32Ret = HI_MPI_VENC_ResetChn(3);
            //CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);
            
            //s32Ret = HI_MPI_VENC_StartRecvPic(3);
            //CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

            s32Ret = HI_MPI_VENC_SendFrame(3, &stFrame, -1);
            CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);
            //DBG("send ok.\n");
            jpeg1_get_picture(3, &out_buffer, &out_size);
            jpeg1_release_picture(3, &out_buffer, &out_size);
            jpeg1_destroy_chn(3);
            DBG("end.\n");
            
            s32Ret = HI_MPI_SYS_Munmap(yuv420sp, u32Size);
            CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);
        }
        s32Ret = HI_MPI_VI_ReleaseFrame(ViChn, &stFrame);
        CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);
        
        break;
    }

    return NULL;
}

int sal_dr_init()
{
    CHECK(NULL == g_dr_args, -1, "reinit error, please uninit first.\n");

    int ret = -1;

    g_dr_args = (sal_dr_args*)malloc(sizeof(sal_dr_args));
    CHECK(NULL != g_dr_args, -1, "malloc %d bytes failed.\n", sizeof(sal_dr_args));

    memset(g_dr_args, 0, sizeof(sal_dr_args));
    pthread_mutex_init(&g_dr_args->mutex, NULL);
    
    g_dr_args->vpss_chn = 1;

    g_dr_args->running = 1;
    ret = pthread_create(&g_dr_args->pid, NULL, dr_proc1, NULL);
    CHECK(ret == 0, -1, "Error with %s.\n", strerror(errno));

    return 0;
}

int sal_dr_exit()
{


    return 0;
}



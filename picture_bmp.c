#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <malloc.h>
#include "sal_debug.h"
#include "sal_util.h"
#include "picture_bmp.h"


//https://blog.csdn.net/yixianfeng41/article/details/52591585


static int bmp_createRGBdata(unsigned char* rgbout,int width,int height)
{
    unsigned int idx=0;
    int j = 0;
    int i = 0;
    for(j=0;j<height;j++) 
    {
        idx=(height-j-1)*width*3;//该值保证所生成的rgb数据逆序存放在rgbbuf中
        for(i=0;i<width;i++)
        {           
            unsigned char r,g,b;
            //生成一个包含红，蓝，绿，黄，黑颜色的条纹图案吧
            if(j/100==0)//红色
            {
                r=246;
                g=30;
                b=8;
            }
            else if(j/100==1)//蓝色
            {
                r=15;
                g=33;
                b=208;
            }
            else if(j/100==2)//绿色
            {
                r=13;
                g=246;
                b=8;
            }
            else if(j/100==3)//黄色
            {
                r=246;
                g=243;
                b=8;
            }
            else
            {
                r=0;
                g=0;
                b=0;
            }

            rgbout[idx++]=b;
            rgbout[idx++]=g;
            rgbout[idx++]=r;
        }
    }
    return 0;
}

int bmp_rgb2yuv(unsigned char r, unsigned char g, unsigned char b, unsigned char* y, unsigned char* u, unsigned char* v)
{
    *y = (unsigned char)(0.299*r + 0.587*g + 0.114*b);
    *u = (unsigned char)(-0.169 * r - 0.331 * g + 0.500 * b + 128);
    *v = (unsigned char)(0.500* r - 0.419 * g - 0.081 * b + 128);
    
    return 0;
}

int bmp_yuv2rgb(unsigned char y, unsigned char u, unsigned char v, unsigned char* r, unsigned char* g, unsigned char* b)
{
    *b=(unsigned char)(y+1.779*(u- 128));
    *g=(unsigned char)(y-0.7169*(v - 128)-0.3455*(u - 128));
    *r=(unsigned char)(y+ 1.4075*(v - 128));
    
    return 0;
}

int bmp_rgb2bgr(unsigned char* buffer, int width, int height)
{
    int bytePerPixel = 3;
    unsigned int size = width * height * bytePerPixel;
    unsigned char* src = (unsigned char*)malloc(size);
    CHECK(src, -1, "failed to malloc %d bytes \n", size);
    memset(src, 0, size);
    
    memcpy(src, buffer, size);
    int i = 0;
    int j = 0;
    int idx_src = 0;
    int idx_dst = 0;
    
    for (i=0; i<height; i++) 
    {
        idx_src=i*width*bytePerPixel;           //从左到右从上到下
        idx_dst=(height-i-1)*width*bytePerPixel;//从左到右从下到上
        for (j=0; j<width; j++)
        {
            buffer[idx_dst+0]=src[idx_src+2]; //b
            buffer[idx_dst+1]=src[idx_src+1]; //g
            buffer[idx_dst+2]=src[idx_src+0]; //r
            idx_src += bytePerPixel;
            idx_dst += bytePerPixel;
        }
    }
    
    free(src);
    return 0;
}

int bmp_bgr2rgb(unsigned char* buffer, int width, int height)
{
    int bytePerPixel = 3;
    unsigned int size = width * height * bytePerPixel;
    unsigned char* src = (unsigned char*)malloc(size);
    CHECK(src, -1, "failed to malloc %d bytes \n", size);
    memset(src, 0, size);

    memcpy(src, buffer, size);
    int i = 0;
    int j = 0;
    int idx_src = 0;
    int idx_dst = 0;

    for (i=0; i<height; i++) 
    {
        idx_src=(height-i-1)*width*bytePerPixel;           //从左到右从下到上
        idx_dst=i*width*bytePerPixel;                      //从左到右从上到下
        for (j=0; j<width; j++)
        {
            buffer[idx_dst+2]=src[idx_src+0]; //b
            buffer[idx_dst+1]=src[idx_src+1]; //g
            buffer[idx_dst+0]=src[idx_src+2]; //r
            idx_src += bytePerPixel;
            idx_dst += bytePerPixel;
        }
    }

    free(src);
    return 0;
}

int bmp_save2file(const char* path, unsigned char* bgrbuffer,int width,int height)
{
    int ret = -1;
    MyBITMAPFILEHEADER bfh;
    memset(&bfh, 0, sizeof(bfh));
    MyBITMAPINFOHEADER bih;
    memset(&bih, 0, sizeof(bih));

    /* Magic number for file. It does not fit in the header structure due to alignment requirements, so put it outside */
    unsigned short bfType=0x4d42;           
    bfh.bfReserved1 = 0;
    bfh.bfReserved2 = 0;
    bfh.bfSize = 2+sizeof(MyBITMAPFILEHEADER) + sizeof(MyBITMAPINFOHEADER)+width*height*3;
    bfh.bfOffBits = 0x36;

    bih.biSize = sizeof(MyBITMAPINFOHEADER);
    bih.biWidth = width;
    bih.biHeight = height;
    bih.biPlanes = 1;
    bih.biBitCount = 24;
    bih.biCompression = 0;
    bih.biSizeImage = 0;
    bih.biXPelsPerMeter =0;
    bih.biYPelsPerMeter =0;
    bih.biClrUsed = 0;
    bih.biClrImportant = 0;

    unsigned int size = sizeof(bfType) + sizeof(bfh) + sizeof(bih) + width*height*3;
    unsigned char* bmp_buffer = (unsigned char*)malloc(size);
    CHECK(bmp_buffer, -1, "failed to malloc %d bytes \n", size);
    memset(bmp_buffer, 0, size);

    int offset = 0;
    memcpy(bmp_buffer+offset, &bfType, sizeof(bfType));
    offset += sizeof(bfType);
    memcpy(bmp_buffer+offset, &bfh, sizeof(bfh));
    offset += sizeof(bfh);
    memcpy(bmp_buffer+offset, &bih, sizeof(bih));
    offset += sizeof(bih);

    memcpy(bmp_buffer+offset, bgrbuffer, width*height*3);
    offset += width*height*3;
    
    ret = util_file_write(path, bmp_buffer, size);
    CHECK(ret == 0, -1, "error with: %#x\n", ret);
    
    free(bmp_buffer);
    return 0;
}

int bmp_fileParse(const char* path, unsigned char** outBgrBuffer, unsigned int* outsize, int* width, int* height)
{
    int ret = -1;
    int filesize = util_file_size(path);
    CHECK(filesize > 0, -1, "error with: %#x\n", filesize);
    
    unsigned char* filebuffer = (unsigned char*)malloc(filesize);
    CHECK(filebuffer, -1, "failed to malloc %d bytes \n", filesize);
    memset(filebuffer, 0, filesize);
    
    ret = util_file_read(path, filebuffer, filesize);
    CHECK(ret == filesize, -1, "error with: %#x\n", ret);

    MyBITMAPFILEHEADER bfh;
    memset(&bfh, 0, sizeof(bfh));
    MyBITMAPINFOHEADER bih;
    memset(&bih, 0, sizeof(bih));

    unsigned short bfType = 0;
    int offset = 0;
    memcpy(&bfType, filebuffer+offset, sizeof(bfType));
    offset += sizeof(bfType);
    memcpy(&bfh, filebuffer+offset, sizeof(bfh));
    offset += sizeof(bfh);
    memcpy(&bih, filebuffer+offset, sizeof(bih));
    offset += sizeof(bih);

    DBG("bfType: %#x\n", bfType);
    DBG("filesize: %d\n", bfh.bfSize);
    DBG("biWidth: %d\n", bih.biWidth);
    DBG("biHeight: %d\n", bih.biHeight);
    DBG("biBitCount: %d\n", bih.biBitCount);
    DBG("biSize: %d\n", bih.biSize);
    DBG("biPlanes: %d\n", bih.biPlanes);
    DBG("biCompression: %d\n", bih.biCompression);
    DBG("biSizeImage: %d\n", bih.biSizeImage);
    DBG("biXPelsPerMeter: %d\n", bih.biXPelsPerMeter);
    DBG("biYPelsPerMeter: %d\n", bih.biYPelsPerMeter);
    DBG("biClrUsed: %d\n", bih.biClrUsed);
    DBG("biClrImportant: %d\n", bih.biClrImportant);
    
    CHECK(bih.biBitCount/8 == 3, -1, "error with: %#x\n", bih.biBitCount);

    unsigned int bgrsize = bih.biWidth*bih.biHeight*3;
    unsigned char* bgrbuf = (unsigned char*)malloc(bgrsize);
    CHECK(bgrbuf, -1, "failed to malloc %d bytes \n", bgrsize);
    memset(bgrbuf, 0, bgrsize);

    memcpy(bgrbuf, filebuffer+offset, bgrsize);
    offset += bgrsize;

    *outBgrBuffer = bgrbuf;
    *outsize = bgrsize;
    *width = bih.biWidth;
    *height = bih.biHeight;

    free(filebuffer);
    return 0;
}

int bmp_rgb2rgb1555(unsigned char* rgb888buffer, unsigned char* rgb1555buffer, int width, int height)
{
    unsigned char r, g, b;
    r = g = b = 0;
    unsigned short pixel = 0;
    int bytePerPixel = 3;
    unsigned short* dst = (unsigned short*)rgb1555buffer;

    int i = 0;
    int j = 0;
    int idx_src = 0;

    for (i=0; i<height; i++) 
    {
        idx_src=i*width*bytePerPixel;           //从左到右从上到下
        for (j=0; j<width; j++)
        {
            r = rgb888buffer[idx_src+0] >> 3; //r
            g = rgb888buffer[idx_src+1] >> 3; //g
            b = rgb888buffer[idx_src+2] >> 3; //b
            pixel = (1 << 15);
            pixel |= (b | (g << 5) | (r << (5 + 5)));
            //DBG("pixel: %#x, rgb %d %d %d r1g1b1 %d %d %d\n", pixel, rgb888buffer[idx_src+0], rgb888buffer[idx_src+1], rgb888buffer[idx_src+b], r, g, b);
            dst[i*width+j] = pixel;
            idx_src += bytePerPixel;
        }
    }

    return 0;
}

int bmp_read(unsigned char** outbuffer, unsigned int* outsize)
{
    int ret = -1;
    unsigned char* bgrBuffer = NULL;
    unsigned int bgrBufferSize = 0;
    int width = 0;
    int height = 0;
    ret = bmp_fileParse("test.bmp", &bgrBuffer, &bgrBufferSize, &width, &height);
    CHECK(ret == 0, -1, "error with: %#x\n", ret);
    
    CHECK(bgrBuffer, -1, "error with: %#x\n", bgrBuffer);
    CHECK(bgrBufferSize > 0, -1, "error with: %#x\n", bgrBufferSize);
    CHECK(width > 0, -1, "error with: %#x\n", width);
    CHECK(height > 0, -1, "error with: %#x\n", height);
    
    DBG("bgrBuffer: %p, bgrBufferSize: %d wh[%dx%d]\n", bgrBuffer, bgrBufferSize, width, height);
    
    ret = bmp_bgr2rgb(bgrBuffer, width, height);
    CHECK(ret == 0, -1, "error with: %#x\n", ret);
    
    unsigned int size = width * height * 2;
    unsigned char* rgb1555buffer = (unsigned char*)malloc(size);
    CHECK(rgb1555buffer, -1, "failed to malloc %d bytes \n", size);
    memset(rgb1555buffer, 0, size);
    
    ret = bmp_rgb2rgb1555(bgrBuffer, rgb1555buffer, width, height);
    CHECK(ret == 0, -1, "error with: %#x\n", ret);

    *outbuffer = rgb1555buffer;
    *outsize = size;
    
    free(bgrBuffer);
    return 0;
}

int bmp_copy(unsigned char* src_buffer, int src_width, int src_height, 
            unsigned char* dst_buffer, int dst_width, int dst_height, int bpp, int start_widht, int start_height)
{
    CHECK((src_width+start_widht) <= dst_width, -1, "error with:src_width[%d] start_widht[%d] dst_width[%d]\n",src_width, start_widht, dst_width);
    CHECK((src_height+start_height) <= dst_height, -1, "error with:src_height[%d] start_height[%d] dst_height[%d]\n",src_height, start_height, dst_height);
    
    int i = 0;
    int offset = 0;
    
    for (i = start_height; i < src_height+start_height; i++)
    {
        unsigned char* dst_begin = dst_buffer + i*dst_width*bpp + start_widht*bpp;
        memcpy(src_buffer+offset, dst_begin, src_width*bpp);
        offset += src_width*bpp;
    }

    return 0;
}

int bmp_demo()
{
    int width = 400;
    int height = 400;
    unsigned char* rgbbuffer = (unsigned char*)malloc(width*height*3);
    memset(rgbbuffer, 0, width*height*3);
    bmp_createRGBdata(rgbbuffer, width, height);

    bmp_save2file("test.bmp", rgbbuffer, width, height);

    free(rgbbuffer);
    return 0;
}


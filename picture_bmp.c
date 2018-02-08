#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <malloc.h>

#include "sal_debug.h"
#include "sal_util.h"
#include "picture_bmp.h"

#if 1
void CreateRGBData(unsigned char *rgbout,int width,int height)
{
    unsigned long  idx=0;
    int j = 0;
    int i = 0;
    for(j=0;j<height;j++) 
    {
        idx=(height-j-1)*width*3;//该值保证所生成的rgb数据逆序存放在rgbbuf中
        for(i=0;i<width;i++)
        {           
            unsigned char r,g,b;
            //生成一个包含红，蓝，绿，黄四种颜色的条纹图案吧
            if(j/100==0)//红色
            {
                r=13;
                g=246;
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
                r=246;
                g=30;
                b=8;
            }
            else if(j/100==3)//黄色
            {
                r=246;
                g=243;
                b=8;
            }
            rgbout[idx++]=b;
            rgbout[idx++]=r;
            rgbout[idx++]=g;
        }
    }
}

int bmp_create(unsigned char* bgrbuf,int width,int height, unsigned char** outbuffer, unsigned int* outsize)
{
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
    CHECK(bmp_buffer, -1, "malloc %d bytes failed.\n", size);
    memset(bmp_buffer, 0, size);
    
    int offset = 0;
    memcpy(bmp_buffer+offset, &bfType, sizeof(bfType));
    offset += sizeof(bfType);
    memcpy(bmp_buffer+offset, &bfh, sizeof(bfh));
    offset += sizeof(bfh);
    memcpy(bmp_buffer+offset, &bih, sizeof(bih));
    offset += sizeof(bih);

    memcpy(bmp_buffer+offset, bgrbuf, width*height*3);
    offset += width*height*3;
    
    *outbuffer = bmp_buffer;
    *outsize = size;
    
    return 0;
}

int bmp_demo()
{
    unsigned char* rgbbuffer;
    rgbbuffer=(unsigned char*)malloc(400*400*3);
    memset(rgbbuffer,0,400*400*3);
    CreateRGBData(rgbbuffer,400,400);
    
    unsigned char* bmp = NULL;
    int size = 0;
    bmp_create(rgbbuffer,400,400, &bmp, &size);
    util_file_write("test.bmp", bmp, size);
    free(rgbbuffer);
    free(bmp);
    return 0;
}

#endif

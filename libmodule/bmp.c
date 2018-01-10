#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <malloc.h>

#include "sal_debug.h"

#if 0
void CreateRGBData(unsigned char *rgbout,int width,int height)
{
    unsigned long  idx=0;
    for(int j=0;j<height;j++) 
    {
        idx=(height-j-1)*width*3;//该值保证所生成的rgb数据逆序存放在rgbbuf中
        for(int i=0;i<width;i++)
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

void MySaveBmp(string  filename,unsigned char *rgbbuf,int width,int height)
{
    MyBITMAPFILEHEADER bfh;
    MyBITMAPINFOHEADER bih;
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

    FILE *file=NULL;
    fopen_s(&file,"c:\\MyTest.bmp", "wb");
    if (!file)
    {
        printf("Could not write file\n");
        return;
    }

    fwrite(&bfType,sizeof(bfType),1,file);
    fwrite(&bfh, sizeof(bfh),1 , file);
    fwrite(&bih, sizeof(bih),1 , file);

    fwrite(rgbbuf,width*height*3,1,file);
    fclose(file);
}

int main()
{
    unsigned char* rgbbuffer;
    rgbbuffer=(unsigned char*)malloc(400*400*3);
    memset(rgbbuffer,0,400*400*3);
    CreateRGBData(rgbbuffer,400,400);
    MySaveBmp("C;\\MyTest.bmp",rgbbuffer,400,400);
    free(rgbbuffer);
    return 0;
}

#endif

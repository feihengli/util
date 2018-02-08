#ifndef __bmp_h__
#define __bmp_h__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif


typedef struct                       /**** BMP file header structure ****/
{
    unsigned int   bfSize;           /* Size of file */
    unsigned short bfReserved1;      /* Reserved */
    unsigned short bfReserved2;      /* ... */
    unsigned int   bfOffBits;        /* Offset to bitmap data */
} MyBITMAPFILEHEADER;

typedef struct                       /**** BMP file info structure ****/
{
    unsigned int   biSize;           /* Size of info header */
    int            biWidth;          /* Width of image */
    int            biHeight;         /* Height of image */
    unsigned short biPlanes;         /* Number of color planes */
    unsigned short biBitCount;       /* Number of bits per pixel */
    unsigned int   biCompression;    /* Type of compression to use */
    unsigned int   biSizeImage;      /* Size of image data */
    int            biXPelsPerMeter;  /* X pixels per meter */
    int            biYPelsPerMeter;  /* Y pixels per meter */
    unsigned int   biClrUsed;        /* Number of colors used */
    unsigned int   biClrImportant;   /* Number of important colors */
}MyBITMAPINFOHEADER;

typedef struct tagRGBQUAD
{  
    unsigned char rgbBlue;//蓝色的亮度（值范围为0-255)  
    unsigned char rgbGreen;//绿色的亮度（值范围为0-255)  
    unsigned char rgbRed;//红色的亮度（值范围为0-255)  
    unsigned char rgbReserved;//保留，必须为0  
}RGBQUAD;




#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif // __bmp_h__

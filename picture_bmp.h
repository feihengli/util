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


/*
 函 数 名: bmp_rgb2yuv
 功能描述: rgb数据转换为yuv数据
 输入参数: r 红色分量
            g 绿色分量
            b 蓝色分量
 输出参数: y 亮度分量
            u cb色度分量
            v cr色度分量
 返 回 值: 成功返回0,失败返回小于0
*/
int bmp_rgb2yuv(unsigned char r, unsigned char g, unsigned char b, unsigned char* y, unsigned char* u, unsigned char* v);

/*
 函 数 名: bmp_yuv2rgb
 功能描述: yuv数据转换为rgb数据
 输入参数: y 亮度分量
            u cb色度分量
            v cr色度分量
 输出参数: r 红色分量
            g 绿色分量
            b 蓝色分量
 返 回 值: 成功返回0,失败返回小于0
*/
int bmp_yuv2rgb(unsigned char y, unsigned char u, unsigned char v, unsigned char* r, unsigned char* g, unsigned char* b);

/*
 函 数 名: bmp_rgb2bgr
 功能描述: rgb数据转换为bgr数据，一个像素占3字节。
            rgb数据是从左到右从上到下的rgb顺序数据；bgr数据是从左到右从下到上的bgr顺序数据；
 输入参数: buffer 输入数据，同事也作为输出。大小为width*height*3
            width 宽
            height 高
 输出参数: buffer 输出数据，同事也作为输入。大小为width*height*3
 返 回 值: 成功返回0,失败返回小于0
*/
int bmp_rgb2bgr(unsigned char* buffer, int width, int height);

/*
 函 数 名: bmp_bgr2rgb
 功能描述: bgr数据转换为rgb数据，一个像素占3字节。
            rgb数据是从左到右从上到下的rgb顺序数据；bgr数据是从左到右从下到上的bgr顺序数据；
 输入参数: buffer 输入数据，同事也作为输出。大小为width*height*3
            width 宽
            height 高
 输出参数: buffer 输出数据，同事也作为输入。大小为width*height*3
 返 回 值: 成功返回0,失败返回小于0
*/
int bmp_bgr2rgb(unsigned char* buffer, int width, int height);

/*
 函 数 名: bmp_save2file
 功能描述: 把bgr图像数据保存到文件。
 输入参数: path 文件路径
            bgrbuffer 图像数据
            width 宽
            height 高
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int bmp_save2file(const char* path, unsigned char* bgrbuffer,int width,int height);

/*
 函 数 名: bmp_fileParse
 功能描述: 把bmp文件解析出bgr数据。
 输入参数: path 文件路径
 输出参数: outBgrBuffer 输出bgr数据
            outsize outBgrBuffer大小
            width 宽
            height 高
 返 回 值: 成功返回0,失败返回小于0
*/
int bmp_fileParse(const char* path, unsigned char** outBgrBuffer, unsigned int* outsize, int* width, int* height);

/*
 函 数 名: bmp_rgb2rgb1555
 功能描述: 把rgb888数据转换为rgb1555数据。
            rgb888一个像素占3字节；rgb1555一个像素占2字节，其中最高位标识alpha（透明度）
 输入参数: rgb888buffer rgb888数据，大小为width*height*3
            width 宽
            height 高
 输出参数: rgb1555buffer rgb1555buffer数据，大小为width*height*2
 返 回 值: 成功返回0,失败返回小于0
*/
int bmp_rgb2rgb1555(unsigned char* rgb888buffer, unsigned char* rgb1555buffer, int width, int height);

/*
 函 数 名: bmp_copy
 功能描述: 小矩形拷贝到大矩形的buffer数据。
 输入参数: src_buffer 源矩形，大小为src_width*src_height*bpp
            src_width 源宽
            src_height 源高
            dst_width 目的宽
            dst_height 目的高
            bpp 一个像素占的字节数（byte per pixel）
            start_widht 开始拷贝的起点宽
            start_height 开始拷贝的起点高
 输出参数: dst_buffer 目的矩形，大小为dst_width*dst_height*bpp
 返 回 值: 成功返回0,失败返回小于0
*/
int bmp_copy(unsigned char* src_buffer, int src_width, int src_height, 
             unsigned char* dst_buffer, int dst_width, int dst_height, int bpp, int start_widht, int start_height);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif // __bmp_h__

#ifndef __SAL_FONT_H__
#define __SAL_FONT_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#define MAX_FONT_NUM 128   //字体缓存池存放的最大字数

typedef struct
{
    int reference;         //引用计数
    int enc_len;           //汉字编码长度
    unsigned char enc[2];  //汉字编码
    int font_size; // default font size 1
    int row;               //汉字点阵行数
    int col;               //汉字点阵列数
    unsigned char* array; // 汉字点阵 row*col
}font_s;


/*
 函 数 名: font_init
 功能描述: 字体缓存池初始化，需与font_exit配对使用
 输入参数: 无
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int font_init();

/*
 函 数 名: font_exit
 功能描述: 字体缓存池初始化，需与font_init配对使用
 输入参数: 无
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int font_exit();

/*
 函 数 名: font_add
 功能描述: 往字体缓存池增加字体点阵相关数据
 输入参数: enc 汉字编码
            len 汉字编码长度
            src 汉字点阵
            row 汉字点阵行数
            col 汉字点阵列数
            font_size 字体大小（16x16的字体大小为1，32x32的字体大小为2，以此类推）
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int font_add(const unsigned char* enc, int len, char* src, int row, int col, int font_size);

/*
 函 数 名: font_get
 功能描述: 获取字体缓存池中的字体点阵相关数据，需与font_release配对使用
 输入参数: enc 汉字编码
            len 汉字编码长度
            font_size 字体大小（16x16的字体大小为1，32x32的字体大小为2，以此类推）
 输出参数: 无
 返 回 值: 成功返回字体点阵相关数据（外部不可更改）,失败返回NULL
*/
font_s* font_get(const unsigned char* enc, int len, int font_size);

/*
 函 数 名: font_get
 功能描述: 释放字体缓存池中的字体点阵相关数据，需与font_get配对使用
 输入参数: font 字体点阵相关数据
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int font_release(font_s* font);

/*
 函 数 名: font_get_num
 功能描述: 获取字体缓存池中实际缓存的汉字数量
 输入参数: 无
 输出参数: 无
 返 回 值: 成功返回缓存的汉字数量,失败返回小于0
*/
int font_get_num();




#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif

#ifndef __SAL_BITMAP_H__
#define __SAL_BITMAP_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#define FORE_GROUND 'X'  //汉字点阵前景（即有效需要显示的字符）
#define BACK_GROUND '-'  //汉字点阵背景（即不需要显示的字符）
#define FONT_EDGE 'o'    //汉字点阵前景的四周边缘

#define MAX_LINE_NUM 20   //最大的行数，通过 ^ 符号来进行换行
#define MAX_LINE_PLL 100  //每行的最大字节数

typedef struct
{
    int row;   //汉字点阵行数
    int col;   //汉字点阵列数
    char* bit_map;  //汉字点阵
}result_s;


/*
 函 数 名: bitmap_create
 功能描述: 初始化bitmap模块，需与bitmap_destroy配对使用
 输入参数: hz_path 汉字字库文件路径
            is_gbk 是否为gbk字库
            asc_path asc16字库文件路径
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int bitmap_create(const char* hz_path, int is_gbk, const char* asc_path);

/*
 函 数 名: bitmap_destroy
 功能描述: 去初始化bitmap模块，需与bitmap_create配对使用
 输入参数: 无
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int bitmap_destroy();

/*
 函 数 名: bitmap_alloc
 功能描述: 获取一段文字的bitmap，需与bitmap_free配对使用
 输入参数: chs 汉字句子
            font_size 字体大小
            edge_enable 是否进行包边处理
            gap 行与行之间的间隙
 输出参数: result 输出相关的bitmap，外部不可更改
 返 回 值: 成功返回0,失败返回小于0
*/
int bitmap_alloc(const unsigned char* chs, int font_size, int edge_enable, int gap, result_s* result);

/*
 函 数 名: bitmap_free
 功能描述: 释放一段文字的bitmap，需与bitmap_alloc配对使用
 输入参数: result 相关的bitmap数据
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int bitmap_free(result_s* result);

/*
 函 数 名: bitmap_demo
 功能描述: 使用此模块的demo，供参考
 输入参数: 无
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int bitmap_demo();




#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif

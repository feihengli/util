
#ifndef sal_vgs_h__
#define sal_vgs_h__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#include "hi_comm.h"

typedef struct rectangle_s
{
    int x;      //矩形起点横坐标
    int y;      //矩形起点纵坐标
    int width;  //矩形宽度
    int height; //矩形高度
}rectangle_s;

/*
 函 数 名: sal_vgs_draw_rectangle
 功能描述: 画矩形
 输入参数: pstOutFrameInfo 帧信息，一般从vpss模块获取
            u32Thick 线宽 2字节对齐
            u32Color 颜色 低3字节表示RGB eg.0x00FF00EE R: FF, G: 00, B: EE
            x 矩形起点横坐标
            y 矩形起点纵坐标
            width 矩形宽度
            height 矩形高度
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int sal_vgs_draw_rectangle(VIDEO_FRAME_INFO_S* pstOutFrameInfo, unsigned int u32Thick, unsigned int u32Color
                           , int x, int y, int width, int height);

/*
 函 数 名: sal_vgs_draw_rectangle
 功能描述: 画矩形（多个）
 输入参数: pstOutFrameInfo 帧信息，一般从vpss模块获取
            u32Thick 线宽 2字节对齐
            u32Color 颜色 低3字节表示RGB eg.0x00FF00EE R: FF, G: 00, B: EE
            pstArectangle 矩形信息数组
            num 数组个数
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int sal_vgs_draw_rectangle1(VIDEO_FRAME_INFO_S* pstOutFrameInfo, unsigned int u32Thick, unsigned int u32Color
                            , rectangle_s* pstArectangle, int num);

/*
 函 数 名: sal_vgs_draw_osd
 功能描述: 画OSD
 输入参数: pstOutFrameInfo 帧信息，一般从vpss模块获取
            buffer RGB1555数据
            width 宽 2字节对齐
            height 高 2字节对齐
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int sal_vgs_draw_osd(VIDEO_FRAME_INFO_S* pstOutFrameInfo, unsigned char* buffer, int width,int height);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif


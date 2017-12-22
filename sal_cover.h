
#ifndef __SAL_COVER_H__
#define __SAL_COVER_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#define INVALID_REGION_HANDLE       0xffff

typedef struct
{
    int handle;
    int posx;
    int posy;
    int width;
    int height;
}sal_cover_s;


/*
 函 数 名: sal_cover_create
 功能描述: SAL层创建视频遮挡区域

 输入参数: 遮挡区域的位置和大小
 输出参数:
 返 回 值: 设置成功返回0, 失败返回小于0
*/
int sal_cover_create(sal_cover_s *param);

/*
 函 数 名: sal_cover_destory
 功能描述: SAL层销毁视频遮挡区域

 输入参数:遮挡区域的位置和大小
 输出参数:
 返 回 值: 设置成功返回0, 失败返回小于0
*/
int sal_cover_destory(sal_cover_s *param);



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif


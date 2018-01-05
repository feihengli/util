
#ifndef __SAL_JPEG_H__
#define __SAL_JPEG_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif



/*
 函 数 名: sal_jpeg_cb
 功能描述: jpeg帧回调函数
 输入参数: 无
 输出参数: frame 视频帧
            len 帧大小
 返 回 值: 成功返回0,失败返回小于0
*/
typedef int (*sal_jpeg_cb)(char *frame, int len);


/*
 函 数 名: sal_jpeg_init
 功能描述: SAL层JPEG功能使能
 输入参数: cb 回调函数
 输出参数:
 返 回 值: 设置成功返回0, 失败返回小于0
*/
int sal_jpeg_init(sal_jpeg_cb cb);

/*
 函 数 名: sal_jpeg_exit
 功能描述: SAL层JPEG功能去初始化，释放资源
 输入参数: cb 回调函数
 输出参数:
 返 回 值: 设置成功返回0, 失败返回小于0
*/
int sal_jpeg_exit();





#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif


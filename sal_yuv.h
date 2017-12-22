
#ifndef __SAL_YUV_H__
#define __SAL_YUV_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif


/*****************************************************************************
 函 数 名: sal_yuv_get
 功能描述  : SAL层获取通道yuv sp420
 输入参数  : stream 通道id
            buffer 传入buffer，至少需要申请 width*height*1.5 大小的内存
            len     传入buffer的长度，
            若buffer小于所需大小，直接返回失败
 输出参数  :
 返 回 值: 成功返回yuv的实际大小, 失败返回小于0
*****************************************************************************/
int sal_yuv_get(int stream, char* buffer, int len);





#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif


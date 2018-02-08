#ifndef sal_mp4record_h__
#define sal_mp4record_h__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#include "sal_standard.h"

/*
 函 数 名: mp4EncoderInit
 功能描述: 初始化h264裸流mp4封装器
 输入参数: filename 文件名
            width 视频宽
            height 视频高
            fps 视频帧率
 输出参数: 无
 返 回 值: 成功返回封装器handle,失败返回NULL
*/
handle mp4EncoderInit(const char * filename,int width,int height, int fps);

/*
 函 数 名: mp4VEncoderWrite
 功能描述: 往mp4封装器写入视频帧
 输入参数: hndMp4 封装器handle
            frame 视频帧
            size 视频帧大小
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int mp4VEncoderWrite(handle hndMp4, unsigned char* frame ,int size);

/*
 函 数 名: mp4AEncoderWrite
 功能描述: 往mp4封装器写入音频帧
 输入参数: hndMp4 封装器handle
            frame 音频帧
            size 音频帧大小
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int mp4AEncoderWrite(handle hndMp4, unsigned char* frame ,int size);

/*
 函 数 名: mp4Encoderclose
 功能描述: 关闭mp4封装器
 输入参数: hndMp4 封装器handle
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int mp4Encoderclose(handle hndMp4);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif 

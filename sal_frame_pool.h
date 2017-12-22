#ifndef __FRAME_POOL_H__
#define __FRAME_POOL_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#include "sal_standard.h"

typedef enum FRAME_TYPE_E
{
    FRAME_TYPE_H264 = 0,
    FRAME_TYPE_G711A,
    FRAME_TYPE_G711U,
}FRAME_TYPE_E;

typedef struct frame_info_s
{
    FRAME_TYPE_E type;
    void* data;
    int len;
    double timestamp;
    int keyFrame;
    unsigned int sequence;

    int reference;
}frame_info_s;

extern handle gHndMainFramePool;  //主码流句柄
extern handle gHndSubFramePool;   //子码流句柄

/*****************************************************************************
 函 数 名: frame_pool_init
 功能描述  : 初始化音视频帧池模块
 输入参数  : capacity 最大缓存的帧数
 输出参数  : 无
 返 回 值: 成功返回句柄,失败返回NULL
*****************************************************************************/
handle frame_pool_init(int capacity);

/*****************************************************************************
 函 数 名: frame_pool_destroy
 功能描述  : 去初始化音视频帧池模块
 输入参数  : hndFramePool 音视频帧池句柄
 输出参数  : 无
 返 回 值: 成功返回0,失败返回小于0
*****************************************************************************/
int frame_pool_destroy(handle hndFramePool);

/*****************************************************************************
 函 数 名: frame_pool_add
 功能描述  : 往音视频帧池里增加帧
 输入参数  : hndFramePool 音视频帧池句柄
            frame 音视频帧
            len 音视频帧大小
            key 是否关键帧
            pts 音视频帧时间戳
 输出参数  : 无
 返 回 值: 成功返回0,失败返回小于0
*****************************************************************************/
int frame_pool_add(handle hndFramePool, char *frame, unsigned long len, FRAME_TYPE_E type, int key, double pts);

/*****************************************************************************
 函 数 名: frame_pool_register
 功能描述  : 注册音视频帧池的Reader，需等池里至少有一个I帧才会返回成功
 输入参数  : hndFramePool 音视频帧池句柄
            head 1：从帧池头部开始读取 0：从帧池尾部开始读取
 输出参数  : 无
 返 回 值: 成功返回句柄,失败返回NULL
*****************************************************************************/
handle frame_pool_register(handle hndFramePool, int head);

/*****************************************************************************
 函 数 名: frame_pool_unregister
 功能描述  : 注销音视频帧池的Reader
 输入参数  : hndReader 句柄
 输出参数  : 无
 返 回 值: 成功返回0,失败返回小于0
*****************************************************************************/
int frame_pool_unregister(handle hndReader);

/*****************************************************************************
 函 数 名: frame_pool_get
 功能描述  : 从帧池获取音视频帧
 输入参数  : hndReader 注册获得的句柄
 输出参数  : 无
 返 回 值: 成功返回frame_info_s结构,失败返回NULL
*****************************************************************************/
frame_info_s* frame_pool_get(handle hndReader);

/*****************************************************************************
 函 数 名: frame_pool_release
 功能描述  : 释放从帧池获取的音视频帧
 输入参数  : hndReader 注册获得的句柄
            pstFrameInfo 从帧池获取的音视频帧
 输出参数  : 无
 返 回 值: 成功返回0,失败返回小于0
*****************************************************************************/
int frame_pool_release(handle hndReader, frame_info_s* pstFrameInfo);

/*****************************************************************************
 函 数 名: frame_pool_sps_get
 功能描述  : 从帧池获取h264视频帧的SPS
 输入参数  : hndReader 注册获得的句柄
 输出参数  : out 输出SPS
            len 输出SPS的长度
 返 回 值: 成功返回0,失败返回小于0
*****************************************************************************/
int frame_pool_sps_get(handle hndReader, char** out, int* len);

/*****************************************************************************
 函 数 名: frame_pool_pps_get
 功能描述  : 从帧池获取h264视频帧的PPS
 输入参数  : hndReader 注册获得的句柄
 输出参数  : out 输出PPS
            len 输出PPS的长度
 返 回 值: 成功返回0,失败返回小于0
*****************************************************************************/
int frame_pool_pps_get(handle hndReader, char** out, int* len);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif


#ifndef __SAL_STATISTICS_H__
#define __SAL_STATISTICS_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif


/*
 函 数 名: sal_statistics_proc
 功能描述: 码率帧率统计，内部使用API
 输入参数: id 视频流id
            pts 时间戳
            len 帧长度
            keyFrame 是否关键帧
            gop I帧间隔
            smooth_frames 滑动几帧开始计算
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int sal_statistics_proc(int id, double pts, int len, int keyFrame, int gop, int smooth_frames);

/*
 函 数 名: sal_statistics_bitrate_get
 功能描述: 获取实时统计的码率
 输入参数: id 视频流id
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int sal_statistics_bitrate_get(int id);

/*
 函 数 名: sal_statistics_framerate_get
 功能描述: 获取实时统计的帧率
 输入参数: id 视频流id
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int sal_statistics_framerate_get(int id);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif


#endif




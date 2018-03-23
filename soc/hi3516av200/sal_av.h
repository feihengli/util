
#ifndef __SAL_AV_H__
#define __SAL_AV_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#include "sal_standard.h"

typedef enum SAL_BITRATE_CONTROL_E
{
    SAL_BITRATE_CONTROL_CBR = 0,
    SAL_BITRATE_CONTROL_VBR = 1,
    SAL_BITRATE_CONTROL_BUTT,
}SAL_BITRATE_CONTROL_E;

typedef enum SAL_ENCODE_TYPE_E
{
    SAL_ENCODE_TYPE_H264 = 0,
    SAL_ENCODE_TYPE_H265 = 1,
    SAL_ENCODE_TYPE_BUTT,
}SAL_ENCODE_TYPE_E;

typedef struct sal_stream_s
{
    int enable;
    int height;
    int width;
    int framerate;
    int bitrate;  //kbps
    int gop;
    SAL_BITRATE_CONTROL_E bitrate_ctl;
    SAL_ENCODE_TYPE_E encode_type;
}sal_stream_s;

typedef struct sal_video_qp_s
{
    int min_qp;
    int max_qp;
    int min_i_qp;
    int max_i_qp;
}sal_video_qp_s;

/*
 函 数 名: sal_video_frame_cb
 功能描述: 视频帧回调函数
 输入参数: 无
 输出参数: stream 流id
            pts 时间戳(us) 绝对时间
            frame 视频帧
            len 帧大小
            key 关键帧标志
 返 回 值: 成功返回0,失败返回小于0
*/
typedef int (*sal_video_frame_cb)(int stream, char *frame, unsigned long len, int key, double pts, SAL_ENCODE_TYPE_E encode_type);

typedef struct sal_video_s
{
    sal_stream_s stream[2];
    sal_video_frame_cb cb;
    int rotate; // 0-->none, 1-->90 degree rotation
    int scale_enable;
    int smartP_enable;
    int wdr_enable;
}sal_video_s;


/*
 函 数 名: sal_sys_init
 功能描述: 初始化dsp系统资源
 输入参数: video 视频配置
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int sal_sys_init(sal_video_s* video);


/*
 函 数 名: sal_sys_exit
 功能描述: dsp系统退出，释放资源
 输入参数: 无
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int sal_sys_exit(void);

/*
 函 数 名: sal_sys_version
 功能描述: 获取MPP版本信息，以及libipc.so的版本信息
 输入参数:    buffer 传入buffer
            len     传入buffer的长度，若buffer小于所需大小，返回失败
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int sal_sys_version(char* buffer, int len);

/*
 函 数 名: sal_video_force_idr
 功能描述: 强制I帧
 输入参数: stream 码流id
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int sal_video_force_idr(int stream);

 /*
 函 数 名: sal_video_mirror_set
 功能描述: 设置水平翻转
 输入参数: enable 使能标志
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int sal_video_mirror_set(int enable);

 /*
 函 数 名: sal_video_flip_set
 功能描述: 设置垂直翻转
 输入参数: enable 使能标志
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int sal_video_flip_set(int enable);

 /*
 函 数 名: sal_video_vi_framerate_set
 功能描述: 设置VI帧率
 输入参数: framerate 帧率
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int sal_video_vi_framerate_set(int framerate);

 /*
 函 数 名  : sal_video_framerate_set
 功能描述: 设置编码帧率
 输入参数: stream 码流id
            framerate VENC帧率
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int sal_video_framerate_set(int stream, int framerate);

/*
 函 数 名: sal_video_bitrate_set
 功能描述: 设置码率
 输入参数: stream 码流id
            bitrate_ctl 定码率或变码率
            bitrate 码率 kbps
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int sal_video_bitrate_set(int stream, SAL_BITRATE_CONTROL_E bitrate_ctl, int bitrate);

 /*
 函 数 名: sal_video_gop_set
 功能描述: 设置GOP
 输入参数: stream 码流id
            gop I帧间隔 一般设为帧率的整数倍
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int sal_video_gop_set(int stream, int gop);

/*
 函 数 名: sal_video_args_get
 功能描述: 获取视频流信息
 输入参数: stream 码流id
 输出参数: video 视频流信息
 返 回 值: 成功返回0,失败返回小于0
*/
int sal_video_args_get(int stream, sal_stream_s* video);

 /*
 函 数 名: sal_video_cbr_qp_set
 功能描述: 设置CBR模式下的QP
 输入参数: stream 码流id
            qp_args QP相关参数
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int sal_video_cbr_qp_set(int stream, sal_video_qp_s* qp_args);

 /*
 函 数 名: sal_video_cbr_qp_get
 功能描述: 获取CBR模式下的QP
 输入参数: stream 码流id
 输出参数: qp_args QP相关参数
 返 回 值: 成功返回0,失败返回小于0
*/
int sal_video_cbr_qp_get(int stream, sal_video_qp_s* qp_args);

/*
 函 数 名: sal_video_lbr_set
 功能描述: 设置码率 供内部lbr模块使用，以辨别用户的CBR和LBR使用的CBR
 输入参数: stream 码流id
            bitrate_ctl 定码率或变码率
            bitrate 码率 kbps
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int sal_video_lbr_set(int stream, SAL_BITRATE_CONTROL_E bitrate_ctl, int bitrate);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif


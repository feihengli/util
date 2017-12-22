
#ifndef __SAL_AUDIO_H__
#define __SAL_AUDIO_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#include "sal_standard.h"

/*****************************************************************************
 函 数 名: sal_audio_cb
 功能描述  : 音频帧回调函数
 输入参数  : 无
 输出参数  : frame 视频帧
            len 帧大小
            pts 时间戳(us) 绝对时间
 返 回 值: 成功返回0,失败返回小于0
*****************************************************************************/
typedef int (*sal_audio_cb)(char *frame, unsigned long len, double pts);


typedef struct sal_audio_s
{
    int enable;                 //开启音频
    char encType[32];           //G.711U G.711A LPCM
    int channels;               //only support single channel(18ev200)
    int bitWidth;               // 16 bit
    int volume;                 //range [0,100]
    int sampleRate;             //8k
    int ptNumPerFrm;            //每一帧的采样点，如果是8k采样率，目标25帧的话，此值应配置为320
    sal_audio_cb cb;
}sal_audio_s;


/*****************************************************************************
 函 数 名: sal_audio_init
 功能描述  : SAL层启用音频
 输入参数  : param 输入音频基本参数
 输出参数  :
 返 回 值: 设置成功返回0, 失败返回小于0
*****************************************************************************/
int sal_audio_init(sal_audio_s *audio);

/*****************************************************************************
 函 数 名: sal_audio_exit
 功能描述  : SAL层禁用音频

 输入参数  :
 输出参数  :
 返 回 值: 设置成功返回0, 失败返回小于0
*****************************************************************************/
int sal_audio_exit(void);

/*****************************************************************************
 函 数 名: sal_audio_play
 功能描述  : SAL层播放音频帧，如果播放的是LPCM数据，输入长度应该和采集的一致

 输入参数  :
 输出参数  :
 返 回 值: 设置成功返回0, 失败返回小于0
*****************************************************************************/
int sal_audio_play(char *frame, int frame_len);

/*****************************************************************************
 函 数 名: sal_audio_volume_set
 功能描述  : SAL层设置采集音量大小

 输入参数  : vol 范围 [0,100]
 输出参数  :
 返 回 值: 设置成功返回0, 失败返回小于0
*****************************************************************************/
int sal_audio_capture_volume_set(int vol);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif


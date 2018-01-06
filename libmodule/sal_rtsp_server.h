#ifndef __RTSP_SERVER_H__
#define __RTSP_SERVER_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#include "sal_standard.h"

typedef struct client_s // 客户端链表的节点数据
{
    int fd;
    int running;
    pthread_t pid;
}client_s;

/*
 函 数 名: rtsps_init
 功能描述: 初始化RTSP服务器模块，使用VLC播放时，
            由于此RTSP模块只支持TCP的传输方式，需要设置VLC的RTP OVER TCP选项
            访问主子码流URL例子：
            rtsp://192.168.0.110:554/MainStream
            rtsp://192.168.0.110:554/SubStream
 输入参数: port 默认输入554
 输出参数: 无
 返 回 值: 成功返回句柄,失败返回NULL
*/
handle rtsps_init(int port);

/*
 函 数 名: rtsps_destroy
 功能描述: 去初始化RTSP服务器模块，释放所有资源
 输入参数: hndRtsps RTSP服务器句柄
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int rtsps_destroy(handle hndRtsps);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif


#endif




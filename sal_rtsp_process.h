#ifndef __RTSP_PROCESS_H__
#define __RTSP_PROCESS_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif


#include "sal_standard.h"

/*
 函 数 名: rtsp_process
 功能描述: 线程入口，RTSP服务器的子模块，只为一个客户端服务
 输入参数: pclient RTSP服务器模块的client_s结构体指针
 输出参数: 无
 返 回 值: 无
*/
void* rtsp_process(void* pclient);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif


#endif




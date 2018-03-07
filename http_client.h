#ifndef http_client_h__
#define http_client_h__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#include "sal_standard.h"

//socket 层运行线程的情况
typedef enum
{
    HTTP_STATUS_INVALID = 0,
    HTTP_STATUS_PROGRESS,      //正在进行中
    HTTP_STATUS_FINISH_OK,     //完成ok
    HTTP_STATUS_FINISH_ERROR,  //完成error
    HTTP_STATUS_WRITE_TIMEOUT, //写超时
    HTTP_STATUS_READ_TIMEOUT,  //读超时
    HTTP_STATUS_UNKNOWN,
}HTTP_STATUS_E;

/*
 函 数 名: http_client_init
 功能描述: 初始化http客户端模块
 输入参数: szUrl 需要访问的http url(eg. http://192.168.0.32:8186/uImage)
            timeout 超时时间（ms）
 输出参数: 无
 返 回 值: 成功返回句柄,失败返回NULL
*/
handle http_client_init(char* szUrl, int timeout);

/*
 函 数 名: http_client_exit
 功能描述: 去初始化http客户端模块
 输入参数: hndHttpClient 句柄
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int http_client_exit(handle hndHttpClient);

/*
 函 数 名: http_client_Query
 功能描述: 查询http状态
 输入参数: hndHttpClient 句柄
 输出参数: status 查询状态
 返 回 值: 成功返回0,失败返回小于0
*/
int http_client_Query(handle hndHttpClient, HTTP_STATUS_E* status);

/*
 函 数 名: http_client_HeaderGet
 功能描述: 获取http的头部
 输入参数: hndHttpClient 句柄
 输出参数: output 输出buffer
            len buffer长度
 返 回 值: 成功返回0,失败返回小于0
*/
int http_client_HeaderGet(handle hndHttpClient, unsigned char** output, int* len);

/*
 函 数 名: http_client_ContentGet
 功能描述: 获取http的内容
 输入参数: hndHttpClient 句柄
 输出参数: output 输出buffer
            len buffer长度
 返 回 值: 成功返回0,失败返回小于0
*/
int http_client_ContentGet(handle hndHttpClient, unsigned char** output, int* len);

/*
 函 数 名: http_client_ProgressGet
 功能描述: 获取http的进度
 输入参数: hndHttpClient 句柄
 输出参数: progress 进度（0-100）
 返 回 值: 成功返回0,失败返回小于0
*/
int http_client_ProgressGet(handle hndHttpClient, int* progress);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif


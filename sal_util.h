#ifndef __SAL_UTIL_H__
#define __SAL_UTIL_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#include "sal_standard.h"

/*
 函 数 名: util_time_abs
 功能描述: 获取系统绝对时间
 输入参数: 无
 输出参数: pTime 系统绝对时间
 返 回 值: 成功返回0,失败返回小于0
*/
int util_time_abs(struct timeval* pTime);

/*
 函 数 名: util_time_local
 功能描述: 获取系统时钟时间
 输入参数: 无
 输出参数: pTime 系统绝对时间
 返 回 值: 成功返回0,失败返回小于0
*/
int util_time_local(struct timeval* pTime);

/*
 函 数 名: util_time_sub
 功能描述: 获取时间差（单位毫秒）
 输入参数: pStart 开始时间
            pEnd 结束时间
 输出参数: 无
 返 回 值: 成功返回时间差(ms),失败返回小于0
*/
int util_time_sub(struct timeval* pStart, struct timeval* pEnd);

/*
 函 数 名: util_time_sub
 功能描述: 获取之前的时间与当前的时间差（单位毫秒）
 输入参数: previous 之前的时间（开始时间）
 输出参数: 无
 返 回 值: 成功返回时间差(ms),失败返回小于0
*/
int util_time_pass(struct timeval* previous);

/*
 函 数 名: util_file_size
 功能描述: 获取文件大小
 输入参数: path 文件路径
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int util_file_size(char* path);

/*
 函 数 名: util_file_read
 功能描述: 读取文件中的内容
 输入参数: path 文件路径
            len 缓存的最大长度
 输出参数: buf 输出缓存
 返 回 值: 成功返回已读取大小,失败返回小于0
*/
int util_file_read(const char* path, unsigned char* buf, int len);

/*
 函 数 名: util_file_write
 功能描述: 把一段内存写到文件中
 输入参数: path 文件路径
            buf buffer内存
            len 缓存的最大长度
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int util_file_write(const char* path, unsigned char* buf, int len);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif

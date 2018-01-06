#ifndef __CACHE_H__
#define __CACHE_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#include "sal_standard.h"

/*
 函 数 名: cache_init
 功能描述: 初始化缓存模块，需与cache_destroy配对使用
 输入参数: size 配置的缓存大小
 输出参数: 无
 返 回 值: 成功返回缓存句柄,失败返回NULL
*/
handle cache_init(int size);

/*
 函 数 名: cache_init
 功能描述: 去初始化缓存模块，需与cache_init配对使用
 输入参数: hndcache 缓存句柄
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int cache_destroy(handle hndcache);

/*
 函 数 名: cache_writable
 功能描述: 获取缓存可写的起始地址以及可写长度
 输入参数: hndcache 缓存句柄
 输出参数: begin 可写的起始地址
            len 可写长度
 返 回 值: 成功返回0,失败返回小于0
*/
int cache_writable(handle hndcache, void** begin, int* len);

/*
 函 数 名: cache_readable
 功能描述: 获取缓存可读的起始地址以及可读长度
 输入参数: hndcache 缓存句柄
 输出参数: begin 可读的起始地址
            len 可读长度
 返 回 值: 成功返回0,失败返回小于0
*/
int cache_readable(handle hndcache, void** begin, int* len);

/*
 函 数 名: cache_offset_writable
 功能描述: 当对缓存进行写操作后需调用此接口告诉缓存模块对可写地址进行偏移
 输入参数: hndcache 缓存句柄
            length 已写长度
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int cache_offset_writable(handle hndcache, int length);

/*
 函 数 名: cache_offset_readable
 功能描述: 当对缓存进行读操作后需调用此接口告诉缓存模块对可读地址进行偏移
 输入参数: hndcache 缓存句柄
            length 已读长度
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int cache_offset_readable(handle hndcache, int length);

/*
 函 数 名: cache_clean
 功能描述: 当对缓存进行多次读写操作后，需调用此接口清理已读取的内容
 输入参数: hndcache 缓存句柄
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int cache_clean(handle hndcache);

/*
 函 数 名: cache_reset
 功能描述: 重置缓存的读写位置（从0开始）
 输入参数: hndcache 缓存句柄
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int cache_reset(handle hndcache);

/*
 函 数 名: cache_debug
 功能描述: 是否使能缓存模块内的打印信息。默认为false
 输入参数: hndcache 缓存句柄
            enable 使能标志
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int cache_debug(handle hndcache, int enable);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif



#ifndef __MALLOC_H__
#define __MALLOC_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#include "sal_standard.h"

//当HEAP_TEST为0时，关闭内存检测
//当HEAP_TEST为1时，开启内存检测
//软件正式发布不应该开启内存检测
// mem_malloc/mem_realloc需与mem_free配对使用
#define HEAP_TEST 0

#if HEAP_TEST
#define mem_malloc(size) mem_malloc1(size, __FUNCTION__, __LINE__)
#define mem_realloc(original, size) mem_realloc1(original, size, __FUNCTION__, __LINE__)
#define mem_free(ptr) mem_free1(ptr)
#else
#define mem_malloc(size) malloc(size)
#define mem_realloc(original, size) realloc(original, size)
#define mem_free(ptr) free(ptr)
#endif

/*
 函 数 名: mem_malloc1
 功能描述: 申请内存，只供mem_malloc宏调用。内部使用API
 输入参数: bytes 申请内存大小
            func 函数名字
            line 行号
 输出参数: 无
 返 回 值: 成功返回内存指针,失败返回NULL
*/
void* mem_malloc1(int bytes, const char* func, int line);

/*
 函 数 名: mem_realloc1
 功能描述: 申请内存，只供mem_realloc宏调用。内部使用API
 输入参数:  original 原来的堆内存指针
            new_bytes 重新申请内存大小
            func 函数名字
            line 行号
 输出参数: 无
 返 回 值: 成功返回内存指针,失败返回NULL
*/
void* mem_realloc1(void* original, int new_bytes, const char* func, int line);

/*
 函 数 名: mem_free1
 功能描述: 释放内存，只供mem_free宏调用。内部使用API
 输入参数:  ptr 堆内存指针
 输出参数: 无
 返 回 值: 无
*/
void mem_free1(void* ptr);

/*
 函 数 名: mem_get_malloc_sum
 功能描述: 获取已对堆内存申请的总大小
 输入参数: 无
 输出参数: 无
 返 回 值: 成功返回堆内存申请的总大小,失败返回小于0
*/
int mem_get_malloc_sum();

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif


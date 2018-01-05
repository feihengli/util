#ifndef __MALLOC_H__
#define __MALLOC_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#include "sal_standard.h"

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

void* mem_malloc1(int bytes, const char* func, int line);
void* mem_realloc1(void* original, int new_bytes, const char* func, int line);
void mem_free1(void* ptr);
int mem_get_malloc_sum();

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif


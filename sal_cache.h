#ifndef __CACHE_H__
#define __CACHE_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#include "sal_standard.h"

handle cache_init(int size);
int cache_destroy(handle hndcache);
int cache_writable(handle hndcache, void** begin, int* len);
int cache_readable(handle hndcache, void** begin, int* len);
int cache_offset_writable(handle hndcache, int length);
int cache_offset_readable(handle hndcache, int length);
int cache_clean(handle hndcache);
int cache_reset(handle hndcache);
int cache_debug(handle hndcache, int enable);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif



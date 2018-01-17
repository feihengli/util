
#include "sal_cache.h"
#include "sal_malloc.h"
#include "sal_debug.h"


typedef struct cache_s
{
    int readable;
    int writable;
    int size;
    void* begin;
    int bDebug;
}cache_s;

handle cache_init(int size)
{
    CHECK(size > 0, NULL, "invalid parameter with: %#x\n", size);

    cache_s* cache = (cache_s*)mem_malloc(sizeof(cache_s) + size);
    CHECK(cache, NULL, "error with %#x\n", cache);
    memset(cache, 0, sizeof(cache_s) + size);

    cache->size = size;
    cache->readable = 0;
    cache->writable = 0;
    cache->begin = (void*)(cache+1);

    return cache;
}

int cache_destroy(handle hndcache)
{
    CHECK(hndcache, -1, "invalid parameter with: %#x\n", hndcache);

    mem_free(hndcache);
    DBG("completed.\n");
    return 0;
}

int cache_writable(handle hndcache, void** begin, int* len)
{
    CHECK(hndcache, -1, "invalid parameter with: %#x\n", hndcache);
    CHECK(begin, -1, "invalid parameter with: %#x\n", begin);
    CHECK(len, -1, "invalid parameter with: %#x\n", len);

    cache_s* cache = (cache_s*)hndcache;
    *begin = cache->begin + cache->writable;
    *len = cache->size - cache->writable;

    if (cache->bDebug && cache->writable%4 != 0)
    {
        WRN("addr is not multiple of 4\n");
    }
    return 0;
}

int cache_readable(handle hndcache, void** begin, int* len)
{
    CHECK(hndcache, -1, "invalid parameter with: %#x\n", hndcache);
    CHECK(begin, -1, "invalid parameter with: %#x\n", begin);
    CHECK(len, -1, "invalid parameter with: %#x\n", len);

    cache_s* cache = (cache_s*)hndcache;
    *begin = cache->begin + cache->readable;
    *len = cache->writable - cache->readable;

    if (cache->bDebug && cache->readable%4 != 0)
    {
        WRN("addr is not multiple of 4\n");
    }
    return 0;
}

int cache_offset_writable(handle hndcache, int length)
{
    CHECK(hndcache, -1, "invalid parameter with: %#x\n", hndcache);

    cache_s* cache = (cache_s*)hndcache;
    CHECK(length <= cache->size - cache->writable, -1, "invalid parameter with: %#x\n", length);
    cache->writable += length;

    if (cache->bDebug && cache->writable%4 != 0)
    {
        WRN("addr is not multiple of 4\n");
    }
    return 0;
}

int cache_offset_readable(handle hndcache, int length)
{
    CHECK(hndcache, -1, "invalid parameter with: %#x\n", hndcache);

    cache_s* cache = (cache_s*)hndcache;
    CHECK(length <= cache->writable - cache->readable, -1, "invalid parameter with: %#x\n", length);
    cache->readable += length;

    if (cache->bDebug && cache->readable%4 != 0)
    {
        WRN("addr is not multiple of 4\n");
    }
    return 0;
}

int cache_clean(handle hndcache)
{
    CHECK(hndcache, -1, "invalid parameter with: %#x\n", hndcache);

    cache_s* cache = (cache_s*)hndcache;
    memmove(cache->begin, cache->begin+cache->readable, cache->writable-cache->readable);
    cache->writable -= cache->readable;
    cache->readable = 0;

    if (cache->bDebug && cache->readable%4 != 0)
    {
        WRN("addr is not multiple of 4\n");
    }
    return 0;
}

int cache_reset(handle hndcache)
{
    CHECK(hndcache, -1, "invalid parameter with: %#x\n", hndcache);

    cache_s* cache = (cache_s*)hndcache;
    cache->readable = 0;
    cache->writable = 0;
    memset(cache->begin, 0, cache->size);
    return 0;
}

int cache_debug(handle hndcache, int enable)
{
    CHECK(hndcache, -1, "invalid parameter with: %#x\n", hndcache);

    cache_s* cache = (cache_s*)hndcache;
    cache->bDebug = enable;

    return 0;
}


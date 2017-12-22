#include "sal_malloc.h"
#include "sal_debug.h"

static unsigned int malloc_sum = 0;

typedef struct mem_s
{
    void* this;
    const char* func;
    int line;
    int size;
}mem_s;

static int mem_align(int bytes)
{
    int ret = bytes;
    if (bytes % 4 != 0)
    {
        ret = (bytes/4 + 1) * 4;
    }
    return ret;
}

static int mem_touch_file(mem_s* pMem)
{
    int ret = -1;
    char path[64];

    ret = snprintf(path, sizeof(path), "/tmp/heap_%p_%s_%d_%d", pMem->this, pMem->func, pMem->line, pMem->size);
    CHECK(ret > 0, -1, "error with %#x: %s\n", ret, strerror(errno));

    int fd = creat(path, S_IWRITE|S_IREAD|S_IEXEC);
    CHECK(fd > 0, -1, "error with %#x: %s\n", ret, strerror(errno));
    close(fd);

    return 0;
}

static int mem_rm_file(mem_s* pMem)
{
    int ret = -1;
    char path[64];

    ret = snprintf(path, sizeof(path), "/tmp/heap_%p_%s_%d_%d", pMem->this, pMem->func, pMem->line, pMem->size);
    CHECK(ret > 0, -1, "error with %#x: %s\n", ret, strerror(errno));

    ret = remove(path);
    CHECK(ret == 0, -1, "error with %#x: %s\n", ret, strerror(errno));

    return 0;
}
void* mem_malloc1(int bytes, const char* func, int line)
{
    if (bytes >= 0)
    {
        bytes += sizeof(mem_s);
        int AlignSize = mem_align(bytes);
        mem_s* ret = (mem_s*)malloc(AlignSize);
        if (NULL != ret)
        {
            ret->this = ret;
            ret->func = func;
            ret->line = line;
            ret->size = AlignSize;
            mem_touch_file(ret);
            malloc_sum += ret->size;
            ret += 1;
            //DBG("malloc %d ok, total malloc %d\n", AlignSize, malloc_sum);
            return (void*)ret;
        }
    }
    return NULL;
}

void* mem_realloc1(void* original, int new_bytes, const char* func, int line)
{
    if (original && new_bytes > 0)
    {
        mem_s* tmp = (mem_s*)original;
        tmp -= 1;
        if (new_bytes == tmp->size)
        {
            DBG("original size[%d] is equal new_bytes[%d]\n", tmp->size, new_bytes);
            return original;
        }

        void* new_addr = mem_malloc1(new_bytes, func, line);
        if (new_addr)
        {
            int need_copy = (new_bytes > tmp->size) ? tmp->size : new_bytes;
            memcpy(new_addr, original, need_copy);
            mem_free1(original);
            return new_addr;
        }
    }

    return NULL;
}

void mem_free1(void* ptr)
{
    if (ptr)
    {
        mem_s* tmp = (mem_s*)ptr;
        tmp -= 1;
        mem_rm_file(tmp);
        malloc_sum -= tmp->size;
        //DBG("free %d ok, total malloc %d\n", *tmp, malloc_sum);
        free(tmp);
    }
}

int mem_get_malloc_sum()
{
    return malloc_sum;
}


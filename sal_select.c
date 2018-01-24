
#include "sal_debug.h"
#include "sal_list.h"
#include "sal_malloc.h"
#include "sal_util.h"
#include "sal_select.h"
#include "sal_cache.h"

typedef struct data_pack_s
{
    int sentsize;
    int size;
    char* begin;
}data_pack_s;

typedef struct select_s
{
    int fd;
    struct timeval last_rtime;
    struct timeval last_wtime;
    handle hndwlist;
    handle hndrlist;
    handle hndcache;
    completeness_cb complete_cb;
    void* cb_param;
    unsigned int total_sentsize;
    unsigned int total_readsize;

    int bDebug;
}select_s;

static int select_nonblock_set(int fd)
{
    int flag = fcntl(fd, F_GETFL);
    CHECK(flag != -1, -1, "failed to get fcntl flag: %s\n", strerror(errno));
    CHECK(fcntl(fd, F_SETFL, flag | O_NONBLOCK) != -1, -1, "failed to set nonblock: %s\n", strerror(errno));
    DBG("setnoblock success.fd: %d\n", fd);

    return 0;
}

handle select_init(completeness_cb complete, void* cb_param, int cache_size, int fd)
{
    CHECK(complete, NULL, "invalid parameter with: %#x\n", complete);
    CHECK(cache_size > 0, NULL, "invalid parameter with: %#x\n", cache_size);
    CHECK(fd > 0, NULL, "invalid parameter with: %#x\n", fd);

    int ret = -1;
    select_s* pstSelect = (select_s*)mem_malloc(sizeof(select_s));
    CHECK(pstSelect, NULL, "failed to malloc %d\n", sizeof(select_s));
    memset(pstSelect, 0, sizeof(select_s));

    util_time_abs(&pstSelect->last_rtime);
    util_time_abs(&pstSelect->last_wtime);

    pstSelect->hndwlist = list_init(sizeof(data_pack_s));
    CHECK(pstSelect->hndwlist, NULL, "error with %#x\n", pstSelect->hndwlist);

    pstSelect->hndrlist = list_init(sizeof(data_pack_s));
    CHECK(pstSelect->hndrlist, NULL, "error with %#x\n", pstSelect->hndrlist);

    pstSelect->hndcache = cache_init(cache_size);
    CHECK(pstSelect->hndcache, NULL, "error with %#x\n", pstSelect->hndcache);

    ret = cache_debug(pstSelect->hndcache, 0);
    CHECK(ret == 0, NULL, "error with %#x\n", ret);

    pstSelect->complete_cb = complete;
    pstSelect->cb_param = cb_param;

    pstSelect->fd = fd;
    ret = select_nonblock_set(fd);
    CHECK(!ret, NULL, "error with %#x\n", ret);

    return (handle)pstSelect;
}

int select_destroy(handle hndselect)
{
    CHECK(hndselect, -1, "invalid parameter with: %#x\n", hndselect);

    int ret = -1;
    select_s* pstSelect = (select_s*)(hndselect);

    //WRN("pstSelect->hndcache: %#x\n", pstSelect->hndcache);
    ret = cache_destroy(pstSelect->hndcache);
    CHECK(!ret, -1, "error with %#x\n", ret);
    pstSelect->hndcache = NULL;

    data_pack_s* pDataPack = list_front(pstSelect->hndwlist);
    while (pDataPack)
    {
        mem_free(pDataPack->begin);
        pDataPack = list_next(pstSelect->hndwlist, pDataPack);
    }

    ret = list_destroy(pstSelect->hndwlist);
    CHECK(!ret, -1, "error with %#x\n", ret);
    pstSelect->hndwlist = NULL;

    pDataPack = list_front(pstSelect->hndrlist);
    while (pDataPack)
    {
        mem_free(pDataPack->begin);
        pDataPack = list_next(pstSelect->hndrlist, pDataPack);
    }

    ret = list_destroy(pstSelect->hndrlist);
    CHECK(!ret, -1, "error with %#x\n", ret);
    pstSelect->hndrlist = NULL;

    mem_free(hndselect);
    DBG("completed.\n");
    return 0;
}

static int select_read(select_s* pstSelect)
{
    void* begin = NULL;
    int len = 0;

    int ret = cache_writable(pstSelect->hndcache, &begin, &len);
    CHECK(!ret, -1, "error with %#x\n", ret);

    if (len < 1024)
    {
        ret = cache_clean(pstSelect->hndcache);
        CHECK(!ret, -1, "error with %#x\n", ret);
        //WRN("cache_clean done.\n");

        //cache_clean后重新获取可写地址和长度
        ret = cache_writable(pstSelect->hndcache, &begin, &len);
        CHECK(!ret, -1, "error with %#x\n", ret);
    }

    CHECK(begin, -1, "error with %#x\n", begin);
    CHECK(len > 0, -1, "error with %#x\n", len);

    int recv_len = read(pstSelect->fd, begin, len);
    if (recv_len < 0)
    {
        if (errno != EINTR && errno != EAGAIN)
        {
            ERR("read failed.ret: %d: %s\n", recv_len, strerror(errno));
            return -1;
        }
        return 0;
    }
    else if (recv_len == 0)
    {
        WRN("remote closed. ret: %d\n", recv_len);
        return -1;
    }

    if (pstSelect->bDebug)
    {
        DBG("fd[%d] need to read %d, actually read %d\n", pstSelect->fd, len, recv_len);
    }

    pstSelect->total_readsize += recv_len;
    util_time_abs(&pstSelect->last_rtime);
    ret = cache_offset_writable(pstSelect->hndcache, recv_len);
    CHECK(!ret, -1, "error with %#x\n", ret);

    while (pstSelect->complete_cb)
    {
        begin = NULL;
        len = 0;
        ret = cache_readable(pstSelect->hndcache, &begin, &len);
        CHECK(!ret, -1, "error with %#x\n", ret);

        if (begin && len > 0)
        {
            int s32CompleteLen = 0;
            ret = pstSelect->complete_cb(begin, len, &s32CompleteLen, pstSelect->cb_param);
            CHECK(ret == 0, -1, "error with %#x\n", ret);

            if (s32CompleteLen > 1)
            {
                data_pack_s stDataPack;
                memset(&stDataPack, 0, sizeof(stDataPack));
                stDataPack.size = s32CompleteLen;
                stDataPack.begin = mem_malloc(s32CompleteLen);
                CHECK(stDataPack.begin, -1, "error with %#x\n", stDataPack.begin);

                memcpy(stDataPack.begin, begin, s32CompleteLen);
                ret = list_push_back(pstSelect->hndrlist, &stDataPack, sizeof(data_pack_s));
                CHECK(!ret, -1, "error with %#x\n", ret);

                ret = cache_offset_readable(pstSelect->hndcache, s32CompleteLen);
                CHECK(!ret, -1, "error with %#x\n", ret);

                continue; // 继续检查，可能接收到多个完整的数据包
            }
            else if (1 == s32CompleteLen) // 数据校验出错，丢弃一个字节继续校验
            {
                ret = cache_offset_readable(pstSelect->hndcache, 1);
                CHECK(!ret, -1, "error with %#x\n", ret);
                continue; // continue check
            }
        }
        break;
    }

    return 0;
}

static int select_write(select_s* pstSelect)
{
    int ret = -1;

    data_pack_s* pDataPack = list_front(pstSelect->hndwlist);
    while (pDataPack)
    {
        void* begin = (pDataPack->begin + pDataPack->sentsize);
        if (pstSelect->bDebug && ((unsigned int)begin)%4 != 0)
        {
            WRN("addr is not multiple of 4\n");
        }
        int len = (pDataPack->size - pDataPack->sentsize);

        CHECK(begin, -1, "error with %#x\n", begin);
        CHECK(len > 0, -1, "error with %#x\n", len);

        int write_len = write(pstSelect->fd, begin, len);
        if (write_len < 0)
        {
            if (errno != EINTR && errno != EAGAIN)
            {
                ERR("write failed.ret: %d: %s\n", write_len, strerror(errno));
                return -1;
            }
            return 0;
        }
        else if (write_len == 0)
        {
            WRN("remote closed. ret: %d\n", write_len);
            return -1;
        }

        if (pstSelect->bDebug)
        {
            DBG("fd[%d] need to write %d, actually write %d\n", pstSelect->fd, len, write_len);
        }
        pstSelect->total_sentsize += write_len;
        util_time_abs(&pstSelect->last_wtime);
        pDataPack->sentsize += write_len;
        if (pDataPack->sentsize == pDataPack->size)
        {
            mem_free(pDataPack->begin);
            pDataPack = list_next(pstSelect->hndwlist, pDataPack);

            ret = list_pop_front(pstSelect->hndwlist);
            CHECK(!ret, -1, "error with %#x\n", ret);
        }
        else
        {
            //一次发不完需等下次，不再循环
            break;
        }
    }

    return 0;
}

int select_rw(handle hndselect)
{
    CHECK(hndselect, -1, "invalid parameter with: %#x\n", hndselect);
    select_s* pstSelect = (select_s*)hndselect;

    int ret = -1;

    ret = select_write(pstSelect);
    CHECK(ret == 0, -1, "error with %#x\n", ret);

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(pstSelect->fd, &read_fds);
    struct timeval TimeoutVal = {0, 1};
    ret = select(pstSelect->fd + 1, &read_fds, NULL, NULL, &TimeoutVal);
    if (ret < 0)
    {
        if (errno != EINTR && errno != EAGAIN)
        {
            ERR("select failed: %s\n", strerror(errno));
            return -1;
        }
        return 0;
    }
    else if (ret == 0)
    {
        //DBG("time out.\n");
        return 0;
    }

    if (FD_ISSET(pstSelect->fd, &read_fds))
    {
        ret = select_read(pstSelect);
        CHECK(ret == 0, -1, "error with %#x\n", ret);
    }

    return 0;
}

int select_wtimeout(handle hndselect)
{
    CHECK(hndselect, -1, "invalid parameter with: %#x\n", hndselect);

    select_s* pstSelect = (select_s*)hndselect;
    return util_time_pass(&pstSelect->last_wtime);
}

int select_rtimeout(handle hndselect)
{
    CHECK(hndselect, -1, "invalid parameter with: %#x\n", hndselect);

    select_s* pstSelect = (select_s*)hndselect;
    return util_time_pass(&pstSelect->last_rtime);
}

int select_send(handle hndselect, void* pData, int DataLen)
{
    CHECK(hndselect, -1, "invalid parameter with: %#x\n", hndselect);
    CHECK(pData, -1, "invalid parameter with: %#x\n", pData);
    CHECK(DataLen > 0, -1, "invalid parameter with: %#x\n", DataLen);

    int ret = -1;
    select_s* pstSelect = (select_s*)hndselect;
    data_pack_s stDataPack;
    memset(&stDataPack, 0, sizeof(stDataPack));
    stDataPack.size = DataLen;
    stDataPack.begin = mem_malloc(DataLen);
    CHECK(stDataPack.begin, -1, "error with %#x\n", stDataPack.begin);

    memcpy(stDataPack.begin, pData, DataLen);
    ret = list_push_back(pstSelect->hndwlist, &stDataPack, sizeof(data_pack_s));
    CHECK(!ret, -1, "error with %#x\n", ret);

    return 0;
}

int select_recv_get(handle hndselect, void** data, int* len)
{
    CHECK(hndselect, -1, "invalid parameter with: %#x\n", hndselect);
    CHECK(data, -1, "invalid parameter with: %#x\n", data);
    CHECK(len, -1, "invalid parameter with: %#x\n", len);

    //int ret = -1;
    select_s* pstSelect = (select_s*)hndselect;
    data_pack_s* dp = (data_pack_s*)list_front(pstSelect->hndrlist);
    if (dp)
    {
        *len = dp->size;
        *data = dp->begin;
    }

    return 0;
}

int select_recv_release(handle hndselect, void** data, int* len)
{
    CHECK(hndselect, -1, "invalid parameter with: %#x\n", hndselect);
    CHECK(data, -1, "invalid parameter with: %#x\n", data);
    CHECK(len, -1, "invalid parameter with: %#x\n", len);

    int ret = -1;
    select_s* pstSelect = (select_s*)hndselect;
    data_pack_s* dp = (data_pack_s*)list_front(pstSelect->hndrlist);
    if (dp)
    {
        mem_free(dp->begin);
        ret = list_pop_front(pstSelect->hndrlist);
        CHECK(ret == 0, -1, "error with %#x\n", ret);
    }

    return 0;
}

int select_sentsize(handle hndselect)
{
    CHECK(hndselect, -1, "invalid parameter with: %#x\n", hndselect);
    select_s* pstSelect = (select_s*)hndselect;

    return pstSelect->total_sentsize;
}

int select_readsize(handle hndselect)
{
    CHECK(hndselect, -1, "invalid parameter with: %#x\n", hndselect);
    select_s* pstSelect = (select_s*)hndselect;

    return pstSelect->total_readsize;
}

int select_debug(handle hndselect, int enable)
{
    CHECK(hndselect, -1, "invalid parameter with: %#x\n", hndselect);
    select_s* pstSelect = (select_s*)hndselect;
    pstSelect->bDebug = enable;

    return 0;
}

int select_wlist_empty(handle hndselect)
{
    CHECK(hndselect, -1, "invalid parameter with: %#x\n", hndselect);
    select_s* pstSelect = (select_s*)hndselect;

    return list_empty(pstSelect->hndwlist);
}

int select_rlist_empty(handle hndselect)
{
    CHECK(hndselect, -1, "invalid parameter with: %#x\n", hndselect);
    select_s* pstSelect = (select_s*)hndselect;

    return list_empty(pstSelect->hndrlist);
}




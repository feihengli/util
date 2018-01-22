
#include "sal_debug.h"
#include "sal_list.h"
#include "sal_malloc.h"
#include "sal_frame_pool.h"

typedef struct frame_pool_s
{
    handle hndlist;
    int capacity;
    unsigned int video_sequence;
    unsigned int audio_sequence;
    pthread_mutex_t mutex;
}frame_pool_s;

typedef struct reader_s
{
    frame_pool_s* pstFramePool;
    int head; //1：从队头开始取 0：从队尾开始取
    unsigned char sps[64];
    int sps_size;
    unsigned char pps[64];
    int pps_size;
    unsigned char vps[64];
    int vps_size;
    frame_info_s* pstLastVKeyFrame;
    
    FRAME_TYPE_E vframe_type;
    FRAME_TYPE_E aframe_type;
}reader_s;

handle gHndMainFramePool = NULL;
handle gHndSubFramePool = NULL;

handle frame_pool_init(int capacity)
{
    frame_pool_s* pstFramePool = (frame_pool_s*)mem_malloc(sizeof(frame_pool_s));
    CHECK(pstFramePool, NULL, "failed to malloc %d.\n", sizeof(frame_pool_s));
    memset(pstFramePool, 0, sizeof(frame_pool_s));
    pthread_mutex_init(&pstFramePool->mutex, NULL);

    pstFramePool->hndlist = list_init(sizeof(frame_info_s));
    CHECK(NULL != pstFramePool->hndlist, NULL, "failed to init list.\n");

    pstFramePool->capacity = capacity;

    DBG("init completed.capacity: %d\n", pstFramePool->capacity);

    return (handle)pstFramePool;
}

int frame_pool_destroy(handle hndFramePool)
{
    CHECK(NULL != hndFramePool, -1, "invalid parameter.\n");

    int ret = -1;
    frame_pool_s* pstFramePool = (frame_pool_s*)(hndFramePool);
    frame_info_s* frame_info = list_front(pstFramePool->hndlist);
    while (frame_info)
    {
        if (frame_info->data)
        {
            mem_free(frame_info->data);
            frame_info->data = NULL;
        }
        frame_info = list_next(pstFramePool->hndlist, frame_info);
    }
    ret = list_destroy(pstFramePool->hndlist);
    CHECK(ret == 0, -1, "error with %#x\n", ret);

    pthread_mutex_destroy(&pstFramePool->mutex);
    mem_free(pstFramePool);
    DBG("destroy  completed\n");

    return 0;
}

int frame_pool_add(handle hndFramePool, char *frame, unsigned long len, FRAME_TYPE_E type, int key, double pts)
{
    CHECK(NULL != hndFramePool, -1, "invalid parameter.\n");

    int ret = -1;
    frame_pool_s* pstFramePool = (frame_pool_s*)(hndFramePool);

    ret = pthread_mutex_lock(&pstFramePool->mutex);
    CHECK(ret == 0 , -1, "error with %s\n", strerror(errno));

    if (list_size(pstFramePool->hndlist) >= pstFramePool->capacity)
    {
        frame_info_s* frame_info = list_front(pstFramePool->hndlist);
        if (frame_info->reference <= 0) //
        {
            mem_free(frame_info->data);
            ret = list_pop_front(pstFramePool->hndlist);
            CHECK(ret == 0, -1, "error with %#x\n", ret);
        }
    }

    frame_info_s frame_info_new;
    memset(&frame_info_new, 0, sizeof(frame_info_new));
    frame_info_new.data = mem_malloc(len);
    CHECK(frame_info_new.data, -1, "failed to malloc %d.\n", len);
    memcpy(frame_info_new.data, frame, len);
    frame_info_new.len = len;
    frame_info_new.type = type;
    frame_info_new.timestamp = pts;
    frame_info_new.keyFrame = key;
    if (type == FRAME_TYPE_H264 || type == FRAME_TYPE_H265)
    {
        frame_info_new.sequence = pstFramePool->video_sequence++;
    }
    else if (type == FRAME_TYPE_G711A || type == FRAME_TYPE_G711U)
    {
        frame_info_new.sequence = pstFramePool->audio_sequence++;
    }

    ret = list_push_back(pstFramePool->hndlist, &frame_info_new, sizeof(frame_info_new));
    CHECK(ret == 0, -1, "error with %#x\n", ret);

    //最大缓存到2倍
    ASSERT(list_size(pstFramePool->hndlist) <= pstFramePool->capacity*2, "error with %#x\n", -1);
/*
    DBG("max capacity: %d, current capacity: %d, type: %d, sequence: %d\n", pstFramePool->capacity,
            list_size(pstFramePool->hndlist), frame_info_new.type, frame_info_new.sequence);
*/
    ret = pthread_mutex_unlock(&pstFramePool->mutex);
    CHECK(ret == 0 , -1, "error with %s\n", strerror(errno));

    return 0;
}

static frame_info_s* frame_pool_vkey_find(handle hndlist, int head)
{
    if (head == 1)
    {
        frame_info_s* pNode = (frame_info_s*)list_front(hndlist);
        while (NULL != pNode)
        {
            if ((pNode->type == FRAME_TYPE_H264 || pNode->type == FRAME_TYPE_H265) && pNode->keyFrame)
            {
                return pNode;
            }
            pNode = (frame_info_s*)list_next(hndlist, pNode);
        }
    }
    else
    {
        frame_info_s* pNode = (frame_info_s*)list_back(hndlist);
        while (NULL != pNode)
        {
            if ((pNode->type == FRAME_TYPE_H264 || pNode->type == FRAME_TYPE_H265) && pNode->keyFrame)
            {
                return pNode;
            }
            pNode = (frame_info_s*)list_prev(hndlist, pNode);
        }
    }
    return NULL;
}

static frame_info_s* frame_pool_akey_find(handle hndlist, int head)
{
    if (head == 1)
    {
        frame_info_s* pNode = (frame_info_s*)list_front(hndlist);
        while (NULL != pNode)
        {
            if ((pNode->type == FRAME_TYPE_G711A || pNode->type == FRAME_TYPE_G711A) && pNode->keyFrame)
            {
                return pNode;
            }
            pNode = (frame_info_s*)list_next(hndlist, pNode);
        }
    }
    else
    {
        frame_info_s* pNode = (frame_info_s*)list_back(hndlist);
        while (NULL != pNode)
        {
            if ((pNode->type == FRAME_TYPE_G711A || pNode->type == FRAME_TYPE_G711A) && pNode->keyFrame)
            {
                return pNode;
            }
            pNode = (frame_info_s*)list_prev(hndlist, pNode);
        }
    }
    return NULL;
}

int frame_pool_split_vframe(unsigned char* _pu8Frame, int _u32FrameSize)
{
    int i = 0;
    for(i = 4; i < _u32FrameSize - 4; i++)
    {
        if(_pu8Frame[i+0] == 0x00 && _pu8Frame[i+1] == 0x00 && _pu8Frame[i+2] == 0x00 && _pu8Frame[i+3] == 0x01)
        {
            return i;
        }
    }
    return -1;
}

handle frame_pool_register(handle hndFramePool, int head)
{
    CHECK(NULL != hndFramePool, NULL, "invalid parameter.\n");

    int ret = -1;
    frame_pool_s* pstFramePool = (frame_pool_s*)(hndFramePool);

    ret = pthread_mutex_lock(&pstFramePool->mutex);
    CHECK(ret == 0 , NULL, "error with %s\n", strerror(errno));

    frame_info_s* pstLastVKeyFrame = frame_pool_vkey_find(pstFramePool->hndlist, head);
    CHECK(pstLastVKeyFrame || pthread_mutex_unlock(&pstFramePool->mutex), NULL, "error with %#x\n", pstLastVKeyFrame);
    
    frame_info_s* pstLastAKeyFrame = frame_pool_akey_find(pstFramePool->hndlist, head);
    //CHECK(pstLastAKeyFrame || pthread_mutex_unlock(&pstFramePool->mutex), NULL, "error with %#x\n", pstLastAKeyFrame);
    
    reader_s* pstReader = (reader_s*)mem_malloc(sizeof(reader_s));
    CHECK(pstReader, NULL, "failed to malloc %d.\n", sizeof(reader_s));
    memset(pstReader, 0, sizeof(reader_s));

    pstReader->pstFramePool = pstFramePool;
    pstReader->head = head;
    pstReader->pstLastVKeyFrame = pstLastVKeyFrame;
    pstReader->vframe_type = pstLastVKeyFrame->type;
    pstReader->aframe_type = FRAME_TYPE_INVALID;
    if (pstLastAKeyFrame)
    {
        pstReader->aframe_type = pstLastAKeyFrame->type;
    }
    
    int offset = 0;
    if (pstReader->vframe_type == FRAME_TYPE_H264)
    {
        ret = frame_pool_split_vframe(pstLastVKeyFrame->data, pstLastVKeyFrame->len);
        CHECK(ret > 0 && sizeof(pstReader->sps) >= ret, NULL, "error with %#x\n", ret);
        memcpy(pstReader->sps, pstLastVKeyFrame->data, ret);
        pstReader->sps_size = ret;

        ret = frame_pool_split_vframe(pstLastVKeyFrame->data+pstReader->sps_size, pstLastVKeyFrame->len-pstReader->sps_size);
        CHECK(ret > 0 && sizeof(pstReader->pps) >= ret, NULL, "error with %#x\n", ret);
        memcpy(pstReader->pps, pstLastVKeyFrame->data+pstReader->sps_size, ret);
        pstReader->pps_size = ret;
        
        // 把获取到的sps pps 去掉开始码 00 00 00 01
        pstReader->sps_size -= 4;
        pstReader->pps_size -= 4;
        memmove(pstReader->sps, pstReader->sps + 4, pstReader->sps_size);
        memmove(pstReader->pps, pstReader->pps + 4, pstReader->pps_size);
    }
    else if (pstReader->vframe_type == FRAME_TYPE_H265)
    {
        ret = frame_pool_split_vframe(pstLastVKeyFrame->data+offset, pstLastVKeyFrame->len-offset);
        CHECK(ret > 0 && sizeof(pstReader->vps) >= ret, NULL, "error with %#x\n", ret);
        memcpy(pstReader->vps, pstLastVKeyFrame->data+offset, ret);
        pstReader->vps_size = ret;
        offset += ret;
        CHECK(pstReader->vps[4] == 0x40 && pstReader->vps[5] == 0x01, NULL, "error with %02x %02x\n", pstReader->vps[4], pstReader->vps[5]);
    
        ret = frame_pool_split_vframe(pstLastVKeyFrame->data+offset, pstLastVKeyFrame->len-offset);
        CHECK(ret > 0 && sizeof(pstReader->sps) >= ret, NULL, "error with %#x\n", ret);
        memcpy(pstReader->sps, pstLastVKeyFrame->data+offset, ret);
        pstReader->sps_size = ret;
        offset += ret;
        CHECK(pstReader->sps[4] == 0x42 && pstReader->sps[5] == 0x01, NULL, "error with %02x %02x\n", pstReader->sps[4], pstReader->sps[5]);
        
        ret = frame_pool_split_vframe(pstLastVKeyFrame->data+offset, pstLastVKeyFrame->len-offset);
        CHECK(ret > 0 && sizeof(pstReader->pps) >= ret, NULL, "error with %#x\n", ret);
        memcpy(pstReader->pps, pstLastVKeyFrame->data+offset, ret);
        pstReader->pps_size = ret;
        offset += ret;
        CHECK(pstReader->pps[4] == 0x44 && pstReader->pps[5] == 0x01, NULL, "error with %02x %02x\n", pstReader->pps[4], pstReader->pps[5]);
        
        // 把获取到的sps pps vps 去掉开始码 00 00 00 01
        pstReader->sps_size -= 4;
        pstReader->pps_size -= 4;
        pstReader->vps_size -= 4;
        memmove(pstReader->sps, pstReader->sps + 4, pstReader->sps_size);
        memmove(pstReader->pps, pstReader->pps + 4, pstReader->pps_size);
        memmove(pstReader->vps, pstReader->vps + 4, pstReader->vps_size);
    }

    DBG("register completed.\n");

    ret = pthread_mutex_unlock(&pstFramePool->mutex);
    CHECK(ret == 0 , NULL, "error with %s\n", strerror(errno));

    return (handle)pstReader;
}

int frame_pool_unregister(handle hndReader)
{
    CHECK(NULL != hndReader, -1, "invalid parameter.\n");

    //int ret = -1;
    reader_s* pstReader = (reader_s*)(hndReader);

    mem_free(pstReader);
    DBG("unregister completed.\n");

    return 0;
}

/*
*可能存在frame_pool_register成功后过了很久才frame_pool_get，导致pstLastVKeyFrame已经被删除了
*的风险，list_next会返回NULL，所以frame_pool_get一直会返回NULL
*/
frame_info_s* frame_pool_get(handle hndReader)
{
    CHECK(NULL != hndReader, NULL, "invalid parameter.\n");

    //int ret = -1;
    reader_s* pstReader = (reader_s*)(hndReader);

    pthread_mutex_lock(&pstReader->pstFramePool->mutex);
    frame_info_s* ret_fm = NULL;
    if (pstReader->pstLastVKeyFrame)
    {
        frame_info_s* last_frame = list_next(pstReader->pstFramePool->hndlist, pstReader->pstLastVKeyFrame);
        if (last_frame)
        {
            pstReader->pstLastVKeyFrame->reference++;
            ret_fm = pstReader->pstLastVKeyFrame;
            pstReader->pstLastVKeyFrame = last_frame;
        }
    }

    pthread_mutex_unlock(&pstReader->pstFramePool->mutex);

    return ret_fm;
}

int frame_pool_release(handle hndReader, frame_info_s* pstFrameInfo)
{
    CHECK(NULL != hndReader, -1, "invalid parameter.\n");
    CHECK(NULL != pstFrameInfo, -1, "invalid parameter.\n");

    //int ret = -1;
    reader_s* pstReader = (reader_s*)(hndReader);

    pthread_mutex_lock(&pstReader->pstFramePool->mutex);

    pstFrameInfo->reference--;
    if (pstFrameInfo->reference < 0)
    {
        pstFrameInfo->reference = 0;
    }

    pthread_mutex_unlock(&pstReader->pstFramePool->mutex);

    return 0;
}

int frame_pool_sps_get(handle hndReader, unsigned char** out, int* len)
{
    CHECK(NULL != hndReader, -1, "invalid parameter.\n");

    //int ret = -1;
    reader_s* pstReader = (reader_s*)(hndReader);

    pthread_mutex_lock(&pstReader->pstFramePool->mutex);

    if (pstReader->sps_size > 0)
    {
        *out = pstReader->sps;
        *len = pstReader->sps_size;
    }

    pthread_mutex_unlock(&pstReader->pstFramePool->mutex);

    return 0;
}

int frame_pool_pps_get(handle hndReader, unsigned char** out, int* len)
{
    CHECK(NULL != hndReader, -1, "invalid parameter.\n");

    //int ret = -1;
    reader_s* pstReader = (reader_s*)(hndReader);

    pthread_mutex_lock(&pstReader->pstFramePool->mutex);

    if (pstReader->pps_size > 0)
    {
        *out = pstReader->pps;
        *len = pstReader->pps_size;
    }

    pthread_mutex_unlock(&pstReader->pstFramePool->mutex);

    return 0;
}

int frame_pool_vps_get(handle hndReader, unsigned char** out, int* len)
{
    CHECK(NULL != hndReader, -1, "invalid parameter.\n");

    //int ret = -1;
    reader_s* pstReader = (reader_s*)(hndReader);

    pthread_mutex_lock(&pstReader->pstFramePool->mutex);

    if (pstReader->vps_size > 0)
    {
        *out = pstReader->vps;
        *len = pstReader->vps_size;
    }

    pthread_mutex_unlock(&pstReader->pstFramePool->mutex);

    return 0;
}

int frame_pool_vframe_type_get(handle hndReader, FRAME_TYPE_E* out)
{
    CHECK(NULL != hndReader, -1, "invalid parameter.\n");
    CHECK(NULL != out, -1, "invalid parameter.\n");

    //int ret = -1;
    reader_s* pstReader = (reader_s*)(hndReader);

    pthread_mutex_lock(&pstReader->pstFramePool->mutex);
    
    *out = pstReader->vframe_type;

    pthread_mutex_unlock(&pstReader->pstFramePool->mutex);

    return 0;
}

int frame_pool_aframe_type_get(handle hndReader, FRAME_TYPE_E* out)
{
    CHECK(NULL != hndReader, -1, "invalid parameter.\n");
    CHECK(NULL != out, -1, "invalid parameter.\n");

    //int ret = -1;
    reader_s* pstReader = (reader_s*)(hndReader);

    pthread_mutex_lock(&pstReader->pstFramePool->mutex);

    *out = pstReader->aframe_type;

    pthread_mutex_unlock(&pstReader->pstFramePool->mutex);

    return 0;
}


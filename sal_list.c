
#include "sal_debug.h"
#include "sal_malloc.h"
#include "sal_list.h"


#define LIST_MAGIC_NUM 0xabcd0000

typedef struct list_node_s
{
    void* pData;
    struct list_node_s* pstNext;
    struct list_node_s* pstPrev;
}list_node_s;

typedef struct list_s
{
    unsigned int u32MagicNumer;
    int s32Cnt; // node numbers of list
    int s32NodeDataSize; // node size

    list_node_s* pstHead;
    list_node_s* pstTail;
}list_s;

static list_node_s* list_look_up(handle _hndList, void* pData)
{
    list_s* pstList = (list_s*)_hndList;
    list_node_s* tmp = pstList->pstHead;
    while (NULL != tmp)
    {
        if (tmp->pData == pData)
        {
            return tmp;
        }
        tmp = tmp->pstNext;
    }
    ERR("the node is not exist.\n");

    return NULL;
}

/*
init list, malloc resource
*/
handle list_init(int s32NodeDataSize)
{
    CHECK(s32NodeDataSize > 0, NULL, "invalid parameter.\n");

    list_s* pstList = (list_s*)mem_malloc(sizeof(list_s));
    CHECK(NULL != pstList, NULL, "failed to malloc %d\n", sizeof(list_s));

    memset(pstList, 0, sizeof(list_s));
    pstList->u32MagicNumer = LIST_MAGIC_NUM;
    pstList->s32NodeDataSize = s32NodeDataSize;
    pstList->s32Cnt = 0;
    pstList->pstHead = NULL;
    pstList->pstTail = NULL;

    return (handle)pstList;
}

/*
destroy list, free resource
*/
int list_destroy(handle _hndList)
{
    CHECK(NULL != _hndList, -1, "invalid parameter with: %#x\n", _hndList);
    CHECK(*((unsigned int*)_hndList) == LIST_MAGIC_NUM, -1, "invalid parameter with: %#x\n", *((unsigned int*)_hndList));

    int ret = -1;
    ret = list_clear(_hndList);
    CHECK(!ret, ret, "error with %#x\n", ret);

    mem_free(_hndList);
    DBG("completed.\n");
    return ret;
}

/*
add one node from list head
*/
int list_push_front(handle _hndList, void* pData, int DataSize)
{
    CHECK(NULL != _hndList, -1, "invalid parameter with: %#x\n", _hndList);
    CHECK(*((unsigned int*)_hndList) == LIST_MAGIC_NUM, -1, "invalid parameter with: %#x\n", *((unsigned int*)_hndList));
    CHECK(NULL != pData, -1, "invalid parameter with: %#x\n", pData);

    list_s* pstList = (list_s*)_hndList;
    CHECK(pstList->s32NodeDataSize == DataSize, -1, "invalid parameter.\n");

    list_node_s* pstNewNode = (list_node_s*)mem_malloc(sizeof(list_node_s) + pstList->s32NodeDataSize);
    CHECK(NULL != pstNewNode, -1, "failed to malloc %d\n", sizeof(list_node_s) + pstList->s32NodeDataSize);

    memset(pstNewNode, 0, sizeof(list_node_s) + pstList->s32NodeDataSize);
    pstNewNode->pData = (void*)(pstNewNode+1);
    memcpy(pstNewNode->pData, pData, pstList->s32NodeDataSize);
    if (NULL == pstList->pstTail && NULL == pstList->pstHead)
    {
        pstList->pstHead = pstNewNode;
        pstList->pstTail = pstNewNode;
        pstNewNode->pstNext = NULL;
        pstNewNode->pstPrev = NULL;
    }
    else
    {
        pstList->pstHead->pstPrev = pstNewNode;
        pstNewNode->pstNext = pstList->pstHead;
        pstList->pstHead = pstNewNode;
        pstNewNode->pstPrev = NULL;
    }
    pstList->s32Cnt++;

    return 0;
}

/*
add one node from list tail
*/
int list_push_back(handle _hndList, void* pData, int DataSize)
{
    CHECK(NULL != _hndList, -1, "invalid parameter with: %#x\n", _hndList);
    CHECK(*((unsigned int*)_hndList) == LIST_MAGIC_NUM, -1, "invalid parameter with: %#x\n", *((unsigned int*)_hndList));
    CHECK(NULL != pData, -1, "invalid parameter with: %#x\n", pData);
    //DBG("magic number: %#x\n", (unsigned int*)_hndList);

    list_s* pstList = (list_s*)_hndList;
    CHECK(pstList->s32NodeDataSize == DataSize, -1, "invalid parameter.\n");

    list_node_s* pstNewNode = (list_node_s*)mem_malloc(sizeof(list_node_s) + pstList->s32NodeDataSize);
    CHECK(NULL != pstNewNode, -1, "failed to malloc %d\n", sizeof(list_node_s) + pstList->s32NodeDataSize);

    memset(pstNewNode, 0, sizeof(list_node_s) + pstList->s32NodeDataSize);
    pstNewNode->pData = (void*)(pstNewNode+1);
    memcpy(pstNewNode->pData, pData, pstList->s32NodeDataSize);
    if (NULL == pstList->pstTail && NULL == pstList->pstHead)
    {
        pstList->pstHead = pstNewNode;
        pstList->pstTail = pstNewNode;
        pstNewNode->pstNext = NULL;
        pstNewNode->pstPrev = NULL;
    }
    else
    {
        pstList->pstTail->pstNext = pstNewNode;
        pstNewNode->pstPrev = pstList->pstTail;
        pstList->pstTail = pstNewNode;
        pstNewNode->pstNext = NULL;
    }
    pstList->s32Cnt++;

    return 0;
}

/*
*delete node specified
*/
int list_del(handle _hndList, void* pData)
{
    CHECK(NULL != _hndList, -1, "invalid parameter with: %#x\n", _hndList);
    CHECK(*((unsigned int*)_hndList) == LIST_MAGIC_NUM, -1, "invalid parameter with: %#x\n", *((unsigned int*)_hndList));
    CHECK(NULL != pData, -1, "invalid parameter with: %#x\n", pData);

    list_s* pstList = (list_s*)_hndList;
    list_node_s* pstFind = list_look_up(pstList, pData);
    if (NULL != pstFind)
    {
        if (pstList->s32Cnt == 1)
        {
            pstList->pstHead = NULL;
            pstList->pstTail = NULL;
        }
        else
        {
            if (pstFind == pstList->pstHead)
            {
                pstList->pstHead->pstNext->pstPrev = NULL;
                pstList->pstHead = pstList->pstHead->pstNext;
            }
            else if (pstFind == pstList->pstTail)
            {
                pstList->pstTail->pstPrev->pstNext = NULL;
                pstList->pstTail = pstList->pstTail->pstPrev;
            }
            else
            {
                pstFind->pstPrev->pstNext = pstFind->pstNext;
                pstFind->pstNext->pstPrev = pstFind->pstPrev;
            }
        }

        mem_free(pstFind);
        pstList->s32Cnt--;
    }

    return 0;
}

/*
*delete one node from list head
*/
int list_pop_front(handle _hndList)
{
    CHECK(NULL != _hndList, -1, "invalid parameter with: %#x\n", _hndList);
    CHECK(*((unsigned int*)_hndList) == LIST_MAGIC_NUM, -1, "invalid parameter with: %#x\n", *((unsigned int*)_hndList));

    list_s* pstList = (list_s*)_hndList;
    if (NULL != pstList->pstHead)
    {
        if (pstList->s32Cnt == 1) // only one node
        {
            mem_free(pstList->pstHead);
            pstList->pstHead = NULL;
            pstList->pstTail = NULL;
        }
        else
        {
            list_node_s* tmp = pstList->pstHead;
            pstList->pstHead->pstNext->pstPrev = NULL;
            pstList->pstHead = pstList->pstHead->pstNext;

            mem_free(tmp);
        }
        pstList->s32Cnt--;
        //DBG("delete head node success.list size: %d\n", pstList->s32Cnt);
    }

    return 0;
}

/*
*delete one node from list tail
*/
int list_pop_back(handle _hndList)
{
    CHECK(NULL != _hndList, -1, "invalid parameter with: %#x\n", _hndList);
    CHECK(*((unsigned int*)_hndList) == LIST_MAGIC_NUM, -1, "invalid parameter with: %#x\n", *((unsigned int*)_hndList));

    list_s* pstList = (list_s*)_hndList;
    if (NULL != pstList->pstTail)
    {
        if (pstList->s32Cnt == 1) // only one node
        {
            mem_free(pstList->pstTail);
            pstList->pstHead = NULL;
            pstList->pstTail = NULL;
        }
        else
        {
            list_node_s* tmp = pstList->pstTail;
            pstList->pstTail->pstPrev->pstNext = NULL;
            pstList->pstTail = pstList->pstTail->pstPrev;

            mem_free(tmp);
        }
        pstList->s32Cnt--;
        //DBG("delete tail node success.list size: %d\n", pstList->s32Cnt);
    }

    return 0;
}

/*
delete all nodes of list
*/
int list_clear(handle _hndList)
{
    CHECK(NULL != _hndList, -1, "invalid parameter with: %#x\n", _hndList);
    CHECK(*((unsigned int*)_hndList) == LIST_MAGIC_NUM, -1, "invalid parameter with: %#x\n", *((unsigned int*)_hndList));

    int ret = -1;
    while (list_size(_hndList) > 0)
    {
        ret = list_pop_back(_hndList);
        CHECK(!ret, ret, "error with %#x\n", ret);
    }

    return 0;
}

/*
return the numbers of list node
*/
int list_size(handle _hndList)
{
    CHECK(NULL != _hndList, -1, "invalid parameter with: %#x\n", _hndList);
    CHECK(*((unsigned int*)_hndList) == LIST_MAGIC_NUM, -1, "invalid parameter with: %#x\n", *((unsigned int*)_hndList));

    list_s* pstList = (list_s*)_hndList;

    return pstList->s32Cnt;
}

/*
return head node of list
*/
void* list_front(handle _hndList)
{
    CHECK(NULL != _hndList, NULL, "invalid parameter with: %#x\n", _hndList);
    CHECK(*((unsigned int*)_hndList) == LIST_MAGIC_NUM, NULL, "invalid parameter with: %#x\n", *((unsigned int*)_hndList));

    list_s* pstList = (list_s*)_hndList;

    return (pstList->pstHead) ? pstList->pstHead->pData : NULL;
}

/*
return tail node of list
*/
void* list_back(handle _hndList)
{
    CHECK(NULL != _hndList, NULL, "invalid parameter with: %#x\n", _hndList);
    CHECK(*((unsigned int*)_hndList) == LIST_MAGIC_NUM, NULL, "invalid parameter with: %#x\n", *((unsigned int*)_hndList));

    list_s* pstList = (list_s*)_hndList;

    return (pstList->pstTail) ? pstList->pstTail->pData : NULL;
}

/*
if list is empty, then return true
*/
int list_empty(handle _hndList)
{
    CHECK(NULL != _hndList, -1, "invalid parameter with: %#x\n", _hndList);
    CHECK(*((unsigned int*)_hndList) == LIST_MAGIC_NUM, -1, "invalid parameter with: %#x\n", *((unsigned int*)_hndList));

    list_s* pstList = (list_s*)_hndList;

    return (pstList->s32Cnt == 0);
}

/*
if the node is exist in list, then return true
*/
int list_exist(handle _hndList, void* pData)
{
    CHECK(NULL != _hndList, -1, "invalid parameter with: %#x\n", _hndList);
    CHECK(*((unsigned int*)_hndList) == LIST_MAGIC_NUM, -1, "invalid parameter with: %#x\n", *((unsigned int*)_hndList));
    CHECK(NULL != pData, -1, "invalid parameter with: %#x\n", pData);
    
    int bExist = 0;
    list_node_s* pstFind = list_look_up(_hndList, pData);
    if (NULL != pstFind)
    {
        bExist = 1;
    }

    return bExist;
}

/*
return next node of list
*/
void* list_next(handle _hndList, void* pData)
{
    CHECK(NULL != _hndList, NULL, "invalid parameter with: %#x\n", _hndList);
    CHECK(*((unsigned int*)_hndList) == LIST_MAGIC_NUM, NULL, "invalid parameter with: %#x\n", *((unsigned int*)_hndList));
    CHECK(NULL != pData, NULL, "invalid parameter with: %#x\n", pData);

    list_node_s* pstFind = list_look_up(_hndList, pData);
    if (NULL != pstFind && NULL != pstFind->pstNext)
    {
        return pstFind->pstNext->pData;
    }

    return NULL;
}

/*
return previous node of list
*/
void* list_prev(handle _hndList, void* pData)
{
    CHECK(NULL != _hndList, NULL, "invalid parameter with: %#x\n", _hndList);
    CHECK(*((unsigned int*)_hndList) == LIST_MAGIC_NUM, NULL, "invalid parameter with: %#x\n", *((unsigned int*)_hndList));
    CHECK(NULL != pData, NULL, "invalid parameter with: %#x\n", pData);

    list_node_s* pstFind = list_look_up(_hndList, pData);
    if (NULL != pstFind && NULL != pstFind->pstPrev)
    {
        return pstFind->pstPrev->pData;
    }

    return NULL;
}


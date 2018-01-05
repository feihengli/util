#ifndef __LIST_H__
#define __LIST_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#include "sal_standard.h"

handle list_init(int _s32NodeDataSize);
int list_destroy(handle _hndList);
int list_push_front(handle _hndList, void* _pData, int _s32DataSize);
int list_push_back(handle _hndList, void* _pData, int _s32DataSize);
int list_del(handle _hndList, void* pData);
int list_pop_front(handle _hndList);
int list_pop_back(handle _hndList);
int list_clear(handle _hndList);
int list_size(handle _hndList);
void* list_front(handle _hndList);
void* list_back(handle _hndList);
void* list_next(handle _hndList, void* pData);
void* list_prev(handle _hndList, void* pData);
int list_empty(handle _hndList);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif


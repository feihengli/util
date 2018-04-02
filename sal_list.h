#ifndef __LIST_H__
#define __LIST_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#include "sal_standard.h"

/*
 函 数 名: list_init
 功能描述: 初始化一个双向链表，需与list_destroy配对使用
 输入参数:  _s32NodeDataSize 链表节点存放的数据大小
 输出参数: 无
 返 回 值: 成功返回链表句柄,失败返回NULL
*/
handle list_init(int _s32NodeDataSize);

/*
 函 数 名: list_destroy
 功能描述: 去初始化一个双向链表，需与list_init配对使用
 输入参数:  _hndList 链表句柄
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int list_destroy(handle _hndList);

/*
 函 数 名: list_push_front
 功能描述: 在链表头部新增一个节点
 输入参数:  _hndList 链表句柄
            _pData 节点数据
            _s32DataSize 节点大小
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int list_push_front(handle _hndList, void* _pData, int _s32DataSize);

/*
 函 数 名: list_push_back
 功能描述: 在链表尾部新增一个节点
 输入参数:  _hndList 链表句柄
            _pData 节点数据
            _s32DataSize 节点大小
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int list_push_back(handle _hndList, void* _pData, int _s32DataSize);

/*
 函 数 名: list_del
 功能描述: 在链表中删除指定节点
 输入参数:  _hndList 链表句柄
            pData 待删除的节点
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int list_del(handle _hndList, void* pData);

/*
 函 数 名: list_pop_front
 功能描述: 在链表头部删除一个节点
 输入参数:  _hndList 链表句柄
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int list_pop_front(handle _hndList);

/*
 函 数 名: list_pop_back
 功能描述: 在链表尾部删除一个节点
 输入参数:  _hndList 链表句柄
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int list_pop_back(handle _hndList);

/*
 函 数 名: list_clear
 功能描述: 删除链表所有节点
 输入参数:  _hndList 链表句柄
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int list_clear(handle _hndList);

/*
 函 数 名: list_size
 功能描述: 获取链表中节点的数目
 输入参数:  _hndList 链表句柄
 输出参数: 无
 返 回 值: 成功返回链表数目,失败返回小于0
*/
int list_size(handle _hndList);

/*
 函 数 名: list_front
 功能描述: 获取链表中头节点
 输入参数:  _hndList 链表句柄
 输出参数: 无
 返 回 值: 成功返回节点,失败返回NULL
*/
void* list_front(handle _hndList);

/*
 函 数 名: list_back
 功能描述: 获取链表中尾节点
 输入参数:  _hndList 链表句柄
 输出参数: 无
 返 回 值: 成功返回节点,失败返回NULL
*/
void* list_back(handle _hndList);

/*
 函 数 名: list_next
 功能描述: 获取链表中后一个节点
 输入参数:  _hndList 链表句柄
            pData 节点
 输出参数: 无
 返 回 值: 成功返回节点,失败返回NULL
*/
void* list_next(handle _hndList, void* pData);

/*
 函 数 名: list_prev
 功能描述: 获取链表中前一个节点
 输入参数:  _hndList 链表句柄
            pData 节点
 输出参数: 无
 返 回 值: 成功返回节点,失败返回NULL
*/
void* list_prev(handle _hndList, void* pData);

/*
 函 数 名: list_empty
 功能描述: 链表是否为空
 输入参数:  _hndList 链表句柄
 输出参数: 无
 返 回 值: 成功返回真,失败返回假
*/
int list_empty(handle _hndList);

/*
 函 数 名: list_exist
 功能描述: 节点是否存在于链表
 输入参数:  _hndList 链表句柄
            pData 节点
 输出参数: 无
 返 回 值: 成功返回真,失败返回假
*/
int list_exist(handle _hndList, void* pData);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif


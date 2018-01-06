#ifndef __SAL_SELECT_H__
#define __SAL_SELECT_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#include "sal_standard.h"

/*
 函 数 名: completeness_cb
 功能描述: 判断数据是否是一个完整的数据包
 输入参数:  pData 待检测的数据
            DataLen 数据长度
            user 用户参数
 输出参数: _ps32CompleteLen 数据完整的长度：>1 表示接收到数据是完整的，且完整的数据包长度为_ps32CompleteLen，处理掉这个数据包
                                            =1 表示接收到数据是错误的，内部会丢弃一个字节继续检测
                                            =0 表示接收到数据是不完整的，内部不做处理，等待下次继续检测
 返 回 值: 成功返回0,失败返回小于0
*/
typedef int (*completeness_cb)(void* pData, int DataLen, int* _ps32CompleteLen, void* cb_param);

/*
 函 数 名: select_init
 功能描述: 初始化select模块
 输入参数: complete 检查一个数据包是否完整
            cache_size 接收缓存的大小
            fd 文件描述符
 输出参数: 无
 返 回 值: 成功返回句柄,失败返回NULL
*/
handle select_init(completeness_cb complete, void* cb_param, int cache_size, int fd);

/*
 函 数 名: select_destroy
 功能描述: 去初始化select模块
 输入参数: hndselect select模块句柄
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int select_destroy(handle hndselect);

/*
 函 数 名: select_rw
 功能描述：select 负责读写，需要循环调用
 输入参数: hndselect select模块句柄
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int select_rw(handle hndselect);

/*
 函 数 名: select_wtimeout
 功能描述: 写超时时间
 输入参数: hndselect select模块句柄
 输出参数: 无
 返 回 值: 成功返回写超时时间（毫秒）,失败返回小于0
*/
int select_wtimeout(handle hndselect);

/*
 函 数 名: select_rtimeout
 功能描述: 读超时时间
 输入参数: hndselect select模块句柄
 输出参数: 无
 返 回 值: 成功返回读超时时间（毫秒）,失败返回小于0
*/
int select_rtimeout(handle hndselect);

/*
 函 数 名: select_send
 功能描述: 发送数据，此函数即时返回，不阻塞。
 输入参数: hndselect select模块句柄
            pData 数据
            DataLen 数据大小
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int select_send(handle hndselect, void* pData, int DataLen);

/*
 函 数 名: select_recv_get
 功能描述: 获取已经读到的数据，处理完之后需select_recv_release
 输入参数: hndselect select模块句柄
 输出参数:    data 数据
            len 数据大小
 返 回 值: 成功返回0,失败返回小于0
*/
int select_recv_get(handle hndselect, void** data, int* len);

/*
 函 数 名: select_recv_release
 功能描述: 解锁通过select_recv_get获取到的数据
 输入参数: hndselect select模块句柄
            data 数据
            len 数据大小
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int select_recv_release(handle hndselect, void** data, int* len);

/*
 函 数 名: select_sentsize
 功能描述: 获取已经发送数据的总量
 输入参数: hndselect select模块句柄
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int select_sentsize(handle hndselect);

/*
 函 数 名: select_readsize
 功能描述: 获取已经读取数据的总量
 输入参数: hndselect select模块句柄
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int select_readsize(handle hndselect);

/*
 函 数 名: select_debug
 功能描述: 使能select模块的调试信息
 输入参数: hndselect select模块句柄
            enable 使能开关
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int select_debug(handle hndselect, int enable);


int select_wlist_empty(handle hndselect);
int select_rlist_empty(handle hndselect);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif


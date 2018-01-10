#ifndef __SAL_MTD_H__
#define __SAL_MTD_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#include "sal_standard.h"

/*
 函 数 名: sal_mtd_erase
 功能描述: 从分区起始地址开始擦除mtd分区
 输入参数: devicename 分区节点（eg. /dev/mtd0）
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int sal_mtd_erase(char *devicename);

/*
 函 数 名: sal_mtd_rerase
 功能描述: 从分区末端地址开始反向擦除mtd分区
 输入参数: devicename 分区节点（eg. /dev/mtd0）
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int sal_mtd_rerase(char *devicename);

/*
 函 数 名: sal_mtd_write
 功能描述: 写分区
 输入参数:  filebuf 待写入的内容
            filesize 内容大小
            devicename 分区节点（eg. /dev/mtd0）
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int sal_mtd_write(char *filebuf, int filesize, char *devicename);

/*
 函 数 名: sal_mtd_verify
 功能描述: 验证分区节点的内容是否与filebuf一致
 输入参数:  filebuf 待验证的内容
            filesize 内容大小
            devicename 分区节点（eg. /dev/mtd0）
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int sal_mtd_verify(char *filebuf, int filesize, char *devicename);

/*
 函 数 名: sal_mtd_readflag
 功能描述: 读取分区末4字节的内容，供做系统双备份时使用
 输入参数:  len 输入output的最大长度
            devicename 分区节点（eg. /dev/mtd0）
 输出参数: output 输出4字节内容
 返 回 值: 成功返回0,失败返回小于0
*/
int sal_mtd_readflag(char *devicename, char* output, int len);

/*
 函 数 名: sal_mtd_writeflag
 功能描述: 写分区末4字节的内容，供做系统双备份时使用
 输入参数:  input 输入4字节的内容
            len 输入output的最大长度
            devicename 分区节点（eg. /dev/mtd0）
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int sal_mtd_writeflag(char *devicename, char* input, int len);



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif


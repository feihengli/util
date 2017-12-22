
#ifndef __SAL_FLASH_H__
#define __SAL_FLASH_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif


/*****************************************************************************
 函 数 名: sal_flashcp
 功能描述  : SAL层flash烧写
 输入参数  : buf 镜像
            size 镜像大小,
            devicename  mtd模块节点 比如，"/dev/mtd1"
 输出参数  :
 返 回 值: 成功返回0, 失败返回小于0
*****************************************************************************/
int sal_flashcp(char *buf, int size,  char *devicename);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif


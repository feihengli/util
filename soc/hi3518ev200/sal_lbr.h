
#ifndef __SAL_LBR_H__
#define __SAL_LBR_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif


/*
 函 数 名: sal_lbr_init
 功能描述: 初始化低码率模块
 输入参数: 无
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int sal_lbr_init();

/*
 函 数 名: sal_lbr_exit
 功能描述: 去初始化低码率模块
 输入参数: 无
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int sal_lbr_exit();

/*
 函 数 名: sal_lbr_get_str
 功能描述: 获取lbr当前的模式，内部使用接口
 输入参数: buf：输入buffer
            len: buf的长度
 输出参数: buf：输出lbr当前的模式
 返 回 值: 成功返回0,失败返回小于0
*/
int sal_lbr_get_str(char* buf, int len);

/*
 函 数 名: sal_lbr_get_vpstr
 功能描述: 获取lbr当前各个模式的统计信息，内部使用接口
 输入参数: buf：输入buffer
            len: buf的长度
 输出参数: buf：输出lbr当前的模式
 返 回 值: 成功返回0,失败返回小于0
*/
int sal_lbr_get_vpstr(char* buf, int len);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif


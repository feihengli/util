
#ifndef __SAL_AF_H__
#define __SAL_AF_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

/*
 函 数 名: sal_af_register_callback
 功能描述: AF模块注册相关回调，内部使用函数接口
 输入参数:
 输出参数:
 返 回 值: 设置成功返回0, 失败返回小于0
*/
int sal_af_register_callback(void);

/*
 函 数 名: sal_af_unregister_callback
 功能描述: AF模块反注册相关回调，内部使用函数接口
 输入参数:
 输出参数:
 返 回 值: 设置成功返回0, 失败返回小于0
*/
int sal_af_unregister_callback(void);

/*
 函 数 名: sal_af_get_fv
 功能描述: SAL层获取AF的值，外部可调用
 输入参数:
 输出参数:
 返 回 值: 设置成功返回0, 失败返回小于0
*/
int sal_af_get_fv(int *value);



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif


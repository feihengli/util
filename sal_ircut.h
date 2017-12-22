#ifndef __SAL_IRCUT_H__
#define __SAL_IRCUT_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif


typedef enum
{
    SAL_IR_MODE_UNSUPPORT = -1,// 硬件不支持红外
    SAL_IR_MODE_AUTO = 0,		// 自动红外，默认
    SAL_IR_MODE_ON   = 1,		// 强制开红外
    SAL_IR_MODE_OFF  = 2,		// 强制关红外
}SAL_IR_MODE;

/*
 函 数 名: sal_ircut_init
 功能描述  : 初始化ircut模块，默认为自动模式
 输入参数  : 无
 输出参数  : 无
 返 回 值: 成功返回0,失败返回小于0
*/
int sal_ircut_init();

/*****************************************************************************
 函 数 名: sal_ircut_exit
 功能描述  : 去初始化ircut模块
 输入参数  : 无
 输出参数  : 无
 返 回 值: 成功返回0,失败返回小于0
*****************************************************************************/
int sal_ircut_exit();

/*****************************************************************************
 函 数 名: sal_ircut_mode_set
 功能描述  : 设置ircut红外模式
 输入参数  : mode ircut红外模式
 输出参数  : 无
 返 回 值: 成功返回0,失败返回小于0
*****************************************************************************/
int sal_ircut_mode_set(SAL_IR_MODE mode);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif


#endif




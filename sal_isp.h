
#ifndef __SAL_ISP_H__
#define __SAL_ISP_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

 typedef enum SAL_ISP_MODE_E
 {
     SAL_ISP_MODE_DAY = 1,
     SAL_ISP_MODE_NIGHT = 0,
 }SAL_ISP_MODE_E;

 typedef enum
 {
     SAL_IMAGE_SATURATION = 0,
     SAL_IMAGE_CONTRAST,
     SAL_IMAGE_BRIGHTNESS,
     SAL_IMAGE_SHARPNESS
 }SAL_IMAGE_E;

 typedef struct sal_exposure_info_s
 {
     unsigned int ExpTime;
     unsigned int AGain;
     unsigned int DGain;
     unsigned int ISPGain;
     unsigned char AveLuma;
     unsigned int ColorTemp;
 }sal_exposure_info_s;

 typedef struct
 {
     unsigned char saturation;
     unsigned char contrast;
     unsigned char brightness;
     unsigned char sharpness;
 }sal_image_attr_s;

 typedef struct
 {
    int slow_shutter_enable;    //0 disable / 1 enable
    sal_image_attr_s image_attr;
 }sal_isp_s;


/*****************************************************************************
 函 数 名: sal_isp_init
 功能描述  : SAL层isp初始化，主要解析iq 的xml参数文件，并使能到dsp
 输入参数  : 无
 输出参数  : 无
 返 回 值: 成功返回0,失败返回小于0
*****************************************************************************/
int sal_isp_init(sal_isp_s* param);


/*****************************************************************************
 函 数 名: sal_isp_exit
 功能描述  : SAL层isp去初始化，回收isp 模块用到的系统资源
 输入参数  : 无
 输出参数  : 无
 返 回 值: 成功返回0,失败返回小于0
*****************************************************************************/
int sal_isp_exit(void);


/*****************************************************************************
 函 数 名: sal_isp_day_night_switch
 功能描述  : SAL层isp切换日夜iq参数
 输入参数  : mode 日夜模式
            attr 图像基本参数
 输出参数  : 无
 返 回 值: 成功返回0,失败返回小于0
*****************************************************************************/
int sal_isp_day_night_switch(SAL_ISP_MODE_E mode, sal_image_attr_s* attr);


/*****************************************************************************
 函 数 名: sal_isp_image_attr_set
 功能描述  : SAL层设置图像属性
 输入参数  :    type  (饱和度，对比度，亮度，锐度)
            value 图像属性的量化值，范围0-255，128是默认值
                  如传入默认值需要设置isp内部自动调节
 输出参数  : 无
 返 回 值: 成功返回0,失败返回小于0
*****************************************************************************/
int sal_isp_image_attr_set(SAL_IMAGE_E type, unsigned char value);

/*****************************************************************************
 函 数 名: sal_isp_slow_shutter_set
 功能描述  : SAL层是否使能降帧
 输入参数  : 1 enable /0 disable
 输出参数  : 无
 返 回 值: 成功返回0,失败返回小于0
*****************************************************************************/
int sal_isp_slow_shutter_set(int enable);


/*****************************************************************************
 函 数 名: sal_isp_exposure_info_get
 功能描述  : SAL层获取isp中曝光相关信息如:
            曝光时间、sensor模拟增益、sensor数字增益、isp的增益
 输入参数  : 无
 输出参数  : sal_exposure_info_s 类型
 返 回 值: 成功返回0,失败返回小于0
*****************************************************************************/
int sal_isp_exposure_info_get(sal_exposure_info_s* info);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif


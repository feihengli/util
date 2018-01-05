#ifndef __SAL_OSD_H__
#define __SAL_OSD_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#define RGB_VALUE_BLACK     0x8000  //黑
#define RGB_VALUE_WHITE     0xffff  // 白色
#define RGB_VALUE_BG        0x7fff //透明


typedef enum OSD_TITLE_TYPE_E
{
    TITLE_ADD_NOTHING = 0,
    TITLE_ADD_RESOLUTION = 1,
    TITLE_ADD_BITRATE = 2,
    TITLE_ADD_ALL = 3,
    TITLE_ADD_BUTT,
}OSD_TITLE_TYPE_E;

typedef enum OSD_LOCATION_E
{
    OSD_LEFT_TOP = 0,
    OSD_LEFT_BOTTOM = 1,
    OSD_RIGHT_TOP = 2,
    OSD_RIGHT_BOTTOM = 3,
    OSD_HIDE = 4,
}OSD_LOCATION_E;

typedef struct sal_osd_item_s
{
    int enable;
    OSD_LOCATION_E location;

    unsigned long tmformat;              //only valid for time
    OSD_TITLE_TYPE_E type;              //only valid for title title type
    char buffer[256];
    char buffer_bak[256];              //only for title type eg: add bitrate
}sal_osd_item_s;

/*
 函 数 名: sal_osd_init
 功能描述: 初始化OSD模块，默认显示时间和标题
 输入参数: 无
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int sal_osd_init();

/*
 函 数 名: sal_osd_exit
 功能描述: 去初始化OSD模块
 输入参数: 无
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int sal_osd_exit();


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif

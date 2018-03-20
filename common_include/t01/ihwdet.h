/*
 * Copyright (C) 2014 Ingenic Semiconductor Co.,Ltd
 */

#ifndef __IHWDET_H__
#define __IHWDET_H__

#include "ioattr.h"
#include "object.h"
#include "channel.h"
#include "ext.h"
#include "debug.h"
#include "event.h"
#include "typedef.h"

typedef enum _IHwDetBlock {
	MD_BLOCK,
	MD_NOBLOCK,
} IHwDetBlock;

typedef enum _IHwDetMetaDataMode {
	META_DATA_OBJECT = 0,
	META_DATA_RAW = 1,
} IHwDetMetaDataMode;

typedef enum _IHwDetThumbnailsSwitch {
	THUMBNAILS_DISABLE = 0,
	THUMBNAILS_ENABLE = 1,
} IHwDetThumbnailsSwitch;

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

/**
 * Init T01
 **/
int IHwDet_Init(IHwDetAttr *Attr);

/**
 * Start T01
 **/
int IHwDet_Start(IHwDetMetaDataMode mode,
		 IHwDetThumbnailsSwitch thumbnails_enable);

/**
 * Get the fps of metadata
 **/
int IHwDet_Get_MD_Fps(void);

/**
 * sync time between T01 and Host SOC
 **/
int IHwDet_Time_Sync(int auto_sync, int interval_min);

/**
 * Stop T01
 **/
int IHwDet_Stop(void);

/**
 * Open T01 TX stream on
 **/
int IHwDet_Mipi_Bypass_On(void);

/**
 * Reserved
 **/
int IHwDet_Mipi_Bypass_Off(void);

/**
 * Reset T01
 **/
int IHwDet_Hardware_Reset(void);

/**
 * Deinit T01, free source
 **/
int IHwDet_DeInit(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __IHWDET_H__ */

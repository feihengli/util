/*
 * Copyright (C) 2014 Ingenic Semiconductor Co.,Ltd
 */

#ifndef __EVENT_H__
#define __EVENT_H__

typedef enum _IHwDetEventSens {
	EVENT_SENS_HIGH = 1,
	EVENT_SENS_MID = 2,
	EVENT_SENS_LOW = 3,
} IHwDetEventSens;

typedef struct _PersonAppearCallback {
	IHwDetEventSens Sens;
	int (*person_appear)(void);
} PersonAppearCallback;

typedef struct _PersonDisAppearCallback {
	IHwDetEventSens Sens;
	int (*person_disappear)(void);
} PersonDisAppearCallback;

typedef struct _IHwDetEventCallbacks {
	PersonAppearCallback person_appear;
	PersonDisAppearCallback person_disappear;
} IHwDetEventCallbacks;

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

/**
 * init callbacks
 **/
int IHwDet_Event_Init(IHwDetEventCallbacks *callbacks);

/**
 * deinit callbacks
 **/
int IHwDet_Event_DeInit(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __EVENT_H__ */

/*
 * Copyright (C) 2014 Ingenic Semiconductor Co.,Ltd
 */

#ifndef __EXT_H__
#define __EXT_H__

typedef struct _IHwDetSensorExp {
	unsigned int analog_gain;
	unsigned int integration_time;
} IHwDetSensorExp;

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */


/**
 * Get the sensor exposure information
 **/
int IHwDet_Get_Sensor_Exp(IHwDetSensorExp *exp);

/**
 * Set and Get the offset value
 **/
int IHwDet_Get_Offset(void);
int IHwDet_Set_Offset(int offset);

/**
 * Set and Get the scales value
 **/
int IHwDet_Get_Scales(void);
int IHwDet_Set_Scales(int scales);

/**
 * Capture the raw image and raw metadata in T01
 **/
int IHwDet_Capture_Raw_Image(void);

/**
 *
 **/
int IHwDet_Load_Model_F(char* path);

/**
 *
 **/
int IHwDet_Get_Load_Model_Progress(void);

/**
 * Update calibration value to T01
 **/
int IHwDet_Update_Calibration(unsigned char* buf, unsigned int size);

/**
 * Spirit channel API
 **/
int IHwDet_Channel_API(unsigned int v_addr, unsigned int value);

/**
 * Check the version number of T01 firmware
 **/
int IHwDet_Check_T01_Version(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __EXT_H__ */

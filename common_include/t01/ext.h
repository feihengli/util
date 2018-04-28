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

#define TSELFTEST                                         0x00000000
#define TSPIRIT                                           0x00000001
#define TIMAGE                                            0x00000002
#define TSYSTEM                                           0x00000003
#define TALGORITHMS                                       0x00000004
#define CALIBRATION                                       0x00000005
#define TREGISTERS                                        0x00000006


// ------------------------------------------------------------------------------ //
//		BUFFER TYPES
// ------------------------------------------------------------------------------ //
#define STATIC_CALIBRATIONS_ID                            0x00000000
#define DYNAMIC_CALIBRATIONS_ID                           0x00000001
#define FILE_TRANSFER_ID                                  0x00000002


// ------------------------------------------------------------------------------ //
//		COMMAND LIST
// ------------------------------------------------------------------------------ //
#define SENSOR_ID                                         0x00000000
#define SENSOR_INTERFACE                                  0x00000001
#define ISP_INTERFACE                                     0x00000002
#define ISP_REVISION                                      0x00000003
#define FW_REVISION                                       0x00000004
#define API_REVISION                                      0x00000005
#define CALIBRATION_REVISION                              0x00000006
#define SPIRIT_SKIP_FRAME_COUNT                           0x00000007
#define SPIRIT_MODE                                       0x00000009
#define SPIRIT_SCALES                                     0x0000000A
#define SPIRIT_OFFSETS                                    0x0000000B
#define SPIRIT_DATA_SETUP                                 0x0000000C
#define SPIRIT_MODEL_ENABLE                               0x0000000D
#define SPIRIT_MODEL_MASK                                 0x0000000E
#define SPIRIT_CLEAR_MODELS                               0x0000000F
#define SPIRIT_THUMBNAIL_ENABLE                           0x00000010
#define SPIRIT_THUMBNAIL_FACE_SIZE                        0x00000011
#define SPIRIT_THUMBNAIL_HS_SIZE                          0x00000012
#define SPIRIT_THUMBNAIL_HS_MIN_STRENGTH                  0x00000013
#define SPIRIT_THUMBNAIL_FACE_MIN_STRENGTH                0x00000014
#define SPIRIT_THUMBNAIL_HSFACE_MIN_LIFE                  0x00000015
#define SPIRIT_THUMBNAIL_FACE_RATE_LIMIT                  0x00000016
#define SPIRIT_THUMBNAIL_HS_RATE_LIMIT                    0x00000017
#define SPIRIT_THUMBNAIL_FACEHS_RATE_LIMIT                0x00000018
#define SPIRIT_CF_SCORE_ON_FAILURE                        0x0000001A
#define SPIRIT_CF_PREPROCESSING                           0x0000001B
#define SPIRIT_TEMPORAL_FILTER                            0x0000001C
#define SPIRIT_ANNOT_STRENGTH_THRESH                      0x0000001D
#define RESOLUTION_ACTIVE_IMAGE_ID                        0x0000001E
#define BUFFER_DATA_TYPE                                  0x0000001F
#define ISP_SYSTEM_STATE                                  0x00000020
#define TEST_PATTERN_ENABLE_ID                            0x00000021
#define TEST_PATTERN_MODE_ID                              0x00000022
#define SYSTEM_FREEZE_FIRMWARE                            0x00000023
#define SYSTEM_MANUAL_EXPOSURE                            0x00000024
#define SYSTEM_MANUAL_INTEGRATION_TIME                    0x00000025
#define SYSTEM_MANUAL_SENSOR_ANALOG_GAIN                  0x00000026
#define SYSTEM_MANUAL_SENSOR_DIGITAL_GAIN                 0x00000027
#define SYSTEM_EXPOSURE                                   0x00000028
#define SYSTEM_INTEGRATION_TIME                           0x00000029
#define SYSTEM_MAX_INTEGRATION_TIME                       0x0000002A
#define SYSTEM_SENSOR_ANALOG_GAIN                         0x0000002B
#define SYSTEM_MAX_SENSOR_ANALOG_GAIN                     0x0000002C
#define SYSTEM_SENSOR_DIGITAL_GAIN                        0x0000002D
#define SYSTEM_MAX_SENSOR_DIGITAL_GAIN                    0x0000002E
#define AE_MODE_ID                                        0x0000002F
#define AE_GAIN_ID                                        0x00000030
#define AE_EXPOSURE_ID                                    0x00000031
#define AE_ROI_ID                                         0x00000032
#define AE_COMPENSATION_ID                                0x00000033
#define REGISTERS_ADDRESS_ID                              0x00000034
#define REGISTERS_SIZE_ID                                 0x00000035
#define REGISTERS_SOURCE_ID                               0x00000036
#define REGISTERS_VALUE_ID                                0x00000037

// ------------------------------------------------------------------------------ //
//		VALUE LIST
// ------------------------------------------------------------------------------ //
#define SPIRIT_IDLE                                       0x00000000
#define SPIRIT_ACTIVE                                     0x00000001
#define SPIRIT_SETUP_RAW                                  0x00000002
#define SPIRIT_SETUP_RGB                                  0x00000003
#define SPIRIT_SETUP_YUV                                  0x00000004
#define PREVIEW_RES_FPS_MAX                               0x00000005
#define FULL_RES_FPS_MAX                                  0x00000006
#define PAUSE                                             0x00000007
#define RUN                                               0x00000008
#define ON                                                0x00000009
#define OFF                                               0x0000000A
#define AE_AUTO                                           0x0000000B
#define AE_MANUAL                                         0x0000000C
#define SENSOR                                            0x0000000D
#define ISP                                               0x0000000E
#define MIPI                                              0x0000000F


// ------------------------------------------------------------------------------ //
//		RETURN VALUES
// ------------------------------------------------------------------------------ //
#define SUCCESS                                           0x00000000
#define NOT_IMPLEMENTED                                   0x00000001
#define NOT_SUPPORTED                                     0x00000002
#define NOT_PERMITTED                                     0x00000003
#define NOT_EXISTS                                        0x00000004
#define FAIL                                              0x00000005


// ------------------------------------------------------------------------------ //
//		ERROR REASONS
// ------------------------------------------------------------------------------ //
#define ERR_UNKNOWN                                       0x00000000
#define ERR_BAD_ARGUMENT                                  0x00000001
#define ERR_WRONG_SIZE                                    0x00000002


// ------------------------------------------------------------------------------ //
//		CALIBRATION VALUES
// ------------------------------------------------------------------------------ //
//------------------ISP CALIBRATIONS-------------------
#define CALIBRATION_AE_BALANCED_LINEAR                    0x00000003
//------------------FILE TRANSFER-------------------
#define SPIRIT_MODEL_FILE                                 0x00000000
#define SPIRIT_BLOB_FILE                                  0x00000001



// ------------------------------------------------------------------------------ //
//		DIRECTION VALUES
// ------------------------------------------------------------------------------ //
#define COMMAND_SET                                       0x00000000
#define COMMAND_GET                                       0x00000001
#define API_VERSION                                       0x00000064

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

/**
 * Modify the tracking parameters
 **/
int IHwDet_Set_Tracking_Parameter(long ch, float confThresh, int clearThresh);

/**
 * reserved
 **/
int IHwDet_Mode_Control(unsigned int enable_or_disable);
int IHwDet_Cf_Score_On_Failure(unsigned int set_score);
int IHwDet_Annot_Strength_Thresh(unsigned int set_strength_thresh);
int	IHwDet_Temporal_Filter(unsigned int set_enable_or_disable);
int IHwDet_Skip_Frame_Count(unsigned int set_skip_frame_count);
int IHwDet_Cf_Preprocessing(unsigned int set_enable_or_disable);

int IHwDet_System_Manual_Exposure(unsigned int set_enable_or_disable);
int IHwDet_System_Exposure(unsigned int set_exporsure_value);

int IHwDet_System_Manual_Integration_Time(unsigned int set_enable_or_disable);
int IHwDet_Max_Intergration_Time(unsigned int set_max_intergration_time);
int IHwDet_Intergration_Time(unsigned int set_intergration_time);

int IHwDet_Manual_Sensor_Analog_Gain(unsigned int set_enable_or_disable);
int IHwDet_Max_Sensor_Analog_Gain(unsigned set_max_sensor_analog_gain);
int IHwDet_Sensor_Analog_Gain(unsigned set_sensor_analog_gain);

int IHwDet_Manual_Sensor_Digital_Gain(unsigned int set_enable_or_disable);
int IHwDet_Max_Sensor_Digital_Gain(unsigned int set_max_sensor_digital_gain);
int IHwDet_Sensor_Digital_Gain(unsigned int set_sensor_digital_gain);

int IHwDet_Ae_Mode_ID(unsigned int set_manual_or_auto);
int IHwDet_Ae_Gain_ID(unsigned int set_ae_gain_value);
int IHwDet_Ae_Exposure_ID(unsigned int set_ae_exposure_value);
int IHwDet_Ae_Roi_ID(unsigned int set_ae_roi);
int IHwDet_Ae_Compensation_ID(unsigned int set_ae_compensation);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __EXT_H__ */

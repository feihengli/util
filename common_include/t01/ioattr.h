/*
 * Copyright (C) 2017 Ingenic Semiconductor Co.,Ltd
 */

#ifndef __IOATTR_H__
#define __IOATTR_H__

// Input
#define INPUT_INTERFACE_MIPI		0x0
#define INPUT_INTERFACE_DVP			0x1
#define INPUT_INTERFACE_DVP_SONY	0x2
#define INPUT_INTERFACE_BT1120		0x3
#define INPUT_INTERFACE_BT656		0x4
#define INPUT_INTERFACE_BT601		0x5

/* data type for dvp interface */
#define INPUT_DVP_DATA_TYPE_RAW8		0x0
#define INPUT_DVP_DATA_TYPE_RAW10		0x1
#define INPUT_DVP_DATA_TYPE_RAW12		0x2
#define INPUT_DVP_DATA_TYPE_YUV422_16B	0x3
#define INPUT_DVP_DATA_TYPE_RGB565_16B	0x4
#define INPUT_DVP_DATA_TYPE_RGB888_16B	0x5
#define INPUT_DVP_DATA_TYPE_YUV422_8B	0x6
#define INPUT_DVP_DATA_TYPE_RGB565_555	0x7

/* data type for mipi interface */
#define INPUT_MIPI_DATA_TYPE_RAW8		0x0
#define INPUT_MIPI_DATA_TYPE_RAW10		0x1
#define INPUT_MIPI_DATA_TYPE_RAW12		0x2
#define INPUT_MIPI_DATA_TYPE_RGB555		0x3
#define INPUT_MIPI_DATA_TYPE_RGB565		0x4
#define INPUT_MIPI_DATA_TYPE_RGB888		0x5
#define INPUT_MIPI_DATA_TYPE_YUV422_8B	0x6
#define INPUT_MIPI_DATA_TYPE_YUV422_10B	0x7

#define MIPI_LANE_2	   	0x1

// Output (Only MIPI)
#define OUTPUT_DATA_TYPE_RGB888 0x1
#define OUTPUT_DATA_TYPE_RGB565 0x2
#define OUTPUT_DATA_TYPE_RAW8   0x3
#define OUTPUT_DATA_TYPE_RAW10  0x4
#define OUTPUT_DATA_TYPE_RAW12  0x5
#define OUTPUT_DATA_TYPE_RGB666 0xC
#define OUTPUT_DATA_TYPE_RGB555 0xD

typedef struct {
	unsigned short hfb_num;				/* horizon front blanking : hsync mode(sony dvp) */
	unsigned short hact_num;			/* reserved */
	unsigned short hbb_num;				/* reserved */
	unsigned short odd_vfb;				/* reserved */
	unsigned short odd_vact;			/* reserved */
	unsigned short odd_vbb;				/* reserved */
	unsigned short even_vfb;			/* reserved */
	unsigned short even_vact;			/* reserved */
	unsigned short even_vbb;			/* reserved */
	unsigned int vic_input_vpara3;		/* vertical sync front porch blanking : hsync mode(sony dvp) */
} IHwDetVicBlanking;

typedef struct _IHwDetInputInfo {
	int interface;						/* Sensor interface type */
	int data_type;						/* Sensor data type */
	int image_width;					/* Width in pixel of sensor output */
	int image_height;					/* Height in pixel of sensor output */
	int raw_rggb;						/* Sensor RGB pattern */
	int raw_align;						/* The alignment of raw data for DVP interface */
	unsigned int dvp_config;			/* Vic dvp input config */
	IHwDetVicBlanking blanking;			/* Blanking of dvp hsync mode */
	union {
		int dvp_bus_select;				/* reserved */
		int mipi_lane;					/* The number of mipi input lane */
		int value;						/* bt config parameter */
	} config;							/* the range is 0x0 ~ 0x7fff */
	int fps;							/* Fps of sensor output */
	int mclk_enable;					/* reserved */
} IHwDetInputInfo;

typedef struct _IHwDetOutputInfo {
	int fps_ratio;						/* reserved */
	int data_type;						/* MIPI output data type */
	int image_width;					/* Width in pixel of MIPI output */
	int image_height;					/* Height in pixel of MIPI output */
	int clk_mode;						/* reserved */
	int mipi_lane;						/* The number of mipi output line */
	int colorbar;						/* reserved */
	int colorbar_enable;				/* reserved */
} IHwDetOutputInfo;

typedef struct _IHwDetAttr {
	IHwDetInputInfo input;				/* T01 input config */
	IHwDetOutputInfo output;			/* only used in mipi bypass mode */
} IHwDetAttr;

#endif /* __IOATTR_H__ */

/*
 * Copyright Ingenic Limited 2011 - 2016.  All rights reserved.
 */

#ifndef __TARGET_H__
#define __TARGET_H__

#include <stdint.h>

#include "typedef.h"
#include "object.h"

#define TARGET_TYPE_PERSON	0
#define TARGET_TYPE_GESTURE	1

typedef struct _IngTarget_T {
	uint8_t         id;         /* target ID */
	uint8_t         type;       /* target type */
	uint16_t        x;          /* x-position in pixels (top left corner) */
	uint16_t        y;          /* y-position in pixels (top left corner) */
	uint16_t        width;      /* Size in pixels (along x-direction) */
	uint16_t        height;     /* Size in pixels (along y-direction) */
	uint8_t         yaw;        /* angle (yaw, 0 ~ 180 degrees) */
	uint8_t         tilt;       /* tilt (roll, 0 ~ 180 degrees) */
	float           strength;   /* target strength */
} IngTarget_T;

typedef struct _IngDetDesc_T {
	uint32_t        frame;    /* video frame index */
	int32_t         error;    /* indicate whether an error occured */
	uint64_t        ts;       /* timestamp in milliseconds */
	uint32_t        tgtnum;   /* number of target in current frame */
	uint8_t		_private[16];  /* private data */
} IngDetDesc_T;

#endif /* __TARGET_H__ */

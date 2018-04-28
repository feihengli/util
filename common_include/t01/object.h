/*
 * Copyright Ingenic Limited 2011 - 2016.  All rights reserved.
 */

#ifndef __OBJECT_H__
#define __OBJECT_H__

#include <stdint.h>

#include "typedef.h"

/**
 * object type
 *
 * NOTE: type relation tree
 * 	HW: hardware detection
 * 	SW: software calculation
 *
 *  HW:    face	     hs	     figure
 *           \	    /  \       /
 *            \	   /    \     /
 *  SW:	      upbody    fullbody
 *                 \    /
 *                  \  /
 *  SW:            person
 *
 **/
#define OBJ_TYPE_FACE			0
#define OBJ_TYPE_HEAD_AND_SHOULDER	1
#define OBJ_TYPE_FIGURE			2
#define OBJ_TYPE_UPPER_BODY		3
#define OBJ_TYPE_FULL_BODY		4
#define OBJ_TYPE_PALM			5
#define OBJ_TYPE_PERSON			6

/**
 * Tracking type
 **/
#define TRACKING_TYPE_FACE			(1 << OBJ_TYPE_FACE)
#define TRACKING_TYPE_HEAD_AND_SHOULDER		(1 << OBJ_TYPE_HEAD_AND_SHOULDER)
#define TRACKING_TYPE_FIGURE			(1 << OBJ_TYPE_FIGURE)
#define TRACKING_TYPE_UPPER_BODY		(1 << OBJ_TYPE_UPPER_BODY)
#define TRACKING_TYPE_FULL_BODY			(1 << OBJ_TYPE_FULL_BODY)
#define TRACKING_TYPE_PALM			(1 << OBJ_TYPE_PALM)
#define TRACKING_TYPE_PERSON			(1 << OBJ_TYPE_PERSON)

/**
 * thumbnail type
 **/
#define THUMB_TYPE_FACE			OBJ_TYPE_FACE
#define THUMB_TYPE_HEAD_AND_SHOULDER	OBJ_TYPE_HEAD_AND_SHOULDER

/**
 * default strength filter
 **/
#define DEFAULT_STRENGTH_FILTER		0.1

/**
 * thumbnail data type
 **/
#define THUMB_DATA_TYPE_RAW12		7
#define MAX_PARENTS_NUM			5

typedef struct _IngObjParent {
	unsigned char type;		/* if type == -1, not vaild parent */
	unsigned short offset;		/* parent offset in the metadata of one frame */
} IngObjParent;

typedef struct _IngDetObject {
    uint8_t         id;         /* object ID, NOTE: reserved */
    uint8_t         type;       /* object type */
    uint16_t        x;          /* x-position in pixels (top left corner) */
    uint16_t        y;          /* y-position in pixels (top left corner) */
    uint16_t        width;      /* Size in pixels (along x-direction) */
    uint16_t        height;     /* Size in pixels (along y-direction) */
    uint8_t         yaw;        /* angle (yaw, 0 ~ 180 degrees) */
    uint8_t         tilt;       /* tilt (roll, 0 ~ 180 degrees) */
    uint8_t         pitch;      /* pitch (0 ~ 180 degrees) */
    uint8_t         nthumbs;    /* Number of thumbnails associated with this object, NOTE: reserved */
    char         	life;       /* tracked object life, NOTE: reserved */
    uint8_t	    nparents;	/* number of parents */
    float           strength;   /* detected object strength */
    IngObjParent    parents[MAX_PARENTS_NUM];	/* parents info */
} IngDetObject;

typedef struct _IngDetDesc {
	uint32_t        frame;    /* video frame index */
	int32_t         error;    /* indicate whether an error occured */
	uint64_t        ts;       /* timestamp in milliseconds */
	uint32_t        objnum;   /* number of objects in current frame */
	uint8_t			_private[16];  /* private data */
} IngDetDesc;

typedef struct _IngDetThumb {   /* NOTE: reserved */
    uint32_t objid;         	/* indicate the thumbnail belongs to, equal to id in IngDetObject */
    uint32_t type;          	/* type of the thumbnail */
    uint32_t data_type;     	/* type of the thumbnail data */
    uint16_t width;         	/* width in pixels */
    uint16_t height;        	/* height in pixels */
    uint16_t stride;        	/* line stride in bytes */
    const void *data;       	/* thumbnail data */
} IngDetThumb;

typedef struct _RaWMetaData {
	void *meta_data;
	int size;
	unsigned long long timestamp;
} RawMetaData;

#endif /* __OBJECT_H__ */

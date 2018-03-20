/*
 * Copyright Ingenic Limited 2011 - 2016.  All rights reserved.
 */

#ifndef __CHANNEL_H__
#define __CHANNEL_H__

#include <stdint.h>

#include "target.h"
#include "typedef.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

/*************************************************************************\
 * Normal Channel                                                        *
\*************************************************************************/
/**
 * Create one Channel
 * return: channel context
 **/
ch_context_t IHwDet_Create_Channel(int block);

/**
 * change Channel block
 **/
void IHwDet_Set_Channel_Block(ch_context_t ch, int block);

/**
 * enable tracking
 * filter_strength: if set 0, use default value
 **/
int IHwDet_Enable_Tracking(ch_context_t ch, int tracking_type, float filter_strength);

/**
 * disable tracking
 **/
int IHwDet_Disable_Tracking(ch_context_t ch);

/**
 * return metadata context of one frame
 **/
md_context_t IHwDet_Request_MD(ch_context_t ch, IngDetDesc *desc);

/**
 * get detection object
 **/
IngDetObject *IHwDet_Get_Object(md_context_t md_context, uint32_t obj_id);

/**
 * get metadata in raw format
 **/
RawMetaData *IHwDet_Get_Raw_Meta_Data(md_context_t md_context);

/**
 * not used
 **/
IngDetThumb *IHwDet_Get_Thumbnail(md_context_t md_context, uint32_t obj_id);

/**
 * release meta of one frame
 **/
int IHwDet_Release_MD(ch_context_t ch, md_context_t md_context);

/**
 * Destory one Channel
 **/
int IHwDet_Destroy_Channel(ch_context_t ch);

/*************************************************************************\
 * T Channel                                                             *
\*************************************************************************/
/**
 * Create one Channel
 * return: channel context
 **/
ch_context_t IHwDet_Create_Channel_T(int block);

/**
 * change Channel block
 **/
void IHwDet_Set_Channel_Block_T(ch_context_t ch, int block);

/**
 * return metadata context of one frame
 **/
md_context_t IHwDet_Request_MD_T(ch_context_t ch, IngDetDesc_T *desc);

/**
 * get target
 **/
IngTarget_T *IHwDet_Get_Target(md_context_t md_context, uint32_t target_id);

/**
 * only for target type = TARGET_TYPE_PERSON
 **/
int IHwDet_Target_Have_Face(IngTarget_T *target);

/**
 * only for target type = TARGET_TYPE_PERSON
 **/
int IHwDet_Target_Have_HeadAndShoulder(IngTarget_T *target);

/**
 * only for target type = TARGET_TYPE_PERSON
 **/
int IHwDet_Target_Have_Figure(IngTarget_T *target);

/**
 * only for target type = TARGET_TYPE_PERSON
 **/
/* int IHwDet_Target_Have_UpBody(IngTarget_T *target); */

/**
 * only for target type = TARGET_TYPE_PERSON
 **/
/* int IHwDet_Target_Have_FullBody(IngTarget_T *target); */

/**
 * get object of target
 **/
IngDetObject *IHwDet_Target_Get_Object(IngTarget_T *target, uint32_t obj_type);

/**
 * release meta of one frame
 **/
int IHwDet_Release_MD_T(ch_context_t ch, md_context_t md_context);

/**
 * Destory one Channel
 **/
int IHwDet_Destroy_Channel_T(ch_context_t ch);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __CHANNEL_H__ */

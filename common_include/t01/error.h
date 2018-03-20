/*
 * Copyright Ingenic Limited 2011 - 2016.  All rights reserved.
 */

#ifndef __STATUS_H__
#define __STATUS_H__

/**
 * error type
 **/
#define IHWDET_ERROR_NO_RESPONSE	-1

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

/**
 * get fetal error Status.
 **/
int IHwDet_Set_Error_Handler(void (*error_handler)(int error_type));

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __OBJECT_H__ */

/*
 * Copyright (C) 2014 Ingenic Semiconductor Co.,Ltd
 */

#ifndef __DEBUG_H__
#define __DEBUG_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

/**
 * read T01 registers
 **/
int IHwDet_Reg_Read(unsigned int addr, unsigned int *value);

/**
 * write T01 registers
 **/
int IHwDet_Reg_Write(unsigned int addr, unsigned int value);

/**
 * To be realized
 **/
/* int IHwDet_Capture_Raw_Image(void *buf); */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __DEBUG_H__ */

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

#define RET_SUCCESS  1
#define RET_FAIL     2

/**
 * read T01 registers
 **/
int IHwDet_Reg_Read(unsigned int addr, unsigned int *value);

/**
 * write T01 registers
 **/
int IHwDet_Reg_Write(unsigned int addr, unsigned int value);

int spirit_api_cmd_set(int cmd_type, int cmd, unsigned int *data);
int spirit_api_cmd_get(int cmd_type, int cmd, unsigned int *data);

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

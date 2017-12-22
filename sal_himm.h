#ifndef __SAL_HIMM_H__
#define __SAL_HIMM_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#include "sal_standard.h"

unsigned int himm_write(unsigned int addr, unsigned int data);
unsigned int himm_read(unsigned int addr);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif


#endif




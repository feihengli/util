#ifndef __SAL_MD5_H__
#define __SAL_MD5_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#include "sal_standard.h"


int md5_filesum(char* _szFilePath, unsigned char sum[64]);
int md5_memsum(char* input, int len, unsigned char sum[64]);



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif

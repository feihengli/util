#ifndef __SAL_MTD_H__
#define __SAL_MTD_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#include "sal_standard.h"

int sal_mtd_erase(char *devicename);
int sal_mtd_rerase(char *devicename);
int sal_mtd_write(char *filebuf, int filesize, char *devicename);
int sal_mtd_verify(char *filebuf, int filesize, char *devicename);

int sal_mtd_readflag(char *devicename, char* output, int len);
int sal_mtd_writeflag(char *devicename, char* input, int len);



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif


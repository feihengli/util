#ifndef sal_mp4record_h__
#define sal_mp4record_h__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#include "sal_standard.h"

handle mp4EncoderInit(const char * filename,int width,int height, int fps);
int mp4VEncoderWrite(handle hndMp4, unsigned char* _naluData ,int _naluSize);
int mp4AEncode(uint8_t * data ,int len);
int mp4Encoderclose(handle hndMp4);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif 

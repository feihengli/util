#ifndef __MA_LIVE555_INTERFACE_H__
#define __MA_LIVE555_INTERFACE_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

char* MY_Base64Encode(unsigned char* _au8Buf, unsigned int _u32BufSize);
char* MY_Authrization(char* _szCmd, char* _szUrl,
		char* _szUsr, char* _szPwd,
		char* _szRealm, char* _szNonce);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif


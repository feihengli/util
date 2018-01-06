#ifndef __MA_LIVE555_INTERFACE_H__
#define __MA_LIVE555_INTERFACE_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

/*
 函 数 名: MY_Base64Encode
 功能描述: base64加密。因为返回的指针是在内部使用malloc申请的，外部需调用free释放
 输入参数: _au8Buf 输入buf
            _u32BufSize buf的长度
 输出参数: 无
 返 回 值: 成功返回加密后的字符串,失败返回NULL
*/
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


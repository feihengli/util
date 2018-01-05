#ifndef __SAL_CURL_H__
#define __SAL_CURL_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#include "sal_standard.h"

typedef enum
{
    CURL_RESULT_NULL,
    CURL_RESULT_OK,
    CURL_RESULT_MANUAL_STOP,
    CURL_RESULT_WRITE_ERROR,
    CURL_RESULT_READ_TIMEOUT,
    CURL_RESULT_UNKNOWN,
} CURL_RESULT_E;

typedef struct
{
    unsigned int u32Progress;
    int bRunning;
    CURL_RESULT_E enResult;

    unsigned char* pu8Recv;
    unsigned int u32RecvHeaderTotal;
    unsigned int u32RecvBodyTotal;
} CURL_STATUS_S;

handle curl_wrapper_init(int timeout);
int curl_wrapper_destroy(handle _hndCurlWrapper);
int curl_wrapper_StartHttpPostImg(handle _hndCurlWrapper, char* _szUrl, char* _szFilename);
int curl_wrapper_StartHttpPost(handle _hndCurlWrapper, char* _szUrl, unsigned char* _pUlData, unsigned int _u32UlDataSize);
int curl_wrapper_StartHttpGet(handle _hndCurlWrapper, char* _szUrl);
int curl_wrapper_StartUpload(handle _hndCurlWrapper, char* _szUrl, char* _szUsr, char* _szPwd, char* _szLocalPath);
int curl_wrapper_StartDownload(handle _hndCurlWrapper, char* _szUrl, char* _szUsr, char* _szPwd, char* _szLocalPath);
int curl_wrapper_StartDownload2Mem(handle _hndCurlWrapper, char* _szUrl, char* _szUsr, char* _szPwd);
int curl_wrapper_Download2MemGet(handle _hndCurlWrapper, char** output, int* len);
int curl_wrapper_GetProgress(handle _hndCurlWrapper, CURL_STATUS_S* _pstStatus);
int curl_wrapper_Stop(handle _hndCurlWrapper);
int curl_wrapper_GetPostData(handle _hndCurlWrapper, char* _szRetUrl, unsigned char* _au8RetData, unsigned int* _u32RetDataSize);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif


#endif




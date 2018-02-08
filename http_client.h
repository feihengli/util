#ifndef http_client_h__
#define http_client_h__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#include "sal_standard.h"

//socket 层运行线程的情况
typedef enum
{
    HTTP_STATUS_INVALID = 0,
    HTTP_STATUS_PROGRESS,
    HTTP_STATUS_FINISH_OK,
    HTTP_STATUS_FINISH_ERROR,
    HTTP_STATUS_WRITE_TIMEOUT,
    HTTP_STATUS_READ_TIMEOUT,
    HTTP_STATUS_UNKNOWN,
}HTTP_STATUS_E;


handle http_client_init(char* szUrl, int timeout);
int http_client_exit(handle hndHttpClient);
int http_client_Query(handle hndHttpClient, HTTP_STATUS_E* status);
int http_client_HeaderGet(handle hndHttpClient, unsigned char** output, int* len);
int http_client_ContentGet(handle hndHttpClient, unsigned char** output, int* len);
int http_client_ProgressGet(handle hndHttpClient, int* progress);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif


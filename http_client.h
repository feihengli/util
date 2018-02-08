#ifndef http_client_h__
#define http_client_h__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

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


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif


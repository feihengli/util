
#ifndef ftp_client_h__
#define ftp_client_h__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#include "sal_standard.h"


//socket 层运行线程的情况
typedef enum FTP_STATUS_E
{
    FTP_STATUS_INVALID = 0,
    FTP_STATUS_PROGRESS,      //正在进行中
    FTP_STATUS_FINISH_OK,     //完成ok
    FTP_STATUS_FINISH_ERROR,  //完成error
    FTP_STATUS_WRITE_TIMEOUT, //写超时
    FTP_STATUS_READ_TIMEOUT,  //读超时
    FTP_STATUS_UNKNOWN,
}FTP_STATUS_E;

/*
 函 数 名: ftp_client_init
 功能描述: 初始化ftp客户端模块
 输入参数: szHost 需要访问的FTP Server域名或ip
            portCmd ftp 端口默认为21
            szUserName 用户名
            szPassword 密码
 输出参数: 无
 返 回 值: 成功返回句柄,失败返回NULL
*/
handle ftp_client_init(char* szHost, int portCmd, char* szUserName, char* szPassword);

/*
 函 数 名: ftp_client_exit
 功能描述: 去初始化ftp客户端模块
 输入参数: hndftpClient 句柄
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int ftp_client_exit(handle hndftpClient);

/*
 函 数 名: ftp_client_startGet
 功能描述: ftpget 下载文件
 输入参数: hndftpClient 句柄
            szFileName 文件名
            timeout 超时时间（毫秒）
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int ftp_client_startGet(handle hndftpClient, char* szFileName, int timeout);

/*
 函 数 名: ftp_client_startPut
 功能描述: ftpput 上传文件
 输入参数: hndftpClient 句柄
            buffer 文件buffer
            len 文件长度
            szRemoteFileName 上传到ftp服务器的文件名
            timeout 超时时间（毫秒）
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int ftp_client_startPut(handle hndftpClient, unsigned char* buffer, int len, char* szRemoteFileName, int timeout);

/*
 函 数 名: ftp_client_stop
 功能描述: 停止上传或下载，释放资源
 输入参数: hndftpClient 句柄
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int ftp_client_stop(handle hndftpClient);

/*
 函 数 名: ftp_client_Query
 功能描述: 查询ftp状态
 输入参数: hndftpClient 句柄
 输出参数: status 查询状态
 返 回 值: 成功返回0,失败返回小于0
*/
int ftp_client_Query(handle hndftpClient, FTP_STATUS_E* status);

/*
 函 数 名: ftp_client_ContentGet
 功能描述: 获取ftpget的内容（下载文件）
 输入参数: hndftpClient 句柄
 输出参数: output 输出buffer
            len buffer长度
 返 回 值: 成功返回0,失败返回小于0
*/
int ftp_client_ContentGet(handle hndftpClient, unsigned char** output, int* len);

/*
 函 数 名: ftp_client_ProgressGet
 功能描述: 获取ftpget/ftpput的进度
 输入参数: hndftpClient 句柄
 输出参数: progress 进度（0-100）
 返 回 值: 成功返回0,失败返回小于0
*/
int ftp_client_ProgressGet(handle hndftpClient, int* progress);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif


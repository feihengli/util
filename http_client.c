
#include "sal_list.h"
#include "sal_select.h"
#include "sal_debug.h"
#include "sal_malloc.h"
#include "sal_util.h"
#include "http_client.h"

#define GOTO(exp, exec, to, fmt...) do\
{\
    if (!(exp))\
    {\
        ERR(fmt);\
        exec;\
        goto to;\
    }\
}while(0)

typedef struct http_client_s
{
    handle hndSocket;
    int cache_size;
    int multi_pack_enable;
    int remain_size;
    int Fd;
    pthread_t tPid;
    int bRunning;

    char szUrl[256];
    char szUsrPwd[32];
    
    char szRequestResource[256];
    char szDomain[256];
    char szIp[16];
    int port;

    int timeout; //超时时间（毫秒
    HTTP_STATUS_E enStatus;
    int progress;

    unsigned char* pu8Header; //malloc 申请
    unsigned int u32HeaderSize;      //buffer 大小
    unsigned int u32HeaderOffset;    //可写的offset

    unsigned char* pu8Content; //malloc 申请
    unsigned int u32ContentSize;      //buffer 大小
    unsigned int u32ContentOffset;    //可写的offset
}
http_client_s;

static int http_complete(void* _pData, int _u32DataLen, int* _pu32CompleteLen, void* cb_param)
{
    //DBG("_u32DataLen=%u\n", _u32DataLen);
    http_client_s* pstHttpClient = (http_client_s*)cb_param;
    *_pu32CompleteLen = 0;
    char szLine[4*1024];
    int minSize = (sizeof(szLine) > _u32DataLen) ? _u32DataLen : sizeof(szLine);
    memcpy(szLine, _pData, minSize);
    szLine[sizeof(szLine)-1] = 0;
    
    if (pstHttpClient->multi_pack_enable)
    {
        if (pstHttpClient->remain_size >= pstHttpClient->cache_size/2 
                && _u32DataLen >= pstHttpClient->cache_size/2)
        {
            *_pu32CompleteLen = pstHttpClient->cache_size/2;
            pstHttpClient->remain_size -= (*_pu32CompleteLen);
        }
        else if (pstHttpClient->remain_size == _u32DataLen)
        {
            *_pu32CompleteLen = pstHttpClient->remain_size;
            pstHttpClient->remain_size -= (*_pu32CompleteLen);
        }
        
        if (pstHttpClient->remain_size == 0)
        {
            pstHttpClient->multi_pack_enable = 0;
        }
        //DBG("multi_pack_enable: %d, remain_size: %d, _u32DataLen: %d, block size: %d, CompleteLen: %d\n", 
        //            pstHttpClient->multi_pack_enable, pstHttpClient->remain_size,
        //            _u32DataLen, pstHttpClient->cache_size/2, (*_pu32CompleteLen));
        
        return 0;
    }
    
    CHECK(!strncmp(szLine, "HTTP", 4), -1, "protocol is not HTTP\n");
    char* szHeaderReturn = strstr(szLine, "\r\n\r\n");
    char* szContentLength = NULL;
    if (szHeaderReturn != NULL)
    {
        szContentLength = strstr(szLine, "Content-Length: ");
        if (szContentLength == NULL)
        {
            *_pu32CompleteLen = szHeaderReturn - szLine + 4;
            pstHttpClient->u32HeaderSize = szHeaderReturn - szLine + 4;
            pstHttpClient->u32HeaderOffset = 0;
            pstHttpClient->pu8Header = mem_malloc(pstHttpClient->u32HeaderSize+1);
            CHECK(pstHttpClient->pu8Header, -1, "error with %#x\n", pstHttpClient->pu8Header);
            memset(pstHttpClient->pu8Header, 0, pstHttpClient->u32HeaderSize+1);
            
            pstHttpClient->u32ContentSize = 0;
            pstHttpClient->u32ContentOffset = 0;
            pstHttpClient->pu8Content = NULL;
            pstHttpClient->remain_size = 0;
            pstHttpClient->multi_pack_enable = 0;
        }
        else if (szContentLength != NULL)
        {
            *_pu32CompleteLen = szHeaderReturn - szLine + 4;
            pstHttpClient->u32HeaderSize = szHeaderReturn - szLine + 4;
            pstHttpClient->u32HeaderOffset = 0;
            pstHttpClient->pu8Header = mem_malloc(pstHttpClient->u32HeaderSize+1);
            CHECK(pstHttpClient->pu8Header, -1, "error with %#x\n", pstHttpClient->pu8Header);
            memset(pstHttpClient->pu8Header, 0, pstHttpClient->u32HeaderSize+1);
            
            pstHttpClient->u32ContentSize = atoi(szContentLength + strlen("Content-Length: "));
            pstHttpClient->u32ContentOffset = 0;
            pstHttpClient->pu8Content = mem_malloc(pstHttpClient->u32ContentSize+1);
            CHECK(pstHttpClient->pu8Content, -1, "error with %#x\n", pstHttpClient->pu8Content);
            memset(pstHttpClient->pu8Content, 0, pstHttpClient->u32ContentSize+1);
            pstHttpClient->remain_size = pstHttpClient->u32ContentSize;
            pstHttpClient->multi_pack_enable = 1;
        }
    }
    else
    {
        WRN("\n");
    }

    //DBG("_u32DataLen: %u, HeaderSize: %d, ContentLength: %d, CompleteLen: %d\n", _u32DataLen, pstHttpClient->u32HeaderSize, pstHttpClient->u32ContentSize, *_pu32CompleteLen);
    
    return 0;
}

static int http_client_parseDomainName(char* szDomainName, char* szOutIp, int len)  
{  
    memset(szOutIp, 0, len);
    int ret = -1;
    struct addrinfo *answer = NULL;
    struct addrinfo hint;
    bzero(&hint, sizeof(hint));
    struct addrinfo* curr = NULL;
    char ipstr[16];
    memset(ipstr, 0, sizeof(ipstr));
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;
    ret = getaddrinfo(szDomainName, NULL, &hint, &answer);
    CHECK(ret == 0, -1, "error with %#x: %s\n", ret, strerror(errno));
    
    for (curr = answer; curr != NULL; curr = curr->ai_next) 
    {
        memset(ipstr, 0, sizeof(ipstr));
        const char* pRet = inet_ntop(AF_INET, &(((struct sockaddr_in *)(curr->ai_addr))->sin_addr), ipstr, 16);
        CHECK(pRet != NULL, -1, "error with %#x: %s\n", ret, strerror(errno));
        DBG("ipaddress:%s\n", ipstr);
        
        if (strlen(szOutIp) == 0)
        {
            strcpy(szOutIp, ipstr);
        }
    }
    freeaddrinfo(answer); 
    return 0;  
} 

static int http_client_setup_tcp(char* ip, int port)
{
    int ret = -1;
    int Fd = socket(AF_INET, SOCK_STREAM, 0);
    CHECK(Fd > 0, -1, "error with %#x: %s\n", Fd, strerror(errno));

    struct sockaddr_in Addr;
    memset((char *)&Addr, 0, sizeof(struct sockaddr_in));
    Addr.sin_family = AF_INET;
    Addr.sin_port = htons(port);
    Addr.sin_addr.s_addr = inet_addr(ip);

    ret = connect(Fd, (struct sockaddr *)&Addr, sizeof(struct sockaddr));
    CHECK(ret == 0 || close(Fd), -1, "error with %#x: %s\n", ret, strerror(errno));
    
    return Fd;
}

static int http_client_setup_tcp1(char* ip, int port)
{
    int ret = -1;
    int Fd = socket(AF_INET, SOCK_STREAM, 0);
    CHECK(Fd > 0, -1, "error with %#x: %s\n", Fd, strerror(errno));

    struct sockaddr_in Addr;
    memset((char *)&Addr, 0, sizeof(struct sockaddr_in));
    Addr.sin_family = AF_INET;
    Addr.sin_port = htons(port);
    Addr.sin_addr.s_addr = inet_addr(ip);
    
    int flag = fcntl(Fd, F_GETFL);
    CHECK(flag != -1, -1, "failed to get fcntl flag: %s\n", strerror(errno));
    CHECK(fcntl(Fd, F_SETFL, flag | O_NONBLOCK) != -1, -1, "failed to set nonblock: %s\n", strerror(errno));
    
    ret = connect(Fd, (struct sockaddr *)&Addr, sizeof(struct sockaddr));
    //CHECK(ret == 0, -1, "error with %#x: %s\n", ret, strerror(errno));
    if (ret == 0)
    {
        return Fd;
    }
    else if (ret < 0 && errno != EINPROGRESS)
    {
        ERR("connect error with: %s\n", strerror(errno));
        goto _ERR_EXIT;
    }
    
    fd_set readFds;
    fd_set writeFds;
    struct timeval timeout = {2, 0};
    FD_ZERO(&readFds);
    FD_ZERO(&writeFds);
    FD_SET(Fd, &writeFds);
    FD_SET(Fd, &readFds);

    ret = select(Fd+1, &readFds, &writeFds, NULL, &timeout);
    if (ret < 0)
    {
        if (errno != EINTR && errno != EAGAIN)
        {
            ERR("select failed with: %s\n", strerror(errno));
            goto _ERR_EXIT;
        }
        WRN("system interrupt.%s\n", strerror(errno));
        goto _ERR_EXIT;
    }
    else if (ret == 0)
    {
        DBG("time out.\n");
        goto _ERR_EXIT;
    }

    if(FD_ISSET(Fd, &writeFds))
    {
        //可读可写有两种可能，一是连接错误，二是在连接后服务端已有数据传来
        if(FD_ISSET(Fd, &readFds))
        {
            if(connect(Fd, (struct sockaddr *)&Addr, sizeof(struct sockaddr)) != 0)
            {
                int _error=0;
                socklen_t length = sizeof(_error);
                //调用getsockopt来获取并清除sockfd上的错误.
                ret = getsockopt(Fd, SOL_SOCKET, SO_ERROR, &_error, &length);
                CHECK(ret == 0, -1, "error with %#x: %s\n", ret, strerror(errno));

                if (_error != EISCONN)
                {
                    ERR("connect error with: %s\n", strerror(errno));
                    goto _ERR_EXIT;;
                }
            }
        }
        //此时已排除所有错误可能，表明连接成功
        return Fd;
    }
    else
    {
        ERR("connect failed with: %s\n", strerror(errno));
    }
    
_ERR_EXIT:
    close(Fd);
    return -1;
}

static int http_client_GetKeyValue(char* _szBegin, char* _szEnd, char* _szStartKey,
                                   char* _szEndKey, char* _szValue, int _len)
{
    int ret = -1;
    char* szValue;
    char* szReturn;

    memset(_szValue, 0, _len);
    szValue = strstr(_szBegin, _szStartKey);
    //szValue = _SafeFind(_szBegin, _szEnd, _szStartKey);
    if (szValue != NULL)
    {
        szValue += strlen(_szStartKey);
        szReturn = strstr(szValue, _szEndKey);
        //szReturn = _SafeFind(szValue, _szEnd, _szEndKey);
        if (szReturn != NULL)
        {
            CHECK(_len > szReturn - szValue, -1, "buffer is too small[%d bytes]\n", _len);
            memcpy(_szValue, szValue, szReturn - szValue);
            _szValue[szReturn - szValue] = 0;
            ret = 0;
        }
    }
    return ret;
}

static int http_client_send_request(http_client_s* pstHttpClient)
{
    int ret = -1;

    const char* szTemplate =
        "GET %s HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "Accept: */*\r\n"
        "\r\n"
        ;

    char auData[2*1024];
    memset(auData, 0, sizeof(auData));

    sprintf(auData, szTemplate, pstHttpClient->szRequestResource, pstHttpClient->szDomain, pstHttpClient->port);
    DBG("send: \n%s\n", auData);

    ret = select_send(pstHttpClient->hndSocket, auData, strlen(auData));
    CHECK(ret == 0, -1, "Error with %#x\n", ret);

    return 0;
}

static int http_client_parseUrl(http_client_s* pstHttpClient)
{
    int ret = -1;
    pstHttpClient->port = 80;
    memset(pstHttpClient->szDomain, 0, sizeof(pstHttpClient->szDomain));
    memset(pstHttpClient->szRequestResource, 0, sizeof(pstHttpClient->szRequestResource));
    char* find = NULL;
    
    CHECK(!strncmp(pstHttpClient->szUrl, "http://", strlen("http://")), -1, "url is invalid[%s]\n", pstHttpClient->szUrl);
    find = strstr(pstHttpClient->szUrl+strlen("http://"), ":");
    if (find)
    {
        pstHttpClient->port = atoi(find+1);
    }
    //DBG("port: %d\n", port);
    
    ret = http_client_GetKeyValue(pstHttpClient->szUrl, pstHttpClient->szUrl + strlen(pstHttpClient->szUrl), "http://", "/", pstHttpClient->szDomain, sizeof(pstHttpClient->szDomain));
    if (ret < 0)
    {
        memset(pstHttpClient->szDomain, 0, sizeof(pstHttpClient->szDomain));
        memcpy(pstHttpClient->szDomain, pstHttpClient->szUrl+strlen("http://"), strlen(pstHttpClient->szUrl)-strlen("http://"));
        sprintf(pstHttpClient->szRequestResource, "/");
        find = strstr(pstHttpClient->szDomain, ":");
        if (find)
        {
            memset(find, 0, pstHttpClient->szDomain+sizeof(pstHttpClient->szDomain)-find);
        }
        //DBG("szDomain: %s szRequestResource: %s\n", szDomain, pstHttpClient->szRequestResource);
    }
    else if (ret == 0)
    {
        find = strstr(pstHttpClient->szUrl+strlen("http://"), "/");
        if (find)
        {
            memcpy(pstHttpClient->szRequestResource, find, strlen(find));
        }   
        find = strstr(pstHttpClient->szDomain, ":");
        if (find)
        {
            memset(find, 0, pstHttpClient->szDomain+sizeof(pstHttpClient->szDomain)-find);
        }
        //DBG("szDomain: %s szRequestResource: %s\n", szDomain, pstHttpClient->szRequestResource);
    }
    
    DBG("port: %d, szDomain: %s szRequestResource: %s\n", pstHttpClient->port, pstHttpClient->szDomain, pstHttpClient->szRequestResource);
    
    return 0;
}

static void* http_client_proc(void* arg)
{
    int ret = -1;
    http_client_s* pstHttpClient = (http_client_s*)arg;
    
    ret = http_client_parseUrl(pstHttpClient);
    GOTO(ret == 0, pstHttpClient->enStatus = HTTP_STATUS_FINISH_ERROR, _EXIT, "Error with: %#x\n", ret);
    
    ret = http_client_parseDomainName(pstHttpClient->szDomain, pstHttpClient->szIp, sizeof(pstHttpClient->szIp));
    GOTO(ret == 0, pstHttpClient->enStatus = HTTP_STATUS_FINISH_ERROR, _EXIT, "Error with: %#x\n", ret);
    
    pstHttpClient->Fd = http_client_setup_tcp(pstHttpClient->szIp, pstHttpClient->port);
    GOTO(pstHttpClient->Fd > 0, pstHttpClient->enStatus = HTTP_STATUS_FINISH_ERROR, _EXIT, "Error with: %#x\n", pstHttpClient->Fd);
    
    pstHttpClient->cache_size = 32*1024;
    pstHttpClient->hndSocket = select_init(http_complete, pstHttpClient, pstHttpClient->cache_size, pstHttpClient->Fd);
    GOTO(pstHttpClient->hndSocket, pstHttpClient->enStatus = HTTP_STATUS_FINISH_ERROR, _EXIT, "Error with: %#x\n", pstHttpClient->hndSocket);

    ret = select_debug(pstHttpClient->hndSocket, 0);
    GOTO(ret == 0, pstHttpClient->enStatus = HTTP_STATUS_FINISH_ERROR, _EXIT, "Error with: %#x\n", ret);
    
    unsigned int u32ExpectTimeout = 0;
    unsigned int u32Timeout = 0;
    char* data = NULL;
    int len = 0;

    ret = http_client_send_request(pstHttpClient);
    GOTO(ret == 0, pstHttpClient->enStatus = HTTP_STATUS_FINISH_ERROR, _EXIT, "Error with: %#x\n", ret);
    
    while (pstHttpClient->bRunning)
    {
        u32ExpectTimeout = pstHttpClient->timeout;
        u32Timeout = select_rtimeout(pstHttpClient->hndSocket);
        GOTO(u32Timeout < u32ExpectTimeout, pstHttpClient->enStatus = HTTP_STATUS_READ_TIMEOUT, _EXIT, "Read  Timeout %u, expect %u\n", u32Timeout, u32ExpectTimeout);

        u32ExpectTimeout = pstHttpClient->timeout;
        u32Timeout = select_wtimeout(pstHttpClient->hndSocket);
        GOTO(u32Timeout < u32ExpectTimeout, pstHttpClient->enStatus = HTTP_STATUS_WRITE_TIMEOUT, _EXIT, "Write Timeout %u, expect %u\n", u32Timeout, u32ExpectTimeout);

        do
        {
            data = NULL;
            len = 0;
            ret = select_recv_get(pstHttpClient->hndSocket, (void**)&data, &len);
            GOTO(ret == 0, pstHttpClient->enStatus = HTTP_STATUS_FINISH_ERROR, _EXIT, "Error with: %#x\n", ret);
            if (data != NULL)
            {
                GOTO((((unsigned int)data)%4) == 0, pstHttpClient->enStatus = HTTP_STATUS_FINISH_ERROR, _EXIT, "pData %p is not align\n", data);
                //DBG("recv package.size: %d\n", len);
                
                if (pstHttpClient->u32HeaderOffset == 0 && pstHttpClient->pu8Header)
                {
                    memcpy(pstHttpClient->pu8Header+pstHttpClient->u32HeaderOffset, data, len);
                    pstHttpClient->u32HeaderOffset += len;
                }
                else if (pstHttpClient->pu8Content)
                {
                    memcpy(pstHttpClient->pu8Content+pstHttpClient->u32ContentOffset, data, len);
                    pstHttpClient->u32ContentOffset += len;
                    pstHttpClient->progress = pstHttpClient->u32ContentOffset*100/pstHttpClient->u32ContentSize;
                }

                ret = select_recv_release(pstHttpClient->hndSocket, (void**)&data, &len);
                GOTO(ret == 0, pstHttpClient->enStatus = HTTP_STATUS_FINISH_ERROR, _EXIT, "Error with: %#x\n", ret);
                
                if (pstHttpClient->pu8Header && pstHttpClient->pu8Content == NULL)
                {
                    DBG("recv complete content(only inlcude head)\n");
                    DBG("head: \n%s\n", pstHttpClient->pu8Header);
                    pstHttpClient->enStatus = HTTP_STATUS_FINISH_OK;
                    pstHttpClient->progress = 100;
                    goto _EXIT;
                }
                if (pstHttpClient->pu8Content && pstHttpClient->u32ContentOffset == pstHttpClient->u32ContentSize)
                {
                    DBG("recv complete content\n");
                    DBG("head: \n%s\n", pstHttpClient->pu8Header);
                    //printf("content: \n%s\n", pstHttpClient->pu8Content);
                    pstHttpClient->enStatus = HTTP_STATUS_FINISH_OK;
                    pstHttpClient->progress = 100;
                    goto _EXIT;
                }
            }
        }
        while (data != NULL && pstHttpClient->bRunning);

        ret = select_rw(pstHttpClient->hndSocket);
        GOTO(ret == 0, pstHttpClient->enStatus = HTTP_STATUS_FINISH_ERROR, _EXIT, "Error with: %#x\n", ret);
    }
    
_EXIT:
    DBG("http client[%d] ready to exit\n", pstHttpClient->Fd);

    if (pstHttpClient->hndSocket)
    {
        ret = select_destroy(pstHttpClient->hndSocket);
        CHECK(ret == 0, NULL, "Error with %#x\n", ret);
        pstHttpClient->hndSocket = NULL;
    }
    
    pstHttpClient->bRunning = 0;
    DBG("http client[%d] exit done.\n", pstHttpClient->Fd);
    
    if (pstHttpClient->Fd > 0)
    {
        close(pstHttpClient->Fd);
        pstHttpClient->Fd = -1;
    }
    
    return NULL;
}

handle http_client_init(char* szUrl, int timeout)
{
    int s32Ret = -1;
    http_client_s* pstHttpClient = (http_client_s*)mem_malloc(sizeof(http_client_s));
    CHECK(pstHttpClient, NULL, "error with %#x\n", pstHttpClient);
    memset(pstHttpClient, 0, sizeof(http_client_s));
    
    CHECK(strlen(szUrl) < sizeof(pstHttpClient->szUrl), NULL, "invalid url %s\n", szUrl);
    strcpy(pstHttpClient->szUrl, szUrl);
    pstHttpClient->timeout = timeout;
    
    pstHttpClient->enStatus = HTTP_STATUS_PROGRESS;
    pstHttpClient->bRunning = 1;
    s32Ret = pthread_create(&pstHttpClient->tPid, NULL, http_client_proc, pstHttpClient);
    ASSERT(s32Ret == 0, "s32Ret is %d\n", s32Ret);
    
    return pstHttpClient;
}

int http_client_exit(handle hndHttpClient)
{
    CHECK(hndHttpClient, -1, "hndHttpClient is not valid\n");
    
    //int ret = -1;
    http_client_s* pstHttpClient = (http_client_s*)hndHttpClient;
    
    if (pstHttpClient->tPid != 0)
    {
        pstHttpClient->bRunning = 0;
        pthread_join(pstHttpClient->tPid, NULL);
    }
    
    if (pstHttpClient->pu8Header)
    {
        mem_free(pstHttpClient->pu8Header);
        pstHttpClient->pu8Header = NULL;
    }
    if (pstHttpClient->pu8Content)
    {
        mem_free(pstHttpClient->pu8Content);
        pstHttpClient->pu8Content = NULL;
    }
    
    mem_free(pstHttpClient);
    pstHttpClient = NULL;
    return 0;
}

int http_client_Query(handle hndHttpClient, HTTP_STATUS_E* status)
{
    CHECK(hndHttpClient, -1, "hndHttpClient is not valid\n");
    CHECK(status, -1, "invalid parameter %#x\n", status);
    
    http_client_s* pstHttpClient = (http_client_s*)hndHttpClient;
    *status = pstHttpClient->enStatus;

    return 0;
}

int http_client_HeaderGet(handle hndHttpClient, unsigned char** output, int* len)
{
    CHECK(hndHttpClient, -1, "hndHttpClient is not valid\n");
    CHECK(output, -1, "invalid parameter %#x\n", output);
    CHECK(len, -1, "invalid parameter %#x\n", len);

    http_client_s* pstHttpClient = (http_client_s*)hndHttpClient;
    *output = pstHttpClient->pu8Header;
    *len = pstHttpClient->u32HeaderSize;

    return 0;
}

int http_client_ContentGet(handle hndHttpClient, unsigned char** output, int* len)
{
    CHECK(hndHttpClient, -1, "hndHttpClient is not valid\n");
    CHECK(output, -1, "invalid parameter %#x\n", output);
    CHECK(len, -1, "invalid parameter %#x\n", len);

    http_client_s* pstHttpClient = (http_client_s*)hndHttpClient;
    *output = pstHttpClient->pu8Content;
    *len = pstHttpClient->u32ContentSize;

    return 0;
}

int http_client_ProgressGet(handle hndHttpClient, int* progress)
{
    CHECK(hndHttpClient, -1, "hndHttpClient is not valid\n");
    CHECK(progress, -1, "invalid parameter %#x\n", progress);

    http_client_s* pstHttpClient = (http_client_s*)hndHttpClient;
    *progress = pstHttpClient->progress;

    return 0;
}




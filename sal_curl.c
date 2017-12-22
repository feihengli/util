
#include "sal_debug.h"
#include "sal_malloc.h"
#include "sal_util.h"
#include "sal_curl.h"

#include "libcurl/curl/curl.h"

#define CURL_WRAPPER_VALID_MAGIC (0x74246F94)
static int g_bCurlWrapperHttpPostImgShowDebug = 0;
static int g_bCurlWrapperHttpPostShowDebug = 0;
static int g_bCurlWrapperHttpGetShowDebug = 0;

typedef struct curl_wrapper_s
{
    unsigned int u32ValidMagie;

    CURL* pstCurl;
    CURLcode enCurlRet;

    pthread_t tPid;
    int bRunning;
    CURL_RESULT_E enResult;

    char szUrl[256];
    char szUsrPwd[32];

    double d32LastTotal;
    double d32LastNow;
    struct timeval stLastChangeTimev;
    int timeout; //超时时间（毫秒）
    FILE* pFile;

    unsigned char* pu8PostSend;
    unsigned int u32PostSendTotal;
    unsigned int u32PostSendCurr;

    unsigned char au8PostRecv[20*1024];
    unsigned int u32PostRecvTotal;
    unsigned int u32PostRecvCurr;
    unsigned int u32PostRecvHeaderTotal;
    unsigned int u32PostRecvBodyTotal;

    struct curl_httppost *formpost;
    struct curl_httppost *lastptr;
    struct curl_slist *headerlist;

    //新增下载到内存，不写文件
    unsigned char* pu8GetRecv; //malloc 申请
    unsigned int u32Size;      //buffer 大小
    unsigned int u32Offset;    //可写的offset
}
curl_wrapper_s;

static size_t _Onrecv(void* buffer, size_t size, size_t nmemb, curl_wrapper_s* pstCurlWrapper)
{
    DBG("buffer=%p, size=%u, nmemb=%u, pstCurlWrapper=%p\n", buffer, size, nmemb, pstCurlWrapper);

    int s32Ret;
    ASSERT(pstCurlWrapper->pFile != NULL, "pFile is NULL\n");

    s32Ret = fwrite(buffer, size, nmemb, pstCurlWrapper->pFile);
    if(s32Ret == nmemb)
    {
        fflush(pstCurlWrapper->pFile);
        return s32Ret;
    }
    else
    {
        ERR("write failed with: %s\n", strerror(errno));
        pstCurlWrapper->bRunning = 0;
        pstCurlWrapper->enResult = CURL_RESULT_WRITE_ERROR;
        return -1;
    }
    return -1;
}

static size_t _OnRecv2mem(void* buffer, size_t size, size_t nmemb, curl_wrapper_s* pstCurlWrapper)
{
    //DBG("buffer=%p, size=%u, nmemb=%u, pstCurlWrapper=%p\n", buffer, size, nmemb, pstCurlWrapper);

    //int s32Ret = -1;

    //初始化大小
    if (pstCurlWrapper->pu8GetRecv == NULL)
    {
        pstCurlWrapper->pu8GetRecv = mem_malloc(size*nmemb);
        CHECK(pstCurlWrapper->pu8GetRecv, -1, "malloc %d bytes failed.\n", size*nmemb);

        pstCurlWrapper->u32Size = size*nmemb;
        pstCurlWrapper->u32Offset = 0;
        WRN("start size: %d\n", size*nmemb);
    }

    //已经知道总共的大小，那么申请整个文件的大小
    if (pstCurlWrapper->pu8GetRecv && pstCurlWrapper->d32LastTotal > 0
        && pstCurlWrapper->d32LastTotal != pstCurlWrapper->u32Size)
    {
        int new_size = pstCurlWrapper->d32LastTotal;
        pstCurlWrapper->pu8GetRecv = mem_realloc(pstCurlWrapper->pu8GetRecv, new_size);
        CHECK(pstCurlWrapper->pu8GetRecv, -1, "realloc %d bytes failed.\n", new_size);

        pstCurlWrapper->u32Size = new_size;
        //pstCurlWrapper->u32Offset = 0;
        WRN("new size: %d\n", new_size);
    }

    //还不知道总共的大小，只能来一个回调就申请多一点内存去装
    if (pstCurlWrapper->pu8GetRecv
        && (pstCurlWrapper->u32Size-pstCurlWrapper->u32Offset) < (size*nmemb))
    {
        int new_size = pstCurlWrapper->u32Size + (size*nmemb);
        pstCurlWrapper->pu8GetRecv = mem_realloc(pstCurlWrapper->pu8GetRecv, new_size);
        CHECK(pstCurlWrapper->pu8GetRecv, -1, "realloc %d bytes failed.\n", new_size);

        pstCurlWrapper->u32Size = new_size;
        //pstCurlWrapper->u32Offset = 0;
        WRN("new size: %d\n", new_size);
    }

    if (pstCurlWrapper->pu8GetRecv
        && (pstCurlWrapper->u32Size-pstCurlWrapper->u32Offset) >= (size*nmemb))
    {
        memcpy(pstCurlWrapper->pu8GetRecv+pstCurlWrapper->u32Offset, buffer, size*nmemb);
        pstCurlWrapper->u32Offset += size*nmemb;

        return size*nmemb;
    }
    else
    {
        ERR("write failed with: %#x\n", -1);
        pstCurlWrapper->bRunning = 0;
        pstCurlWrapper->enResult = CURL_RESULT_WRITE_ERROR;
        return -1;
    }
    return -1;
}

static size_t _Onsend(void *buffer, size_t size, size_t nmemb, curl_wrapper_s* pstCurlWrapper)
{
    //DBG("buffer=%p, size=%u, nmemb=%u, pstCurlWrapper=%p\n", buffer, size, nmemb, pstCurlWrapper);

    int s32Ret;
    ASSERT(pstCurlWrapper->pFile != NULL, "pFile is NULL\n");

    s32Ret = fread(buffer, size, nmemb, pstCurlWrapper->pFile);
    if(s32Ret >= 0)
    {
        return s32Ret;
    }
    else
    {
        ERR("read failed with: %s\n", strerror(errno));
        pstCurlWrapper->bRunning = 0;
        pstCurlWrapper->enResult = CURL_RESULT_WRITE_ERROR;
        return -1;
    }
    return -1;
}

static size_t _OnDlProgress(curl_wrapper_s* pstCurlWrapper, double dltotal, double dlnow, double ultotal, double ulnow)
{
    //DBG("pstCurlWrapper=%p, dltotal=%d, dlnow=%d, ultotal=%d, ulnow=%d\n", pstCurlWrapper, (int)dltotal, (int)dlnow, (int)ultotal, (int)ulnow);

    if(pstCurlWrapper->bRunning == 0 && dlnow != dltotal)
    {
        WRN("found manaual stop, d32LastTotal=%f, d32LastNow=%f\n",
                        pstCurlWrapper->d32LastTotal, pstCurlWrapper->d32LastNow);
        pstCurlWrapper->enResult = CURL_RESULT_MANUAL_STOP;
        return -1;
    }

    size_t tRet = 0;
    if(pstCurlWrapper->stLastChangeTimev.tv_sec == 0 && pstCurlWrapper->stLastChangeTimev.tv_usec == 0)
    {
         util_time_abs(&pstCurlWrapper->stLastChangeTimev);
    }

    struct timeval stNow = {0, 0};
    util_time_abs(&stNow);
    if(pstCurlWrapper->d32LastNow == dlnow && pstCurlWrapper->d32LastTotal == dltotal)
    {
        int pass_time = util_time_pass(&pstCurlWrapper->stLastChangeTimev);
        if (pass_time > pstCurlWrapper->timeout)
        {
            WRN("DlProgress timeout, d32LastTotal=%f, d32LastNow=%f\n",
                            pstCurlWrapper->d32LastTotal, pstCurlWrapper->d32LastNow);
            pstCurlWrapper->bRunning = 0;
            pstCurlWrapper->enResult = CURL_RESULT_READ_TIMEOUT;
            tRet = -1;
        }
    }
    else
    {
        pstCurlWrapper->d32LastNow = dlnow;
        pstCurlWrapper->d32LastTotal = dltotal;
        pstCurlWrapper->stLastChangeTimev = stNow;
    }

    return tRet;
}

static size_t _OnUlProgress(curl_wrapper_s* pstCurlWrapper, double dltotal, double dlnow, double ultotal, double ulnow)
{
    //DBG("pstCurlWrapper=%p, dltotal=%d, dlnow=%d, ultotal=%d, ulnow=%d\n", pstCurlWrapper, (int)dltotal, (int)dlnow, (int)ultotal, (int)ulnow);

    //todo:why no using this func?
    if(pstCurlWrapper->bRunning == 0 && ulnow != ultotal)
    {
        WRN("found manaual stop, d32LastTotal=%f, d32LastNow=%f\n",
                        pstCurlWrapper->d32LastTotal, pstCurlWrapper->d32LastNow);
        pstCurlWrapper->enResult = CURL_RESULT_MANUAL_STOP;
        return -1;
    }

    size_t tRet = 0;
    if(pstCurlWrapper->stLastChangeTimev.tv_sec == 0 && pstCurlWrapper->stLastChangeTimev.tv_usec == 0)
    {
        util_time_abs(&pstCurlWrapper->stLastChangeTimev);
    }

    struct timeval stNow = {0, 0};
    util_time_abs(&stNow);
    if(pstCurlWrapper->d32LastNow == ulnow && pstCurlWrapper->d32LastTotal == ultotal)
    {
        int pass_time = util_time_pass(&pstCurlWrapper->stLastChangeTimev);
        if (pass_time > 30*1000)
        {
            WRN("DlProgress timeout, d32LastTotal=%f, d32LastNow=%f\n",
                            pstCurlWrapper->d32LastTotal, pstCurlWrapper->d32LastNow);
            pstCurlWrapper->bRunning = 0;
            pstCurlWrapper->enResult = CURL_RESULT_READ_TIMEOUT;
            tRet = -1;
        }
    }
    else
    {
        pstCurlWrapper->d32LastNow = ulnow;
        pstCurlWrapper->d32LastTotal = ultotal;
        pstCurlWrapper->stLastChangeTimev = stNow;
    }

    return tRet;
}

static void* _Proc(void* _pArgs)
{
    prctl(PR_SET_NAME, __FUNCTION__);
    curl_wrapper_s* pstCurlWrapper = (curl_wrapper_s*)_pArgs;

    pstCurlWrapper->enCurlRet = curl_easy_perform(pstCurlWrapper->pstCurl);

    if(pstCurlWrapper->enCurlRet != CURLE_OK)
    {
        pstCurlWrapper->enResult = CURL_RESULT_UNKNOWN;
    }
    else
    {
        pstCurlWrapper->enResult = CURL_RESULT_OK;
    }

    pstCurlWrapper->bRunning = 0;

    return NULL;
}

static size_t _OnHttpPostHead(void* buffer, size_t size, size_t nmemb, curl_wrapper_s* pstCurlWrapper)
{
    //DBG("pstCurlWrapper=%p, size=%d, nmemb=%d, buffer=\"%s\"\n", pstCurlWrapper, size, nmemb, (char*)buffer);

    ASSERT(pstCurlWrapper->u32PostRecvCurr < pstCurlWrapper->u32PostRecvTotal, "u32PostRecvTotal %u, u32PostRecvCurr %u\n", pstCurlWrapper->u32PostRecvTotal, pstCurlWrapper->u32PostRecvCurr);

    size_t s32NeedCopy = size * nmemb;
    unsigned int u32Left = pstCurlWrapper->u32PostRecvTotal - pstCurlWrapper->u32PostRecvCurr;
    CHECK(s32NeedCopy < u32Left, -1, "s32NeedCopy %d, u32Left %u\n", s32NeedCopy, u32Left);

    memcpy(pstCurlWrapper->au8PostRecv + pstCurlWrapper->u32PostRecvCurr, buffer, s32NeedCopy);
    pstCurlWrapper->u32PostRecvCurr += s32NeedCopy;
    pstCurlWrapper->u32PostRecvHeaderTotal += s32NeedCopy;

    return s32NeedCopy;
}

static size_t _OnHttpPostRecv(void* buffer, size_t size, size_t nmemb, curl_wrapper_s* pstCurlWrapper)
{
//  DBG("pstCurlWrapper=%p, size=%d, nmemb=%d\n", pstCurlWrapper, size, nmemb);

    ASSERT(pstCurlWrapper->u32PostRecvCurr < pstCurlWrapper->u32PostRecvTotal, "u32PostRecvTotal %u, u32PostRecvCurr %u\n", pstCurlWrapper->u32PostRecvTotal, pstCurlWrapper->u32PostRecvCurr);

    size_t s32NeedCopy = size * nmemb;
    unsigned int u32Left = pstCurlWrapper->u32PostRecvTotal - pstCurlWrapper->u32PostRecvCurr;
    CHECK(s32NeedCopy < u32Left, -1, "s32NeedCopy %d, u32PostRecvTotal %u, u32PostRecvCurr %u\n", s32NeedCopy, pstCurlWrapper->u32PostRecvTotal, pstCurlWrapper->u32PostRecvCurr);

    memcpy(pstCurlWrapper->au8PostRecv + pstCurlWrapper->u32PostRecvCurr, buffer, s32NeedCopy);
    pstCurlWrapper->u32PostRecvCurr += s32NeedCopy;
    pstCurlWrapper->u32PostRecvBodyTotal += s32NeedCopy;

    return s32NeedCopy;
}

static size_t _OnHttpPostProgress(curl_wrapper_s* pstCurlWrapper, double dltotal, double dlnow, double ultotal, double ulnow)
{
//  DBG("pstCurlWrapper=%p, dltotal=%f, dlnow=%f, ultotal=%f, ulnow=%f\n", pstCurlWrapper, dltotal, dlnow, ultotal, ulnow);
//  DBG("bRunning=%d\n", pstCurlWrapper->bRunning);


    //todo:why no using this func?
    if(pstCurlWrapper->bRunning == 0)
    {
//      ERR("found manaual stop, d32LastTotal=%f, d32LastNow=%f\n",
//              pstCurlWrapper->d32LastTotal, pstCurlWrapper->d32LastNow);
        pstCurlWrapper->enResult = CURL_RESULT_MANUAL_STOP;
        return -1;
    }

    size_t tRet = 0;
    if(pstCurlWrapper->stLastChangeTimev.tv_sec == 0 && pstCurlWrapper->stLastChangeTimev.tv_usec == 0)
    {
        util_time_abs(&pstCurlWrapper->stLastChangeTimev);
    }

    struct timeval stNow = {0, 0};
    util_time_abs(&stNow);
    if(pstCurlWrapper->d32LastNow == ulnow && pstCurlWrapper->d32LastTotal == ultotal)
    {
        int pass_time = util_time_pass(&pstCurlWrapper->stLastChangeTimev);
        if (pass_time > 30*1000)
        {
            ERR("DlProgress timeout, d32LastTotal=%f, d32LastNow=%f\n",
                            pstCurlWrapper->d32LastTotal, pstCurlWrapper->d32LastNow);
            pstCurlWrapper->bRunning = 0;
            pstCurlWrapper->enResult = CURL_RESULT_READ_TIMEOUT;
            tRet = -1;
        }
    }
    else
    {
        pstCurlWrapper->d32LastNow = ulnow;
        pstCurlWrapper->d32LastTotal = ultotal;
        pstCurlWrapper->stLastChangeTimev = stNow;
    }

    return tRet;
}

static int _curl_wrapper_IsValid(curl_wrapper_s* _pstCurlWrapper)
{
    CHECK(_pstCurlWrapper != NULL, 0, "_pstCurlWrapper is NULL\n");
    CHECK(_pstCurlWrapper->u32ValidMagie == CURL_WRAPPER_VALID_MAGIC, 0, "u32ValidMagie is %08x, expect %08x\n", _pstCurlWrapper->u32ValidMagie, CURL_WRAPPER_VALID_MAGIC);
    return 1;
}

handle curl_wrapper_init(int timeout)
{
    static int bInitAll = 0; //todo:see curl_global_cleanup
    if(bInitAll == 1)
    {
        bInitAll = 1;
        curl_global_init(CURL_GLOBAL_DEFAULT);
    }

    curl_wrapper_s* pstCurlWrapper = (curl_wrapper_s*)mem_malloc(sizeof(curl_wrapper_s));
    CHECK(pstCurlWrapper, NULL, "error with %#x\n", pstCurlWrapper);
    memset(pstCurlWrapper, 0, sizeof(curl_wrapper_s));

    pstCurlWrapper->timeout = timeout;
    pstCurlWrapper->u32ValidMagie = CURL_WRAPPER_VALID_MAGIC;
    return pstCurlWrapper;
}

int curl_wrapper_destroy(handle _hndCurlWrapper)
{
    CHECK(_curl_wrapper_IsValid(_hndCurlWrapper), -1, "_hndCurlWrapper is not valid\n");
    curl_wrapper_s* pstCurlWrapper = _hndCurlWrapper;
    pstCurlWrapper->u32ValidMagie = ~CURL_WRAPPER_VALID_MAGIC;

    mem_free(_hndCurlWrapper);

    return 0;
}

int curl_wrapper_StartHttpPostImg(handle _hndCurlWrapper, char* _szUrl, char* _szFilename)
{
    CHECK(_curl_wrapper_IsValid(_hndCurlWrapper), -1, "_hndCurlWrapper is not valid\n");

    int s32Ret;
    curl_wrapper_s* pstCurlWrapper = _hndCurlWrapper;

    //todo:check _szUrl, _pUlData, _u32UlDataSize, _pDlData, _u32DlDataSize

    CHECK(_szUrl != NULL && strlen(_szUrl) < sizeof(pstCurlWrapper->szUrl), -1, "_pUlData is NULL\n");

    strcpy(pstCurlWrapper->szUrl, _szUrl);

    memset(pstCurlWrapper->au8PostRecv, 0, sizeof(pstCurlWrapper->au8PostRecv));
    pstCurlWrapper->u32PostRecvTotal = sizeof(pstCurlWrapper->au8PostRecv);
    pstCurlWrapper->u32PostRecvCurr = 0;
    pstCurlWrapper->u32PostRecvHeaderTotal = 0;
    pstCurlWrapper->u32PostRecvBodyTotal = 0;

    pstCurlWrapper->formpost = NULL;
    pstCurlWrapper->lastptr = NULL;
    pstCurlWrapper->headerlist = NULL;

    curl_formadd(&pstCurlWrapper->formpost,
                 &pstCurlWrapper->lastptr,
                 CURLFORM_COPYNAME, "file1",
                 CURLFORM_FILE, _szFilename,
                 CURLFORM_END);

    /* Fill in the submit field too, even if this is rarely needed */
    curl_formadd(&pstCurlWrapper->formpost,
                 &pstCurlWrapper->lastptr,
                 CURLFORM_COPYNAME, "submit",
                 CURLFORM_COPYCONTENTS, "Submit",
                 CURLFORM_END);

    pstCurlWrapper->pstCurl = curl_easy_init();
    ASSERT(pstCurlWrapper->pstCurl != NULL, "pstCurl is NULL\n");

    pstCurlWrapper->headerlist = curl_slist_append(pstCurlWrapper->headerlist, "Expect:");

    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_URL, pstCurlWrapper->szUrl);
    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_HTTPHEADER, pstCurlWrapper->headerlist);
    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_HTTPPOST, pstCurlWrapper->formpost);

//  curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)pstCurlWrapper->u32PostSendCurr);

    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_WRITEFUNCTION, _OnHttpPostRecv);
    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_WRITEDATA, pstCurlWrapper);

    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_NOPROGRESS, 0);
    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_PROGRESSFUNCTION, _OnHttpPostProgress);
    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_PROGRESSDATA, pstCurlWrapper);

    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_NOSIGNAL, 1L);

    if(g_bCurlWrapperHttpPostImgShowDebug == 1)
    {
        curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_VERBOSE, 1L);
    }

    pstCurlWrapper->bRunning = 1;
    s32Ret = pthread_create(&pstCurlWrapper->tPid, NULL, _Proc, pstCurlWrapper);
    ASSERT(s32Ret == 0, "s32Ret is %d\n", s32Ret);

    return 0;
}

int curl_wrapper_StartHttpPost(handle _hndCurlWrapper, char* _szUrl, unsigned char* _pUlData, unsigned int _u32UlDataSize)
{
    CHECK(_curl_wrapper_IsValid(_hndCurlWrapper), -1, "_hndCurlWrapper is not valid\n");

    int s32Ret;
    curl_wrapper_s* pstCurlWrapper = _hndCurlWrapper;

    //todo:check _szUrl, _pUlData, _u32UlDataSize, _pDlData, _u32DlDataSize

    CHECK(_szUrl != NULL && strlen(_szUrl) < sizeof(pstCurlWrapper->szUrl), -1, "_pUlData is NULL\n");
    CHECK(_pUlData != NULL, -1, "_pUlData is NULL\n");

    strcpy(pstCurlWrapper->szUrl, _szUrl);

    pstCurlWrapper->pu8PostSend = (unsigned char*)mem_malloc(_u32UlDataSize);
    memcpy(pstCurlWrapper->pu8PostSend, _pUlData, _u32UlDataSize);
    pstCurlWrapper->u32PostSendTotal = _u32UlDataSize;
    pstCurlWrapper->u32PostSendCurr = _u32UlDataSize;

    memset(pstCurlWrapper->au8PostRecv, 0, sizeof(pstCurlWrapper->au8PostRecv));
    pstCurlWrapper->u32PostRecvTotal = sizeof(pstCurlWrapper->au8PostRecv);
    pstCurlWrapper->u32PostRecvCurr = 0;
    pstCurlWrapper->u32PostRecvHeaderTotal = 0;
    pstCurlWrapper->u32PostRecvBodyTotal = 0;

    pstCurlWrapper->headerlist = NULL;

    pstCurlWrapper->pstCurl = curl_easy_init();
    ASSERT(pstCurlWrapper->pstCurl != NULL, "pstCurl is NULL\n");

    pstCurlWrapper->headerlist = curl_slist_append(pstCurlWrapper->headerlist, "Expect:");

    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_URL, pstCurlWrapper->szUrl);
    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_HTTPHEADER, pstCurlWrapper->headerlist);
    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_POST, 1L);
//  curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_TIMEOUT, 5);
//  curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_CONNECTTIMEOUT, 120);
    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_POSTFIELDS, pstCurlWrapper->pu8PostSend);
    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_POSTFIELDSIZE, pstCurlWrapper->u32PostSendCurr);
    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)pstCurlWrapper->u32PostSendCurr);

    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_HEADERFUNCTION, _OnHttpPostHead);
    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_HEADERDATA, pstCurlWrapper);

    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_WRITEFUNCTION, _OnHttpPostRecv);
    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_WRITEDATA, pstCurlWrapper);

    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_NOPROGRESS, 0);
    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_PROGRESSFUNCTION, _OnHttpPostProgress);
    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_PROGRESSDATA, pstCurlWrapper);

    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_NOSIGNAL, 1L);

    if(g_bCurlWrapperHttpPostShowDebug == 1)
    {
        curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_VERBOSE, 1L);
    }

    pstCurlWrapper->bRunning = 1;
    s32Ret = pthread_create(&pstCurlWrapper->tPid, NULL, _Proc, pstCurlWrapper);
    ASSERT(s32Ret == 0, "s32Ret is %d\n", s32Ret);

    return 0;
}

int curl_wrapper_StartHttpGet(handle _hndCurlWrapper, char* _szUrl)
{
    CHECK(_curl_wrapper_IsValid(_hndCurlWrapper), -1, "_hndCurlWrapper is not valid\n");

    int s32Ret;
    curl_wrapper_s* pstCurlWrapper = _hndCurlWrapper;

    //todo:check _szUrl, _pUlData, _u32UlDataSize, _pDlData, _u32DlDataSize

    CHECK(_szUrl != NULL && strlen(_szUrl) < sizeof(pstCurlWrapper->szUrl), -1, "_pUlData is NULL\n");

    strcpy(pstCurlWrapper->szUrl, _szUrl);

    memset(pstCurlWrapper->au8PostRecv, 0, sizeof(pstCurlWrapper->au8PostRecv));
    pstCurlWrapper->u32PostRecvTotal = sizeof(pstCurlWrapper->au8PostRecv);
    pstCurlWrapper->u32PostRecvCurr = 0;
    pstCurlWrapper->u32PostRecvHeaderTotal = 0;
    pstCurlWrapper->u32PostRecvBodyTotal = 0;

    pstCurlWrapper->pstCurl = curl_easy_init();
    ASSERT(pstCurlWrapper->pstCurl != NULL, "pstCurl is NULL\n");

    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_URL, pstCurlWrapper->szUrl);
    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_HTTPGET, 1L);
//  curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_TIMEOUT, 5);
//  curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_CONNECTTIMEOUT, 120);

    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_HEADERFUNCTION, _OnHttpPostHead);
    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_HEADERDATA, pstCurlWrapper);

    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_WRITEFUNCTION, _OnHttpPostRecv);
    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_WRITEDATA, pstCurlWrapper);

    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_NOPROGRESS, 0);
    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_PROGRESSFUNCTION, _OnHttpPostProgress);
    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_PROGRESSDATA, pstCurlWrapper);

    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_NOSIGNAL, 1L);

    if(g_bCurlWrapperHttpGetShowDebug == 1)
    {
        curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_VERBOSE, 1L);
    }

    pstCurlWrapper->bRunning = 1;
    s32Ret = pthread_create(&pstCurlWrapper->tPid, NULL, _Proc, pstCurlWrapper);
    ASSERT(s32Ret == 0, "s32Ret is %d\n", s32Ret);

    return 0;
}

int curl_wrapper_StartUpload(handle _hndCurlWrapper, char* _szUrl, char* _szUsr, char* _szPwd, char* _szLocalPath)
{
    CHECK(_curl_wrapper_IsValid(_hndCurlWrapper), -1, "_hndCurlWrapper is not valid\n");

    int s32Ret;
    curl_wrapper_s* pstCurlWrapper = _hndCurlWrapper;

    //todo:check _szUrl, _szUsr, _szPwd, _szLocalPath

    pstCurlWrapper->pFile = fopen(_szLocalPath, "rb");
    CHECK(pstCurlWrapper->pFile != NULL, -1, "failed to fopen[%s] with: %s\n", _szLocalPath, strerror(errno));

    fseek(pstCurlWrapper->pFile, 0, SEEK_END);
    unsigned int u32FileSize = ftell(pstCurlWrapper->pFile);
    fseek(pstCurlWrapper->pFile, 0, SEEK_SET);


    //MA_SAFE_STRCPY(pstCurlWrapper->szUrl, _szUrl);
    strcpy(pstCurlWrapper->szUrl, _szUrl);
    if(strlen(_szUsr) > 0 || strlen(_szPwd) > 0)
    {
        sprintf(pstCurlWrapper->szUsrPwd, "%s:%s", _szUsr, _szPwd);
    }

    pstCurlWrapper->pstCurl = curl_easy_init();
    ASSERT(pstCurlWrapper->pstCurl != NULL, "pstCurl is NULL\n");

    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_URL, pstCurlWrapper->szUrl);
    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_USERPWD, pstCurlWrapper->szUsrPwd);

    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_READFUNCTION, _Onsend);
    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_READDATA, pstCurlWrapper);

    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)u32FileSize);

    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_NOPROGRESS, 0);
    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_PROGRESSFUNCTION, _OnUlProgress);
    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_PROGRESSDATA, pstCurlWrapper);

//  curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_VERBOSE, 1L);

    pstCurlWrapper->bRunning = 1;
    s32Ret = pthread_create(&pstCurlWrapper->tPid, NULL, _Proc, pstCurlWrapper);
    ASSERT(s32Ret == 0, "s32Ret is %d\n", s32Ret);

    return 0;
}

int curl_wrapper_StartDownload(handle _hndCurlWrapper, char* _szUrl, char* _szUsr, char* _szPwd, char* _szLocalPath)
{
    CHECK(_curl_wrapper_IsValid(_hndCurlWrapper), -1, "_hndCurlWrapper is not valid\n");

    int s32Ret;
    curl_wrapper_s* pstCurlWrapper = _hndCurlWrapper;

    //todo:check _szUrl, _szUsr, _szPwd, _szLocalPath

    pstCurlWrapper->pFile = fopen(_szLocalPath, "wb+");
    CHECK(pstCurlWrapper->pFile != NULL, -1, "failed to fopen[%s] with: %s\n", _szLocalPath, strerror(errno));

    strcpy(pstCurlWrapper->szUrl, _szUrl);
    pstCurlWrapper->szUsrPwd[0] = 0;
    if(_szUsr && strlen(_szUsr) > 0 && _szPwd && strlen(_szPwd) > 0)
    {
        sprintf(pstCurlWrapper->szUsrPwd, "%s:%s", _szUsr, _szPwd);
    }

    memset(pstCurlWrapper->au8PostRecv, 0, sizeof(pstCurlWrapper->au8PostRecv));
    pstCurlWrapper->u32PostRecvTotal = sizeof(pstCurlWrapper->au8PostRecv);
    pstCurlWrapper->u32PostRecvCurr = 0;
    pstCurlWrapper->u32PostRecvHeaderTotal = 0;
    pstCurlWrapper->u32PostRecvBodyTotal = 0;

    pstCurlWrapper->pstCurl = curl_easy_init();
    ASSERT(pstCurlWrapper->pstCurl != NULL, "pstCurl is NULL\n");

    DBG("timeout set: %d\n", pstCurlWrapper->timeout);
    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_TIMEOUT, pstCurlWrapper->timeout/1000);
    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_CONNECTTIMEOUT, pstCurlWrapper->timeout/1000);

    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_URL, pstCurlWrapper->szUrl);
    if(strlen(pstCurlWrapper->szUsrPwd) > 0)
    {
        curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_USERPWD, pstCurlWrapper->szUsrPwd);
    }



    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_HEADERFUNCTION, _OnHttpPostHead);
    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_HEADERDATA, pstCurlWrapper);

    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_WRITEFUNCTION, _Onrecv);
    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_WRITEDATA, pstCurlWrapper);

    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_NOPROGRESS, 0);
    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_PROGRESSFUNCTION, _OnDlProgress);
    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_PROGRESSDATA, pstCurlWrapper);

    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_NOSIGNAL, 1L);

//  curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_VERBOSE, 1L);

    pstCurlWrapper->bRunning = 1;
    s32Ret = pthread_create(&pstCurlWrapper->tPid, NULL, _Proc, pstCurlWrapper);
    ASSERT(s32Ret == 0, "s32Ret is %d\n", s32Ret);

    return 0;
}

int curl_wrapper_StartDownload2Mem(handle _hndCurlWrapper, char* _szUrl, char* _szUsr, char* _szPwd)
{
    CHECK(_curl_wrapper_IsValid(_hndCurlWrapper), -1, "_hndCurlWrapper is not valid\n");

    int s32Ret;
    curl_wrapper_s* pstCurlWrapper = _hndCurlWrapper;

    pstCurlWrapper->pu8GetRecv = NULL;
    pstCurlWrapper->u32Size = 0;
    pstCurlWrapper->u32Offset = 0;

    strcpy(pstCurlWrapper->szUrl, _szUrl);
    pstCurlWrapper->szUsrPwd[0] = 0;
    if(_szUsr && strlen(_szUsr) > 0 && _szPwd && strlen(_szPwd) > 0)
    {
        sprintf(pstCurlWrapper->szUsrPwd, "%s:%s", _szUsr, _szPwd);
    }

    memset(pstCurlWrapper->au8PostRecv, 0, sizeof(pstCurlWrapper->au8PostRecv));
    pstCurlWrapper->u32PostRecvTotal = sizeof(pstCurlWrapper->au8PostRecv);
    pstCurlWrapper->u32PostRecvCurr = 0;
    pstCurlWrapper->u32PostRecvHeaderTotal = 0;
    pstCurlWrapper->u32PostRecvBodyTotal = 0;

    pstCurlWrapper->pstCurl = curl_easy_init();
    ASSERT(pstCurlWrapper->pstCurl != NULL, "pstCurl is NULL\n");

    DBG("timeout set: %d\n", pstCurlWrapper->timeout);
    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_TIMEOUT, pstCurlWrapper->timeout/1000);
    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_CONNECTTIMEOUT, pstCurlWrapper->timeout/1000);

    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_URL, pstCurlWrapper->szUrl);
    if (strlen(pstCurlWrapper->szUsrPwd) > 0)
    {
        curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_USERPWD, pstCurlWrapper->szUsrPwd);
    }

    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_HEADERFUNCTION, _OnHttpPostHead);
    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_HEADERDATA, pstCurlWrapper);

    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_WRITEFUNCTION, _OnRecv2mem);
    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_WRITEDATA, pstCurlWrapper);

    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_NOPROGRESS, 0);
    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_PROGRESSFUNCTION, _OnDlProgress);
    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_PROGRESSDATA, pstCurlWrapper);

    curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_NOSIGNAL, 1L);

//  curl_easy_setopt(pstCurlWrapper->pstCurl, CURLOPT_VERBOSE, 1L);

    pstCurlWrapper->bRunning = 1;
    s32Ret = pthread_create(&pstCurlWrapper->tPid, NULL, _Proc, pstCurlWrapper);
    ASSERT(s32Ret == 0, "s32Ret is %d\n", s32Ret);

    return 0;
}

int curl_wrapper_Download2MemGet(handle _hndCurlWrapper, char** output, int* len)
{
    CHECK(_curl_wrapper_IsValid(_hndCurlWrapper), -1, "_hndCurlWrapper is not valid\n");
    CHECK(output, -1, "invalid parameter %#x\n", output);
    CHECK(len, -1, "invalid parameter %#x\n", len);

    //int s32Ret = -1;
    curl_wrapper_s* pstCurlWrapper = _hndCurlWrapper;

    CHECK(pstCurlWrapper->pu8GetRecv && pstCurlWrapper->u32Size > 0, -1, "Error with: %#x\n", -1);

    *output = (char*)pstCurlWrapper->pu8GetRecv;
    *len = pstCurlWrapper->u32Size;

    return 0;
}

int curl_wrapper_GetProgress(handle _hndCurlWrapper, CURL_STATUS_S* _pstStatus)
{
    CHECK(_curl_wrapper_IsValid(_hndCurlWrapper), -1, "_hndCurlWrapper is not valid\n");
    CHECK(_pstStatus != NULL, -1, "_pstStatus is NULL\n");

    curl_wrapper_s* pstCurlWrapper = _hndCurlWrapper;

//  DBG("d32LastTotal=%f, d32LastNow=%f, bRunning=%d, bFoundErr=%d",
//          pstCurlWrapper->d32LastTotal, pstCurlWrapper->d32LastNow,
//          pstCurlWrapper->bRunning, pstCurlWrapper->bFoundErr);
    double d32Progress = 0.0;
    if(((unsigned int)pstCurlWrapper->d32LastTotal) == 0)
    {
        d32Progress = 0;
    }
    else
    {
        d32Progress = pstCurlWrapper->d32LastNow / pstCurlWrapper->d32LastTotal;
    }

    _pstStatus->u32Progress = (unsigned int)((double)100 * d32Progress);
    _pstStatus->bRunning = pstCurlWrapper->bRunning;
    _pstStatus->enResult = pstCurlWrapper->enResult;

    _pstStatus->pu8Recv = pstCurlWrapper->au8PostRecv;
    _pstStatus->u32RecvHeaderTotal = pstCurlWrapper->u32PostRecvHeaderTotal;
    _pstStatus->u32RecvBodyTotal = pstCurlWrapper->u32PostRecvBodyTotal;

    return 0;
}

int curl_wrapper_Stop(handle _hndCurlWrapper)
{
    CHECK(_curl_wrapper_IsValid(_hndCurlWrapper), -1, "_hndCurlWrapper is not valid\n");

    int s32Ret;
    curl_wrapper_s* pstCurlWrapper = _hndCurlWrapper;

    pstCurlWrapper->bRunning = 0;

    if(pstCurlWrapper->pu8PostSend != NULL)
    {
        mem_free(pstCurlWrapper->pu8PostSend);
    }

    if(pstCurlWrapper->pFile != NULL)
    {
        fclose(pstCurlWrapper->pFile);
    }

    s32Ret = pthread_join(pstCurlWrapper->tPid, NULL);
    ASSERT(s32Ret == 0, "s32Ret is %d\n", s32Ret);

    if (pstCurlWrapper->pu8GetRecv != NULL)
    {
        mem_free(pstCurlWrapper->pu8GetRecv);
        pstCurlWrapper->pu8GetRecv = NULL;
    }

    curl_easy_cleanup(pstCurlWrapper->pstCurl);

    if(pstCurlWrapper->formpost != NULL)
    {
        curl_formfree(pstCurlWrapper->formpost);
    }
    if(pstCurlWrapper->headerlist != NULL)
    {
        curl_slist_free_all(pstCurlWrapper->headerlist);
    }

    return 0;
}

int curl_wrapper_GetPostData(handle _hndCurlWrapper, char* _szRetUrl, unsigned char* _au8RetData, unsigned int* _u32RetDataSize)
{
    curl_wrapper_s* pstCurlWrapper = _hndCurlWrapper;
    strcpy(_szRetUrl, pstCurlWrapper->szUrl);
    memcpy(_au8RetData, pstCurlWrapper->pu8PostSend, pstCurlWrapper->u32PostSendCurr);
    *_u32RetDataSize = pstCurlWrapper->u32PostSendCurr;

    return 0;
}







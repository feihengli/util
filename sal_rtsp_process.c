
#include "libdigest/my_interface.h"
#include "sal_rtp.h"
#include "sal_config.h"
#include "sal_frame_pool.h"
#include "sal_select.h"
#include "sal_debug.h"
#include "sal_malloc.h"
#include "sal_util.h"

#include "sal_rtsp_server.h"
#include "sal_rtsp_process.h"

#define GENERAL_RWTIMEOUT (10*1000)
#define RTSP_INTERLEAVED_FRAME_SIZE (4)

#define GOTO(exp, to, fmt...) do\
{\
    if (!(exp))\
    {\
        ERR(fmt);\
        goto to;\
    }\
}while(0)

typedef enum RTSP_SERVER_STATUS_E
{
    RTSP_SERVER_STATUS_RTSP,
    RTSP_SERVER_STATUS_RTP,
    RTSP_SERVER_STATUS_TEARDOWN,
    RTSP_SERVER_STATUS_ERROR,
} RTSP_SERVER_STATUS_E;

typedef enum TRANS_TYPE_E
{
    TRANS_TYPE_TCP,
    TRANS_TYPE_UDP,
} TRANS_TYPE_E;

typedef struct RTSP_SERVER_S
{
    RTSP_SERVER_STATUS_E enStatus;

    handle hndSocket;
    handle hndReader;

    TRANS_TYPE_E enTranType;

    //rtsp
    char szSession[32];
    unsigned int u32SetupCount;
    char szUrl[128];

    unsigned char au8Sps[64];
    unsigned int u32SpsSize;
    unsigned char au8Pps[64];
    unsigned int u32PpsSize;
    unsigned char au8Vps[64];
    unsigned int u32VpsSize;

    //rtp
    unsigned int u32LastAVUniqueIndex;
    unsigned long long u64LastVTimeStamp;
    unsigned long long u64LastATimeStamp;

    unsigned short u16VSeqNum;
    unsigned short u16ASeqNum;

    unsigned int u32VTimeStamp;
    unsigned int u32ATimeStamp;

    // audio and video enable switch
    int bASupport; //由服务端控制是否支持
    int bAEnable;  //由客户端控制是否使能
    int bVSupport; //由服务端控制是否支持
    int bVEnable;  //由客户端控制是否使能
    
    //Authenticate
    int bAuthEnable;
    int enAuthAlgo; //0: Digest 1: Basic
    char username[32];
    char password[32];
    char realm[32];
    char nonce[64];
    
    //send over udp
    char szClientIP[16];
    int client_Vfd[2];
    int client_Vport[2]; //rtp rtcp
    int server_Vfd[2];
    int server_Vport[2]; //rtp rtcp
    int client_Afd[2];
    int client_Aport[2]; //rtp rtcp
    int server_Afd[2];
    int server_Aport[2]; //rtp rtcp
}
RTSP_SERVER_S;

static int _RtspComplete(void* _pData, int _u32DataLen, int* _pu32CompleteLen)
{
    //DBG("_u32DataLen=%u\n", _u32DataLen);
    *_pu32CompleteLen = 0;
    char szLine[4*1024];
    int minSize = (sizeof(szLine) > _u32DataLen) ? _u32DataLen : sizeof(szLine);
    memcpy(szLine, _pData, minSize);
    szLine[sizeof(szLine)-1] = 0;

/*
    if (memcmp(_pData, "OPTIONS", strlen("OPTIONS"))
        && memcmp(_pData, "DESCRIBE", strlen("DESCRIBE"))
        && memcmp(_pData, "SETUP", strlen("SETUP"))
        && memcmp(_pData, "PLAY", strlen("PLAY"))
        && memcmp(_pData, "TEARDOWN", strlen("TEARDOWN")))
    {
        CHECK(0, -1, "Unknown cmd: %s\n", szLine);
    }
*/

    char* szHeaderReturn = strstr(szLine, "\r\n\r\n");
    char* szContentLength;
    int s32ContentLength;
    if (szHeaderReturn != NULL)
    {
        szContentLength = strstr(szLine, "Content-Length: ");
        if (szContentLength == NULL)
        {
            *_pu32CompleteLen = szHeaderReturn - szLine + 4;
        }

        if (szContentLength != NULL)
        {
            s32ContentLength = atoi(szContentLength + strlen("Content-Length: "));
            if (szHeaderReturn - szLine + 4 + s32ContentLength <= _u32DataLen)
            {
                *_pu32CompleteLen = szHeaderReturn - szLine + 4 + s32ContentLength;
            }
        }
    }

    return 0;
}

static int _RtspOrRtcpComplete(void* _pData, int _u32DataLen, int* _pu32CompleteLen, void* cb_param)
{
    //RTSP_SERVER_S* pstRtspServer = (RTSP_SERVER_S*)cb_param;
    //DBG("_u32DataLen=%u\n", _u32DataLen);
    *_pu32CompleteLen = 0;

    unsigned char u8Start;
    unsigned short u16Size;
    memcpy(&u8Start, _pData, sizeof(unsigned char));
    if (u8Start == 0x24) // $
    {
        if (_u32DataLen >= RTSP_INTERLEAVED_FRAME_SIZE)
        {
            memcpy(&u16Size, _pData + 2, sizeof(unsigned short));
            u16Size = ntohs(u16Size);
            if (_u32DataLen >= RTSP_INTERLEAVED_FRAME_SIZE + u16Size)
            {
                *_pu32CompleteLen = RTSP_INTERLEAVED_FRAME_SIZE + u16Size;
            }
        }
    }
    else
    {
        _RtspComplete(_pData, _u32DataLen, _pu32CompleteLen);
    }

    //DBG("_pu32CompleteLen=%u\n", *_pu32CompleteLen);

    return 0;
}

static int _CheckBuf(char* _szFormat, ...)
{
    va_list pArgs;
    va_start(pArgs, _szFormat);
    int s32Ret = vsnprintf(NULL, 0, _szFormat, pArgs);
    va_end(pArgs);
    return s32Ret;
}

static char* _SafeFind(char* _pBegin, char* _pEnd, char* _szKey)
{
    int i = 0, j = 0, k = 0;
    int s32CmpLen = strlen(_szKey);
    int Len = _pEnd - _pBegin;
    CHECK(Len > 0, NULL, "size is invalid[%d]\n", Len);

    for (i = 0; i < Len; i++)
    {
        if (_pBegin[i] == _szKey[j])
        {
            for (j = 0, k = i; j < s32CmpLen; j++, k++)
            {
                if (_pBegin[k] != _szKey[j])
                {
                    j = 0;
                    break;
                }
                if (j == s32CmpLen - 1)
                {
                    return _pBegin + i;
                }
            }
        }
    }

    return NULL;
}

static int _GetKeyValue(char* _szBegin, char* _szEnd, char* _szStartKey,
                        char* _szEndKey, char* _szValue, int _len)
{
    int ret = -1;
    char* szValue;
    char* szReturn;

    memset(_szValue, 0, _len);
    //szValue = strstr(_szBegin, _szStartKey);
    szValue = _SafeFind(_szBegin, _szEnd, _szStartKey);
    if (szValue != NULL)
    {
        szValue += strlen(_szStartKey);
        //szReturn = strstr(szValue, _szEndKey);
        szReturn = _SafeFind(szValue, _szEnd, _szEndKey);
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

static int _GetRtspValue(char* _szRtsp, char* _szKey, char* _szValue)
{
    int ret = -1;
    char* szValue;
    char* szReturn;

    szValue = strstr(_szRtsp, _szKey);
    if (szValue != NULL)
    {
        szValue += strlen(_szKey);
        szReturn = strstr(szValue, "\r\n");
        if (szReturn != NULL)
        {
            memcpy(_szValue, szValue, szReturn - szValue);
            _szValue[szReturn - szValue] = 0;
            ret = 0;
        }
    }
    return ret;
}

static int _GetStreamId(char* _szRtspReq, RTSP_SERVER_S* _pstRtspServer)
{
    int ret = -1;
    char szStreamId[12];
    int s32Id = -1;

    ret = _GetRtspValue(_szRtspReq, "streamid=", szStreamId);
    if (ret == 0)
    {
        if (!strncmp(szStreamId, "0", 1))
        {
            s32Id = 0;
        }
        else if (!strncmp(szStreamId, "1", 1))
        {
            s32Id = 1;
        }
    }
    else
    {
        WRN("not found streamid\n");
    }

    return s32Id;
}

static int __SessionIsValid(char* _szRtspReq, RTSP_SERVER_S* _pstRtspServer)
{
    int ret = -1;
    char szSession[32];

    ret = _GetRtspValue(_szRtspReq, "Session: ", szSession);
    if (ret == 0)
    {
        if (!strcmp(_pstRtspServer->szSession, szSession))
        {
            return 0;
        }
        else
        {
            WRN("Not my session[%s]: %s\n", _pstRtspServer->szSession, szSession);
        }
    }
    else
    {
        WRN("not found session\n");
    }

    return -1;
}

static char* __MakeSPS(unsigned char* buffer, unsigned int size)
{
    int s32Ret;
    char* szRet;

    char* szFormat = "%s";
    char* szBase64 = MY_Base64Encode(buffer, size);

    s32Ret = _CheckBuf(szFormat, szBase64);
    szRet = (char*)mem_malloc(s32Ret + 1);
    sprintf(szRet, szFormat, szBase64);

    free(szBase64);

    return szRet;
}

static char* __MakePPS(unsigned char* buffer, unsigned int size)
{
    int s32Ret;
    char* szRet;

    char* szFormat = "%s";
    char* szBase64 = MY_Base64Encode(buffer, size);

    s32Ret = _CheckBuf(szFormat, szBase64);
    szRet = (char*)mem_malloc(s32Ret + 1);
    sprintf(szRet, szFormat, szBase64);

    free(szBase64);

    return szRet;
}

static char* __MakeVPS(unsigned char* buffer, unsigned int size)
{
    int s32Ret;
    char* szRet;

    char* szFormat = "%s";
    char* szBase64 = MY_Base64Encode(buffer, size);

    s32Ret = _CheckBuf(szFormat, szBase64);
    szRet = (char*)mem_malloc(s32Ret + 1);
    sprintf(szRet, szFormat, szBase64);

    free(szBase64);

    return szRet;
}

static char* __MakePLI(unsigned char* _pu8Sps)
{
    int s32Ret;
    char* szRet;

    unsigned int u32Tmp1 = _pu8Sps[1];
    unsigned int u32Tmp2 = _pu8Sps[2];
    unsigned int u32Tmp3 = _pu8Sps[3];
    char* szFormat = "%02x%02x%02x";

    s32Ret = _CheckBuf(szFormat, u32Tmp1, u32Tmp2, u32Tmp3);
    szRet = (char*)mem_malloc(s32Ret + 1);
    sprintf(szRet, szFormat, u32Tmp1, u32Tmp2, u32Tmp3);

    return szRet;
}

static char* __MakeSdpH264(RTSP_SERVER_S* _pstRtspServer, char* _szIp, unsigned char* _pu8Sps, unsigned int _u32SpsSize
                        , unsigned char* _pu8Pps, unsigned int _u32PpsSize)
{
    char* szRet = NULL;
    int s32Ret = -1;
    char szFormat[1024];
    memset(szFormat, 0, sizeof(szFormat));
    
    if (_pstRtspServer->bVSupport)
    {
        char* szVFormat =
            "v=0\r\n"
            "o=- 0 0 IN IP4 127.0.0.1\r\n"
            "s=No Name\r\n"
            "c=In IP4 %s\r\n"
            "t=0 0\r\n"
            "a=tool:libavformat 56.15.102\r\n"
            "m=video 0 RTP/AVP 96\r\n"
            "a=rtpmap:96 H264/90000\r\n"
            "a=fmtp:96 packetization-mode=1; sprop-parameter-sets=%s,%s; profile-level-id=%s\r\n"
            "a=control:streamid=0\r\n"
            ;
        strcat(szFormat, szVFormat);
    }
    
    if (_pstRtspServer->bASupport)
    {
        char* szAFormat =
            "a=rtpmap:8 pcma/8000/1\r\n" //8: g711a rtppayloadtype.pcma: g711a. 8000: 采样率. 1: 单声道
            "a=fmtp:8 octet-align=1\r\n"
            "a=framerate:25\r\n"
    /*
            "m=audio 0 RTP/AVP 97\r\n"
            "b=AS:12\r\n"
            "a=rtpmap:97 AMR/8000/1\r\n"
            "a=fmtp:97 octet-align=1\r\n"
    */
            "a=control:streamid=1\r\n"
            ;
        strcat(szFormat, szAFormat);
    }
    
    char* szSPS = __MakeSPS(_pu8Sps, _u32SpsSize);
    char* szPPS = __MakePPS(_pu8Pps, _u32PpsSize);
    char* szPLI = __MakePLI(_pu8Sps);

    s32Ret = _CheckBuf(szFormat, _szIp, szSPS, szPPS, szPLI);
    szRet = (char*)mem_malloc(s32Ret + 1);
    sprintf(szRet, szFormat, _szIp, szSPS, szPPS, szPLI);

    mem_free(szPLI);
    mem_free(szPPS);
    mem_free(szSPS);
    return szRet;
}

static unsigned int __RemoveH264or5EmulationBytes(unsigned char* to, unsigned int toMaxSize,unsigned char const* from, unsigned int fromSize) 
{
    unsigned toSize = 0;
    unsigned i = 0;
    while (i < fromSize && toSize+1 < toMaxSize) 
    {
        if (i+2 < fromSize && from[i] == 0 && from[i+1] == 0 && from[i+2] == 3) 
        {
            to[toSize] = to[toSize+1] = 0;
            toSize += 2;
            i += 3;
        } 
        else 
        {
            to[toSize] = from[i];
            toSize += 1;
            i += 1;
        }
    }

    return toSize;
}

static char* __MakeSdpH265(RTSP_SERVER_S* _pstRtspServer, char* _szIp, unsigned char* _pu8Sps, unsigned int _u32SpsSize
                        , unsigned char* _pu8Pps, unsigned int _u32PpsSize
                        , unsigned char* _pu8Vps, unsigned int _u32VpsSize)
{
    char* szRet = NULL;
    int s32Ret = -1;
    char szFormat[1024];
    memset(szFormat, 0, sizeof(szFormat));
    
    if (_pstRtspServer->bVSupport)
    {
        char* szVFormat =
            "v=0\r\n"
            "o=- 0 0 IN IP4 127.0.0.1\r\n"
            "s=No Name\r\n"
            "c=In IP4 %s\r\n"
            "t=0 0\r\n"
            "a=range:npt=0-\r\n"
            "a=tool:libavformat 56.15.102\r\n"
            "m=video 0 RTP/AVP 96\r\n"
            "c=IN IP4 0.0.0.0\r\n"
            "b=AS:500\r\n"
            "a=rtpmap:96 H265/90000\r\n"
            "a=fmtp:96 profile-space=%u"
            ";profile-id=%u"
            ";tier-flag=%u"
            ";level-id=%u"
            ";interop-constraints=%s"
            ";sprop-vps=%s"
            ";sprop-sps=%s"
            ";sprop-pps=%s\r\n"
            "a=control:streamid=0\r\n"
            ;
        strcat(szFormat, szVFormat);
    }
    
    if (_pstRtspServer->bASupport)
    {
        char* szAFormat =
            "m=audio 0 RTP/AVP 8\r\n"
            "a=rtpmap:8 pcma/8000/1\r\n"
            "a=fmtp:8 octet-align=1\r\n"
            "a=framerate:25\r\n"
    /*
            "m=audio 0 RTP/AVP 97\r\n"
            "b=AS:12\r\n"
            "a=rtpmap:97 AMR/8000/1\r\n"
            "a=fmtp:97 octet-align=1\r\n"
    */
            "a=control:streamid=1\r\n"
            ;
        strcat(szFormat, szAFormat);
    }

    // Set up the "a=fmtp:" SDP line for this stream.
    unsigned char* vpsWEB = (unsigned char*)mem_malloc(_u32VpsSize); // "WEB" means "Without Emulation Bytes"
    unsigned int vpsWEBSize = __RemoveH264or5EmulationBytes(vpsWEB, _u32VpsSize, _pu8Vps, _u32VpsSize);
    if (vpsWEBSize < 6/*'profile_tier_level' offset*/ + 12/*num 'profile_tier_level' bytes*/)
    {
        // Bad VPS size => assume our source isn't ready
        mem_free(vpsWEB);
        return NULL;
    }

    unsigned char const* profileTierLevelHeaderBytes = &vpsWEB[6];
    unsigned int profileSpace  = profileTierLevelHeaderBytes[0]>>6; // general_profile_space
    unsigned int profileId = profileTierLevelHeaderBytes[0]&0x1F; // general_profile_idc
    unsigned int tierFlag = (profileTierLevelHeaderBytes[0]>>5)&0x1; // general_tier_flag
    unsigned int levelId = profileTierLevelHeaderBytes[11]; // general_level_idc
    unsigned char const* interop_constraints = &profileTierLevelHeaderBytes[5];
    char interopConstraintsStr[32];
    memset(interopConstraintsStr, 0, sizeof(interopConstraintsStr));
    sprintf(interopConstraintsStr, "%02X%02X%02X%02X%02X%02X", 
        interop_constraints[0], interop_constraints[1], interop_constraints[2],
        interop_constraints[3], interop_constraints[4], interop_constraints[5]);

    char* szVPS = __MakeVPS(_pu8Vps, _u32VpsSize);
    char* szSPS = __MakeSPS(_pu8Sps, _u32SpsSize);
    char* szPPS = __MakePPS(_pu8Pps, _u32PpsSize);

    s32Ret = _CheckBuf(szFormat, _szIp, 
                                profileSpace,
                                profileId,
                                tierFlag,
                                levelId,
                                interopConstraintsStr,
                                szVPS, 
                                szSPS, 
                                szPPS);
    szRet = (char*)mem_malloc(s32Ret + 1);
    sprintf(szRet, szFormat, 
                        _szIp, 
                        profileSpace,
                        profileId,
                        tierFlag,
                        levelId,
                        interopConstraintsStr,
                        szVPS, 
                        szSPS, 
                        szPPS);

    mem_free(szPPS);
    mem_free(szSPS);
    mem_free(szVPS);
    mem_free(vpsWEB);
    return szRet;
}

static int _InitResource(RTSP_SERVER_S* _pstRtspServer, int _bIsMainStream)
{
    int ret = -1;

    if (NULL == _pstRtspServer->hndReader)
    {
        if (_bIsMainStream)
        {
            do
            {
                _pstRtspServer->hndReader = frame_pool_register(gHndMainFramePool, 0);
                usleep(20*1000);
            }
            while (_pstRtspServer->hndReader == NULL);
            //CHECK(_pstRtspServer->hndReader, -1, "Error with: %#x\n", _pstRtspServer->hndReader);
        }
        else
        {
            do
            {
                _pstRtspServer->hndReader = frame_pool_register(gHndSubFramePool, 0);
                usleep(20*1000);
            }
            while (_pstRtspServer->hndReader == NULL);
            //CHECK(_pstRtspServer->hndReader, -1, "Error with: %#x\n", _pstRtspServer->hndReader);
        }

        unsigned char* data = NULL;
        int len = 0;
        ret = frame_pool_sps_get(_pstRtspServer->hndReader, &data, &len);
        CHECK(ret == 0, -1, "Error with: %#x\n", ret);

        CHECK(len < sizeof(_pstRtspServer->au8Sps), -1, "Error with: %#x\n", -1);
        memcpy(_pstRtspServer->au8Sps, data, len);
        _pstRtspServer->u32SpsSize = len;

        ret = frame_pool_pps_get(_pstRtspServer->hndReader, &data, &len);
        CHECK(ret == 0, -1, "Error with: %#x\n", ret);

        CHECK(len < sizeof(_pstRtspServer->au8Pps), -1, "Error with: %#x\n", -1);
        memcpy(_pstRtspServer->au8Pps, data, len);
        _pstRtspServer->u32PpsSize = len;
        
        FRAME_TYPE_E type = FRAME_TYPE_INVALID;
        ret = frame_pool_vframe_type_get(_pstRtspServer->hndReader, &type);
        CHECK(ret == 0, -1, "Error with: %#x\n", ret);
        
        if (type == FRAME_TYPE_H265)
        {
            ret = frame_pool_vps_get(_pstRtspServer->hndReader, &data, &len);
            CHECK(ret == 0, -1, "Error with: %#x\n", ret);

            CHECK(len < sizeof(_pstRtspServer->au8Vps), -1, "Error with: %#x\n", -1);
            memcpy(_pstRtspServer->au8Vps, data, len);
            _pstRtspServer->u32VpsSize = len;
        }
    }

    return 0;
}

static int _SplitName(char* _szRtspUrl, char* _szRet, int _Len)
{
    int ret = -1;
    memset(_szRet, 0, _Len);

    char* Found = rindex(_szRtspUrl, '/');
    if (NULL != Found)
    {
        int NeedLen = _szRtspUrl + strlen(_szRtspUrl) - Found - strlen("/");
        CHECK(_Len > NeedLen, -1, "buffer is too small\n");
        strncpy(_szRet, Found + strlen("/"), NeedLen);
        ret = 0;
    }
    return ret;
}

static int _CheckUrl(char* _szRtspReq, RTSP_SERVER_S* _pstRtspServer, char* _szKey)
{
    int ret = -1;
    char szName[64];
    memset(szName, 0, sizeof(szName));

    ret = _GetKeyValue(_szRtspReq, _szRtspReq + strlen(_szRtspReq), _szKey, " RTSP/", _pstRtspServer->szUrl, sizeof(_pstRtspServer->szUrl));
    CHECK(ret == 0, -1, "url is invalid[%s]\n", _pstRtspServer->szUrl);
    DBG("url: %s\n", _pstRtspServer->szUrl);

    ret = _SplitName(_pstRtspServer->szUrl, szName, sizeof(szName));
    CHECK(ret == 0, -1, "url is invalid[%s]\n", _pstRtspServer->szUrl);

    if (!strcmp(szName, "MainStream"))
    {
        ret = _InitResource(_pstRtspServer, 1);
        CHECK(ret == 0, -1, "_InitResource failed.\n");
    }
    else if (!strcmp(szName, "SubStream"))
    {
        ret = _InitResource(_pstRtspServer, 0);
        CHECK(ret == 0, -1, "_InitResource failed.\n");
    }
    else
    {
        CHECK(0, -1, "url is invalid[%s]\n", _pstRtspServer->szUrl);
    }

    return 0;
}

static int __RecvOptions(char* _szRtspReq, RTSP_SERVER_S* _pstRtspServer)
{
    int ret = -1;
    char szCseq[16];

    ret = _CheckUrl(_szRtspReq, _pstRtspServer, "OPTIONS ");
    CHECK(ret == 0, -1, "_CheckUrl failed.\n");

    ret = _GetRtspValue(_szRtspReq, "CSeq: ", szCseq);
    if (ret != 0)
    {
        ret = _GetRtspValue(_szRtspReq, "Cseq: ", szCseq);
    }
    CHECK(ret == 0, -1, "ret %d, _szRtspReq %s\n", ret, _szRtspReq);

    char* szTemplate =
        "RTSP/1.0 200 OK\r\n"
        "Server: myServer\r\n"
        "CSeq: %s\r\n"
        /*"Public: DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE, OPTIONS, ANNOUNCE, RECORD\r\n"*/
        "Public: DESCRIBE, SETUP, PLAY, OPTIONS, TEARDOWN\r\n"
        "\r\n";

    void* pData;
    unsigned int u32DataLen;

    u32DataLen = _CheckBuf(szTemplate, szCseq);
    pData = mem_malloc(u32DataLen + 1);
    sprintf(pData, szTemplate, szCseq);
    DBG("send: \n%s\n", (char*)pData);

    ret = select_send(_pstRtspServer->hndSocket, pData, u32DataLen);
    CHECK(ret == 0, -1, "Error with %#x\n", ret);

    mem_free(pData);

    return 0;
}

static int __SendUnauthorized(char* _szRtspReq, RTSP_SERVER_S* _pstRtspServer)
{
    int ret = -1;
    char szCseq[16] = {0};

    ret = _CheckUrl(_szRtspReq, _pstRtspServer, "DESCRIBE ");
    CHECK(ret == 0, -1, "_CheckUrl failed.\n");

    ret = _GetRtspValue(_szRtspReq, "CSeq: ", szCseq);
    if (ret != 0)
    {
        ret = _GetRtspValue(_szRtspReq, "Cseq: ", szCseq);
    }
    CHECK(ret == 0, -1, "ret %d, _szRtspReq %s\n", ret, _szRtspReq);
    
    char* szTemplate = NULL;
    void* pData = NULL;
    unsigned int u32DataLen = 0;
    if (_pstRtspServer->enAuthAlgo == 0) //Digest
    {
        szTemplate =
            "RTSP/1.0 401 Unauthorized\r\n"
            "Server: myServer\r\n"
            "Cseq: %s\r\n"
            "WWW-Authenticate: Digest realm=\"%s\", nonce=\"%s\"\r\n"
            "\r\n";

        u32DataLen = _CheckBuf(szTemplate, szCseq, _pstRtspServer->realm, _pstRtspServer->nonce);
        pData = mem_malloc(u32DataLen + 1);
        sprintf(pData, szTemplate, szCseq, _pstRtspServer->realm, _pstRtspServer->nonce);
    }
    else if (_pstRtspServer->enAuthAlgo == 1) //Basic
    {
        szTemplate =
            "RTSP/1.0 401 Unauthorized\r\n"
            "Server: myServer\r\n"
            "Cseq: %s\r\n"
            "WWW-Authenticate: Basic realm=\"/\"\r\n"
            "\r\n";

        u32DataLen = _CheckBuf(szTemplate, szCseq, _pstRtspServer->realm, _pstRtspServer->nonce);
        pData = mem_malloc(u32DataLen + 1);
        sprintf(pData, szTemplate, szCseq, _pstRtspServer->realm, _pstRtspServer->nonce);
    }
    else
    {
        ASSERT(0, "\n");
    }
    DBG("send: \n%s\n", (char*)pData);

    ret = select_send(_pstRtspServer->hndSocket, pData, u32DataLen);
    CHECK(ret == 0, -1, "Error with %#x\n", ret);

    mem_free(pData);

    return 0;
}

static int __Authorize(char* _szRecvResponse, char* _szCmd, RTSP_SERVER_S* _pstRtspServer)
{
    int ret = -1;
    
    if (strstr(_szRecvResponse, "Basic"))
    {
        char szBasic[128];
        memset(szBasic, 0, sizeof(szBasic));
        sprintf(szBasic, "%s:%s", _pstRtspServer->username, _pstRtspServer->password);
        char* szBasicBase64 = MY_Base64Encode((unsigned char*)szBasic, strlen(szBasic));
        memset(szBasic, 0, sizeof(szBasic));
        sprintf(szBasic, "Basic %s", szBasicBase64);
        
        if (!strncmp(_szRecvResponse, szBasic, strlen(szBasic)))
        {
            DBG("Authorization success\n");
            ret = 0;
        }
        else
        {

            WRN("failed to Authorization.\n_szRecvResponse: %s\n_szCalcResponse: %s\n", _szRecvResponse, szBasic);
            ret = -1;
        }
        
        free(szBasicBase64);
    }
    else if (strstr(_szRecvResponse, "Digest"))
    {
        char* szDigest = MY_Authrization(_szCmd, _pstRtspServer->szUrl
            , _pstRtspServer->username, _pstRtspServer->password
            , _pstRtspServer->realm, _pstRtspServer->nonce);
        const char* szTmp = "Authorization: ";//剔除前面"Authorization: "
        memmove(szDigest, szDigest + strlen(szTmp), strlen(szDigest)-strlen(szTmp)+1);

        if (!strncmp(_szRecvResponse, szDigest, strlen(_szRecvResponse)))
        {
            DBG("Authorization success\n");
            ret = 0;
        }
        else
        {

            WRN("failed to Authorization.\n_szRecvResponse: %s\n_szCalcResponse: %s\n", _szRecvResponse, szDigest);
            ret = -1;
        }

        free(szDigest);
    }
    else
    {
        ASSERT(0, "\n");
    }

    return ret;
}

static int __RecvDescribe(char* _szRtspReq, RTSP_SERVER_S* _pstRtspServer)
{
    int ret = -1;
    char szCseq[16];
    memset(szCseq, 0, sizeof(szCseq));
    char Authorization[1024];
    memset(Authorization, 0, sizeof(Authorization));

    ret = _CheckUrl(_szRtspReq, _pstRtspServer, "DESCRIBE ");
    CHECK(ret == 0, -1, "_CheckUrl failed.\n");

    ret = _GetRtspValue(_szRtspReq, "CSeq: ", szCseq);
    if (ret != 0)
    {
        ret = _GetRtspValue(_szRtspReq, "Cseq: ", szCseq);
    }
    CHECK(ret == 0, -1, "ret %d, _szRtspReq %s\n", ret, _szRtspReq);
    
    if (_pstRtspServer->bAuthEnable)
    {
        ret = _GetRtspValue(_szRtspReq, "Authorization: ", Authorization);
        if (ret != 0)
        {
            ret = __SendUnauthorized(_szRtspReq, _pstRtspServer);
            CHECK(ret == 0, -1, "Error with: %#x\n", ret);
            return 0;
        }
        else
        {
            ret = __Authorize(Authorization, "DESCRIBE", _pstRtspServer);
            if (ret == -1)
            {
                ret = __SendUnauthorized(_szRtspReq, _pstRtspServer);
                CHECK(ret == 0, -1, "Error with: %#x\n", ret);
                return 0;
            }
        }
    }
    
    FRAME_TYPE_E type = FRAME_TYPE_INVALID;
    ret = frame_pool_vframe_type_get(_pstRtspServer->hndReader, &type);
    CHECK(ret == 0, -1, "Error with: %#x\n", ret);
    
    char* szSdpContent = NULL;
    if (type == FRAME_TYPE_H264)
    {
        szSdpContent = __MakeSdpH264(_pstRtspServer, "0.0.0.0", _pstRtspServer->au8Sps, _pstRtspServer->u32SpsSize,
            _pstRtspServer->au8Pps, _pstRtspServer->u32PpsSize);
    }
    else if (type == FRAME_TYPE_H265)
    {
        szSdpContent = __MakeSdpH265(_pstRtspServer, "0.0.0.0", _pstRtspServer->au8Sps, _pstRtspServer->u32SpsSize,
            _pstRtspServer->au8Pps, _pstRtspServer->u32PpsSize,
            _pstRtspServer->au8Vps, _pstRtspServer->u32VpsSize);
    }
    unsigned int u32ContentLength = strlen(szSdpContent);

    char* szTemplate =
        "RTSP/1.0 200 OK\r\n"
        "Server: myServer\r\n"
        "Cseq: %s\r\n"
        "Content-length: %u\r\n"
        "Content-Type: application/sdp\r\n"
        "\r\n"
        "%s";

    void* pData;
    unsigned int u32DataLen;

    u32DataLen = _CheckBuf(szTemplate, szCseq, u32ContentLength, szSdpContent);
    pData = mem_malloc(u32DataLen + 1);
    sprintf(pData, szTemplate, szCseq, u32ContentLength, szSdpContent);
    DBG("send: \n%s\n", (char*)pData);

    ret = select_send(_pstRtspServer->hndSocket, pData, u32DataLen);
    CHECK(ret == 0, -1, "Error with %#x\n", ret);

    mem_free(szSdpContent);
    mem_free(pData);

    return 0;
}

static int __Setup_udp(int port)
{
    int ret = -1;
    int Fd = socket(AF_INET, SOCK_DGRAM, 0);
    CHECK(Fd > 0, -1, "error with %#x: %s\n", Fd, strerror(errno));

    int Open = 1;
    ret = setsockopt(Fd, SOL_SOCKET, SO_REUSEADDR, &Open, sizeof(int));
    CHECK(ret == 0, -1, "error with %#x: %s\n", ret, strerror(errno));

    struct sockaddr_in Addr;
    memset((char *)&Addr, 0, sizeof(struct sockaddr_in));
    Addr.sin_family = AF_INET;
    Addr.sin_port = htons(port);
    Addr.sin_addr.s_addr = htonl(INADDR_ANY);//允许连接到所有本地地址上
    ret = bind(Fd, (struct sockaddr *)&Addr, sizeof(struct sockaddr));
    CHECK(ret == 0, -1, "error with %#x: %s\n", ret, strerror(errno));
    
    int flag = fcntl(Fd, F_GETFL);
    CHECK(flag != -1, -1, "failed to get fcntl flag: %s\n", strerror(errno));
    CHECK(fcntl(Fd, F_SETFL, flag | O_NONBLOCK) != -1, -1, "failed to set nonblock: %s\n", strerror(errno));
    DBG("setnoblock success.fd: %d\n", Fd);

    return Fd;
}

static int __RecvSetup(char* _szRtspReq, RTSP_SERVER_S* _pstRtspServer)
{
    int ret = -1;
    char szCseq[16];
    memset(szCseq, 0, sizeof(szCseq));
    char Authorization[1024];
    memset(Authorization, 0, sizeof(Authorization));
    char szTransport[128];
    memset(szTransport, 0, sizeof(szTransport));
    char szRespStatus[32];
    memset(szRespStatus, 0, sizeof(szRespStatus));

    ret = _GetRtspValue(_szRtspReq, "CSeq: ", szCseq);
    if (ret != 0)
    {
        ret = _GetRtspValue(_szRtspReq, "Cseq: ", szCseq);
    }
    CHECK(ret == 0, -1, "ret Cseq %d, _szRtspReq %s\n", ret, _szRtspReq);
    
    if (_pstRtspServer->bAuthEnable)
    {
        ret = _GetRtspValue(_szRtspReq, "Authorization: ", Authorization);
        if (ret != 0)
        {
            ret = __SendUnauthorized(_szRtspReq, _pstRtspServer);
            CHECK(ret == 0, -1, "Error with: %#x\n", ret);
            return 0;
        }
        else
        {
            ret = __Authorize(Authorization, "SETUP", _pstRtspServer);
            if (ret == -1)
            {
                ret = __SendUnauthorized(_szRtspReq, _pstRtspServer);
                CHECK(ret == 0, -1, "Error with: %#x\n", ret);
                return 0;
            }
        }
    }
    
    int s32StreamId = _GetStreamId(_szRtspReq, _pstRtspServer);
    if (0 == s32StreamId)
    {
        _pstRtspServer->bVEnable = 1;
    }
    else if (1 == s32StreamId)
    {
        _pstRtspServer->bAEnable = 1;
    }
    else
    {
        CHECK(0, -1, "Error with s32StreamId %d\n", s32StreamId);
    }
    
    ret = _GetRtspValue(_szRtspReq, "Transport: ", szTransport);
    CHECK(ret == 0, -1, "ret Transport %d, _szRtspReq %s\n", ret, _szRtspReq);

    if (strstr(szTransport, "TCP"))
    {
        _pstRtspServer->enTranType = TRANS_TYPE_TCP;
    }
    // udp
    else if (0 == s32StreamId)
    {
        _pstRtspServer->enTranType = TRANS_TYPE_UDP;
        char* find = strstr(szTransport, "client_port=");
        CHECK(find, -1, "client_Vport is invalid[%s]\n", szTransport);
        _pstRtspServer->client_Vport[0] = atoi(find+strlen("client_port="));
        DBG("client_Vport[0]: %d\n", _pstRtspServer->client_Vport[0]);
        
        char* find1 = strstr(find, "-");
        CHECK(find1, -1, "client_Vport is invalid[%s]\n", find);
        _pstRtspServer->client_Vport[1] = atoi(find1+strlen("-"));
        DBG("client_Vport[1]: %d\n", _pstRtspServer->client_Vport[1]);
        
        _pstRtspServer->client_Vfd[0] = __Setup_udp(_pstRtspServer->client_Vport[0]);
        CHECK(_pstRtspServer->client_Vfd[0] > 0, -1, "fd is invalid[%s]\n", _pstRtspServer->client_Vfd[0]);
        _pstRtspServer->client_Vfd[1] = __Setup_udp(_pstRtspServer->client_Vport[1]);
        CHECK(_pstRtspServer->client_Vfd[1] > 0, -1, "fd is invalid[%s]\n", _pstRtspServer->client_Vfd[1]);
        _pstRtspServer->server_Vfd[0] = __Setup_udp(_pstRtspServer->server_Vport[0]);
        CHECK(_pstRtspServer->server_Vfd[0] > 0, -1, "fd is invalid[%s]\n", _pstRtspServer->server_Vfd[0]);
        _pstRtspServer->server_Vfd[1] = __Setup_udp(_pstRtspServer->server_Vport[1]);
        CHECK(_pstRtspServer->server_Vfd[1] > 0, -1, "fd is invalid[%s]\n", _pstRtspServer->server_Vfd[1]);
    }
    else if (1 == s32StreamId)
    {
        _pstRtspServer->enTranType = TRANS_TYPE_UDP;
        char* find = strstr(szTransport, "client_port=");
        CHECK(find, -1, "client_Vport is invalid[%s]\n", szTransport);
        _pstRtspServer->client_Aport[0] = atoi(find+strlen("client_port="));
        DBG("client_Aport[0]: %d\n", _pstRtspServer->client_Aport[0]);

        char* find1 = strstr(find, "-");
        CHECK(find1, -1, "client_Vport is invalid[%s]\n", find);
        _pstRtspServer->client_Aport[1] = atoi(find1+strlen("-"));
        DBG("client_Aport[1]: %d\n", _pstRtspServer->client_Aport[1]);

        _pstRtspServer->client_Afd[0] = __Setup_udp(_pstRtspServer->client_Aport[0]);
        CHECK(_pstRtspServer->client_Afd[0] > 0, -1, "fd is invalid[%s]\n", _pstRtspServer->client_Afd[0]);
        _pstRtspServer->client_Afd[1] = __Setup_udp(_pstRtspServer->client_Aport[1]);
        CHECK(_pstRtspServer->client_Afd[1] > 0, -1, "fd is invalid[%s]\n", _pstRtspServer->client_Afd[1]);
        _pstRtspServer->server_Afd[0] = __Setup_udp(_pstRtspServer->server_Aport[0]);
        CHECK(_pstRtspServer->server_Afd[0] > 0, -1, "fd is invalid[%s]\n", _pstRtspServer->server_Afd[0]);
        _pstRtspServer->server_Afd[1] = __Setup_udp(_pstRtspServer->server_Aport[1]);
        CHECK(_pstRtspServer->server_Afd[1] > 0, -1, "fd is invalid[%s]\n", _pstRtspServer->server_Afd[1]);
    }

    char* szTemplate = NULL;
    void* pData = NULL;
    unsigned int u32DataLen = 0;

    if (-1 == s32StreamId)
    {
        strcpy(szRespStatus, "406 Not Acceptable");
        _pstRtspServer->enStatus = RTSP_SERVER_STATUS_ERROR;
    }
    else if (TRANS_TYPE_TCP == _pstRtspServer->enTranType)
    {
        szTemplate =
            "RTSP/1.0 %s\r\n"
            "Server: myServer\r\n"
            "Cseq: %s\r\n"
            "Session: %s\r\n"
            "Transport: RTP/AVP/TCP;unicast;interleaved=%u-%u;mode=play\r\n"
            "\r\n";
        strcpy(szRespStatus, "200 OK");
        u32DataLen = _CheckBuf(szTemplate, szRespStatus, szCseq, _pstRtspServer->szSession, _pstRtspServer->u32SetupCount, _pstRtspServer->u32SetupCount + 1);
        pData = mem_malloc(u32DataLen + 1);
        sprintf(pData, szTemplate, szRespStatus, szCseq, _pstRtspServer->szSession, _pstRtspServer->u32SetupCount, _pstRtspServer->u32SetupCount + 1);
        /*
        ** interleaved=0-1 对应RTP OVER TCP 的交错帧头字段channel，0表示RTP 1表示RTCP
        ** 所以每建立一路都是要加2，源端RTP封音视频包的时候也要对应起来
        */
        _pstRtspServer->u32SetupCount += 2;
    }
    else if (TRANS_TYPE_UDP == _pstRtspServer->enTranType && 0 == s32StreamId)
    {
        //strcpy(szRespStatus, "406 Not Acceptable");
        //_pstRtspServer->enStatus = RTSP_SERVER_STATUS_ERROR;
        szTemplate =
            "RTSP/1.0 %s\r\n"
            "Server: myServer\r\n"
            "Cseq: %s\r\n"
            "Session: %s\r\n"
            "Transport: RTP/AVP;unicast;client_port=%d-%d;server_port=%d-%d\r\n"
            "\r\n";
        strcpy(szRespStatus, "200 OK");
        u32DataLen = _CheckBuf(szTemplate, szRespStatus, szCseq, _pstRtspServer->szSession
                                            , _pstRtspServer->client_Vport[0], _pstRtspServer->client_Vport[1]
                                            , _pstRtspServer->server_Vport[0], _pstRtspServer->server_Vport[1]);
        pData = mem_malloc(u32DataLen + 1);
        sprintf(pData, szTemplate, szRespStatus, szCseq, _pstRtspServer->szSession
                                            , _pstRtspServer->client_Vport[0], _pstRtspServer->client_Vport[1]
                                            , _pstRtspServer->server_Vport[0], _pstRtspServer->server_Vport[1]);
    }
    else if (TRANS_TYPE_UDP == _pstRtspServer->enTranType && 1 == s32StreamId)
    {
        //strcpy(szRespStatus, "406 Not Acceptable");
        //_pstRtspServer->enStatus = RTSP_SERVER_STATUS_ERROR;
        szTemplate =
            "RTSP/1.0 %s\r\n"
            "Server: myServer\r\n"
            "Cseq: %s\r\n"
            "Session: %s\r\n"
            "Transport: RTP/AVP;unicast;client_port=%d-%d;server_port=%d-%d\r\n"
            "\r\n";
        strcpy(szRespStatus, "200 OK");
        u32DataLen = _CheckBuf(szTemplate, szRespStatus, szCseq, _pstRtspServer->szSession
            , _pstRtspServer->client_Aport[0], _pstRtspServer->client_Aport[1]
        , _pstRtspServer->server_Aport[0], _pstRtspServer->server_Aport[1]);
        pData = mem_malloc(u32DataLen + 1);
        sprintf(pData, szTemplate, szRespStatus, szCseq, _pstRtspServer->szSession
            , _pstRtspServer->client_Aport[0], _pstRtspServer->client_Aport[1]
        , _pstRtspServer->server_Aport[0], _pstRtspServer->server_Aport[1]);
    }

    DBG("send: \n%s\n", (char*)pData);

    ret = select_send(_pstRtspServer->hndSocket, pData, u32DataLen);
    CHECK(ret == 0, -1, "Error with %#x\n", ret);

    mem_free(pData);

    return 0;
}

static int __RecvPlay(char* _szRtspReq, RTSP_SERVER_S* _pstRtspServer)
{
    int ret = -1;
    char szCseq[16];
    memset(szCseq, 0, sizeof(szCseq));
    char Authorization[1024];
    memset(Authorization, 0, sizeof(Authorization));

    ret = _GetRtspValue(_szRtspReq, "CSeq: ", szCseq);
    if (ret != 0)
    {
        ret = _GetRtspValue(_szRtspReq, "Cseq: ", szCseq);
    }
    CHECK(ret == 0, -1, "ret %d, _szRtspReq %s\n", ret, _szRtspReq);
    
    if (_pstRtspServer->bAuthEnable)
    {
        ret = _GetRtspValue(_szRtspReq, "Authorization: ", Authorization);
        if (ret != 0)
        {
            ret = __SendUnauthorized(_szRtspReq, _pstRtspServer);
            CHECK(ret == 0, -1, "Error with: %#x\n", ret);
            return 0;
        }
        else
        {
            ret = __Authorize(Authorization, "PLAY", _pstRtspServer);
            if (ret == -1)
            {
                ret = __SendUnauthorized(_szRtspReq, _pstRtspServer);
                CHECK(ret == 0, -1, "Error with: %#x\n", ret);
                return 0;
            }
        }
    }
    
    CHECK(!__SessionIsValid(_szRtspReq, _pstRtspServer), -1,
              "session is invalid, _szRtspReq %s\n", _szRtspReq);

    char* szTemplate =
        "RTSP/1.0 200 OK\r\n"
        "Server: myServer\r\n"
        "Cseq: %s\r\n"
        "Session: %s\r\n"
        "Range: npt=now-\r\n"
        /*"RTP-Info: url=rtsp://192.168.34.203:554/269ec94d-fe66-4b0d-b75e-d321f1a90aff.sdp/trackID=0,url=rtsp://192.168.34.203:554/269ec94d-fe66-4b0d-b75e-d321f1a90aff.sdp/trackID=1\r\n"*/
        "\r\n";

    void* pData;
    unsigned int u32DataLen;

    u32DataLen = _CheckBuf(szTemplate, szCseq, _pstRtspServer->szSession);
    pData = mem_malloc(u32DataLen + 1);
    sprintf(pData, szTemplate, szCseq, _pstRtspServer->szSession);
    DBG("send: \n%s\n", (char*)pData);

    _pstRtspServer->enStatus = RTSP_SERVER_STATUS_RTP;

    ret = select_send(_pstRtspServer->hndSocket, pData, u32DataLen);
    CHECK(ret == 0, -1, "Error with %#x\n", ret);

    mem_free(pData);

    return 0;
}

static int __RecvTearDown(char* _szRtspReq, RTSP_SERVER_S* _pstRtspServer)
{
    int ret = -1;
    char szCseq[16];
    memset(szCseq, 0, sizeof(szCseq));
    char Authorization[1024];
    memset(Authorization, 0, sizeof(Authorization));

    ret = _GetRtspValue(_szRtspReq, "CSeq: ", szCseq);
    if (ret != 0)
    {
        ret = _GetRtspValue(_szRtspReq, "Cseq: ", szCseq);
    }
    CHECK(ret == 0, -1, "ret %d, _szRtspReq %s\n", ret, _szRtspReq);
    
    if (_pstRtspServer->bAuthEnable)
    {
        ret = _GetRtspValue(_szRtspReq, "Authorization: ", Authorization);
        if (ret != 0)
        {
            ret = __SendUnauthorized(_szRtspReq, _pstRtspServer);
            CHECK(ret == 0, -1, "Error with: %#x\n", ret);
            return 0;
        }
        else
        {
            ret = __Authorize(Authorization, "TEARDOWN", _pstRtspServer);
            if (ret == -1)
            {
                ret = __SendUnauthorized(_szRtspReq, _pstRtspServer);
                CHECK(ret == 0, -1, "Error with: %#x\n", ret);
                return 0;
            }
        }
    }
    
    CHECK(!__SessionIsValid(_szRtspReq, _pstRtspServer), -1,
              "session is invalid, _szRtspReq %s\n", _szRtspReq);

    char* szTemplate =
        "RTSP/1.0 200 OK\r\n"
        "Server: myServer\r\n"
        "Cseq: %s\r\n"
        "Session: %s\r\n"
        "Range: npt=now-\r\n"
        "Connection: Close\r\n"
        /*"RTP-Info: url=rtsp://192.168.34.203:554/269ec94d-fe66-4b0d-b75e-d321f1a90aff.sdp/trackID=0,url=rtsp://192.168.34.203:554/269ec94d-fe66-4b0d-b75e-d321f1a90aff.sdp/trackID=1\r\n"*/
        "\r\n";

    void* pData;
    unsigned int u32DataLen;

    u32DataLen = _CheckBuf(szTemplate, szCseq, _pstRtspServer->szSession);
    pData = mem_malloc(u32DataLen + 1);
    sprintf(pData, szTemplate, szCseq, _pstRtspServer->szSession);
    DBG("send: \n%s\n", (char*)pData);

    _pstRtspServer->enStatus = RTSP_SERVER_STATUS_TEARDOWN;

    ret = select_send(_pstRtspServer->hndSocket, pData, u32DataLen);
    CHECK(ret == 0, -1, "Error with %#x\n", ret);

    mem_free(pData);

    return 0;
}

static int __SendBadRequest(RTSP_SERVER_S* _pstRtspServer)
{
    int ret = -1;

    _pstRtspServer->enStatus = RTSP_SERVER_STATUS_ERROR;
    char* szTemplate =
        "RTSP/1.0 400 Bad Request\r\n"
        "Server: myServer\r\n"
        "Connection: Close\r\n"
        /*"RTP-Info: url=rtsp://192.168.34.203:554/269ec94d-fe66-4b0d-b75e-d321f1a90aff.sdp/trackID=0,url=rtsp://192.168.34.203:554/269ec94d-fe66-4b0d-b75e-d321f1a90aff.sdp/trackID=1\r\n"*/
        "\r\n";

    void* pData;
    unsigned int u32DataLen;

    u32DataLen = _CheckBuf(szTemplate);
    pData = mem_malloc(u32DataLen + 1);
    sprintf(pData, szTemplate);
    DBG("send: \n%s\n", (char*)pData);

    ret = select_send(_pstRtspServer->hndSocket, pData, u32DataLen);
    CHECK(ret == 0, -1, "Error with %#x\n", ret);

    mem_free(pData);

    return 0;
}

static int __RecvRtcp(char* _szRtspReq, RTSP_SERVER_S* _pstRtspServer)
{
    // todo: parse rtcp
    //DBG("Recv: RTCP packet");

    return 0;
}

static int _RecvRtspReq(char* _szRtspReq, RTSP_SERVER_S* _pstRtspServer)
{
    if (memcmp(_szRtspReq, "$", strlen("$")))
    {
        DBG("recv: \n%s\n", _szRtspReq);
    }

    int ret = -1;
    if (memcmp(_szRtspReq, "OPTIONS", strlen("OPTIONS")) == 0)
    {
        ret = __RecvOptions(_szRtspReq, _pstRtspServer);
    }
    else if (memcmp(_szRtspReq, "DESCRIBE", strlen("DESCRIBE")) == 0)
    {
        ret = __RecvDescribe(_szRtspReq, _pstRtspServer);
    }
    else if (memcmp(_szRtspReq, "SETUP", strlen("SETUP")) == 0)
    {
        ret = __RecvSetup(_szRtspReq, _pstRtspServer);
    }
    else if (memcmp(_szRtspReq, "PLAY", strlen("PLAY")) == 0)
    {
        ret = __RecvPlay(_szRtspReq, _pstRtspServer);
    }
    else if (memcmp(_szRtspReq, "TEARDOWN", strlen("TEARDOWN")) == 0)
    {
        ret = __RecvTearDown(_szRtspReq, _pstRtspServer);
    }
    else if (memcmp(_szRtspReq, "$", strlen("$")) == 0)
    {
        ret = __RecvRtcp(_szRtspReq, _pstRtspServer);
    }
    else
    {
        ret = -1;
        WRN("Unknown cmd: %s\n", _szRtspReq);
    }

    if (ret != 0)
    {
        ret = __SendBadRequest(_pstRtspServer);
        CHECK(ret == 0, -1, "Error with %#x\n", ret);
    }

    return 0;
}

static int _SendAFrameG711A(frame_info_s* _pstInfo, RTSP_SERVER_S* _pstRtspServer)
{
    int ret = -1;
    void* pData;
    unsigned int u32DataLen;

    if (_pstRtspServer->u64LastATimeStamp == 0)
    {
        _pstRtspServer->u64LastATimeStamp = _pstInfo->timestamp;
    }

    _pstRtspServer->u32ATimeStamp = (unsigned int)(_pstInfo->timestamp - _pstRtspServer->u64LastATimeStamp) * 8; //8 8k samplerate

    unsigned char* pu8Frame = _pstInfo->data;
    unsigned int u32FrameSize = _pstInfo->len;
    //DBG("pu8Frame : %02x, len: %u\n", pu8Frame[0], u32FrameSize);

    RTP_SPLIT_S* pstRetARtpSplit = (RTP_SPLIT_S*)mem_malloc(sizeof(RTP_SPLIT_S));
    ret = rtp_g711a_alloc(pu8Frame, u32FrameSize, &_pstRtspServer->u16ASeqNum,
                        _pstRtspServer->u32ATimeStamp, 11, pstRetARtpSplit);
    CHECK(ret == 0, -1, "Error with %#x\n", ret);

    u32DataLen = pstRetARtpSplit->u32BufSize;
    pData = pstRetARtpSplit->pu8Buf;
    //DBG("rtp : u32DataLen: %u\n", u32DataLen);
    
    if (_pstRtspServer->enTranType == TRANS_TYPE_TCP)
    {
        ret = select_send(_pstRtspServer->hndSocket, pData, u32DataLen);
        CHECK(ret == 0, -1, "Error with %#x\n", ret);
    }
    else if (_pstRtspServer->enTranType == TRANS_TYPE_UDP)
    {
        struct sockaddr_in Addr;
        memset((char *)&Addr, 0, sizeof(struct sockaddr_in));
        Addr.sin_family = AF_INET;
        Addr.sin_port = htons(_pstRtspServer->client_Vport[0]);
        Addr.sin_addr.s_addr = inet_addr(_pstRtspServer->szClientIP);
        //DBG("udp addr: %s:%d\n", _pstRtspServer->szClientIP, _pstRtspServer->client_Vport[0]);
        int i = 0;
        for (i = 0; i < pstRetARtpSplit->u32SegmentCount; i++)
        {
            ret = sendto(_pstRtspServer->client_Vfd[0], pstRetARtpSplit->ppu8Segment[i]+RTSP_INTERLEAVED_FRAME_SIZE, pstRetARtpSplit->pU32SegmentSize[i]-RTSP_INTERLEAVED_FRAME_SIZE, 0, (struct sockaddr *)&Addr, sizeof(Addr));
            CHECK((ret == pstRetARtpSplit->pU32SegmentSize[i]-RTSP_INTERLEAVED_FRAME_SIZE) || (ret == -1 && errno == EAGAIN), -1, "Error with %s\n", ret);
            //DBG("sendto ok. size: %d\n", pstRetVRtpSplit->pU32SegmentSize[i]-4);
        }
    }

    ret = rtp_free(pstRetARtpSplit);
    CHECK(ret == 0, -1, "Error with %#x\n", ret);

    mem_free(pstRetARtpSplit);

    return 0;
}

static int _SendVFrameH264(frame_info_s* _pstInfo, RTSP_SERVER_S* _pstRtspServer)
{
    int ret = -1;
    void* pData;
    unsigned int u32DataLen;

    if (_pstRtspServer->u64LastVTimeStamp == 0)
    {
        _pstRtspServer->u64LastVTimeStamp = _pstInfo->timestamp;
    }

    _pstRtspServer->u32VTimeStamp = (unsigned int)(_pstInfo->timestamp - _pstRtspServer->u64LastVTimeStamp) * 90;

    unsigned char* pu8Frame;
    unsigned int u32FrameSize;
    unsigned char* pu8Tmp = _pstInfo->data;
    unsigned int u32TmpSize = _pstInfo->len;
    int offset;
    do
    {
        offset = rtp_vframe_split(pu8Tmp, u32TmpSize);
        if (offset != -1)
        {
            //DBG("offset=%d", offset);
            pu8Frame = pu8Tmp;
            u32FrameSize = offset;

            pu8Tmp += offset;
            u32TmpSize -= offset;
        }
        else
        {
            //DBG("u32TmpSize=%u", u32TmpSize);
            pu8Frame = pu8Tmp;
            u32FrameSize = u32TmpSize;
        }
        //DBG("u32VTimeStamp: %d, u16VSeqNum: %d\n", _pstRtspServer->u32VTimeStamp, _pstRtspServer->u16VSeqNum);
        RTP_SPLIT_S* pstRetVRtpSplit = (RTP_SPLIT_S*)mem_malloc(sizeof(RTP_SPLIT_S));
        ret = rtp_h264_alloc(pu8Frame, u32FrameSize, &_pstRtspServer->u16VSeqNum,
                            _pstRtspServer->u32VTimeStamp, pstRetVRtpSplit);
        CHECK(ret == 0, -1, "Error with %#x\n", ret);

        u32DataLen = pstRetVRtpSplit->u32BufSize;
        pData = pstRetVRtpSplit->pu8Buf;
        
        if (_pstRtspServer->enTranType == TRANS_TYPE_TCP)
        {
            ret = select_send(_pstRtspServer->hndSocket, pData, u32DataLen);
            CHECK(ret == 0, -1, "Error with %#x\n", ret);
        }
        else if (_pstRtspServer->enTranType == TRANS_TYPE_UDP)
        {
            struct sockaddr_in Addr;
            memset((char *)&Addr, 0, sizeof(struct sockaddr_in));
            Addr.sin_family = AF_INET;
            Addr.sin_port = htons(_pstRtspServer->client_Vport[0]);
            Addr.sin_addr.s_addr = inet_addr(_pstRtspServer->szClientIP);
            //DBG("udp addr: %s:%d\n", _pstRtspServer->szClientIP, _pstRtspServer->client_Vport[0]);
            int i = 0;
            for (i = 0; i < pstRetVRtpSplit->u32SegmentCount; i++)
            {
                ret = sendto(_pstRtspServer->client_Vfd[0], pstRetVRtpSplit->ppu8Segment[i]+RTSP_INTERLEAVED_FRAME_SIZE, pstRetVRtpSplit->pU32SegmentSize[i]-RTSP_INTERLEAVED_FRAME_SIZE, 0, (struct sockaddr *)&Addr, sizeof(Addr));
                CHECK((ret == pstRetVRtpSplit->pU32SegmentSize[i]-RTSP_INTERLEAVED_FRAME_SIZE) || (ret == -1 && errno == EAGAIN), -1, "Error with %s\n", ret);
                //DBG("sendto ok. size: %d\n", pstRetVRtpSplit->pU32SegmentSize[i]-4);
            }
        }

        ret = rtp_free(pstRetVRtpSplit);
        CHECK(ret == 0, -1, "Error with %#x\n", ret);

        mem_free(pstRetVRtpSplit);
    }
    while (offset != -1);

    return 0;
}

static int _SendVFrameH265(frame_info_s* _pstInfo, RTSP_SERVER_S* _pstRtspServer)
{
    int ret = -1;
    void* pData;
    unsigned int u32DataLen;

    if (_pstRtspServer->u64LastVTimeStamp == 0)
    {
        _pstRtspServer->u64LastVTimeStamp = _pstInfo->timestamp;
    }

    _pstRtspServer->u32VTimeStamp = (unsigned int)(_pstInfo->timestamp - _pstRtspServer->u64LastVTimeStamp) * 90;
    unsigned char* pu8Frame;
    unsigned int u32FrameSize;
    unsigned char* pu8Tmp = _pstInfo->data;
    unsigned int u32TmpSize = _pstInfo->len;
    int offset;
    do
    {
        offset = rtp_vframe_split(pu8Tmp, u32TmpSize);
        if (offset != -1)
        {
            //DBG("offset=%d", offset);
            pu8Frame = pu8Tmp;
            u32FrameSize = offset;

            pu8Tmp += offset;
            u32TmpSize -= offset;
        }
        else
        {
            //DBG("u32TmpSize=%u", u32TmpSize);
            pu8Frame = pu8Tmp;
            u32FrameSize = u32TmpSize;
        }
        //DBG("u32VTimeStamp: %d, u16VSeqNum: %d\n", _pstRtspServer->u32VTimeStamp, _pstRtspServer->u16VSeqNum);
        RTP_SPLIT_S* pstRetVRtpSplit = (RTP_SPLIT_S*)mem_malloc(sizeof(RTP_SPLIT_S));
        ret = rtp_h265_alloc(pu8Frame, u32FrameSize, &_pstRtspServer->u16VSeqNum,
            _pstRtspServer->u32VTimeStamp, pstRetVRtpSplit);
        CHECK(ret == 0, -1, "Error with %#x\n", ret);

        u32DataLen = pstRetVRtpSplit->u32BufSize;
        pData = pstRetVRtpSplit->pu8Buf;
        
        if (_pstRtspServer->enTranType == TRANS_TYPE_TCP)
        {
            ret = select_send(_pstRtspServer->hndSocket, pData, u32DataLen);
            CHECK(ret == 0, -1, "Error with %#x\n", ret);
        }
        else if (_pstRtspServer->enTranType == TRANS_TYPE_UDP)
        {
            struct sockaddr_in Addr;
            memset((char *)&Addr, 0, sizeof(struct sockaddr_in));
            Addr.sin_family = AF_INET;
            Addr.sin_port = htons(_pstRtspServer->client_Vport[0]);
            Addr.sin_addr.s_addr = inet_addr(_pstRtspServer->szClientIP);
            //DBG("udp addr: %s:%d\n", _pstRtspServer->szClientIP, _pstRtspServer->client_Vport[0]);
            int i = 0;
            for (i = 0; i < pstRetVRtpSplit->u32SegmentCount; i++)
            {
                ret = sendto(_pstRtspServer->client_Vfd[0], pstRetVRtpSplit->ppu8Segment[i]+RTSP_INTERLEAVED_FRAME_SIZE, pstRetVRtpSplit->pU32SegmentSize[i]-RTSP_INTERLEAVED_FRAME_SIZE, 0, (struct sockaddr *)&Addr, sizeof(Addr));
                CHECK((ret == pstRetVRtpSplit->pU32SegmentSize[i]-RTSP_INTERLEAVED_FRAME_SIZE) || (ret == -1 && errno == EAGAIN), -1, "Error with %s\n", ret);
                //DBG("sendto ok. size: %d\n", pstRetVRtpSplit->pU32SegmentSize[i]-4);
            }
        }

        ret = rtp_free(pstRetVRtpSplit);
        CHECK(ret == 0, -1, "Error with %#x\n", ret);

        mem_free(pstRetVRtpSplit);
    }
    while (offset != -1);

    return 0;
}

static int _RecvRtcpOverUdp(RTSP_SERVER_S* _pstRtspServer)
{
    int ret = -1;

    fd_set rset;
    FD_ZERO(&rset);
    int maxfd = 0;

    FD_SET(_pstRtspServer->server_Vfd[1], &rset);
    maxfd = (maxfd < _pstRtspServer->server_Vfd[1]) ? _pstRtspServer->server_Vfd[1] : maxfd;
    FD_SET(_pstRtspServer->server_Afd[1], &rset);
    maxfd = (maxfd < _pstRtspServer->server_Afd[1]) ? _pstRtspServer->server_Afd[1] : maxfd;
    //DBG("maxfd: %d\n", maxfd);
    
    struct timeval TimeoutVal = {0, 1};
    ret = select(maxfd + 1, &rset, NULL, NULL, &TimeoutVal);
    if (ret < 0)
    {
        CHECK(errno == EINTR || errno == EAGAIN, -1, "select failed with: %s\n", strerror(errno));
        return 0;
    }
    else if (ret == 0)
    {
        //DBG("time out.\n");
        return 0;
    }
    struct sockaddr_in Addr;
    memset((char *)&Addr, 0, sizeof(struct sockaddr_in));
    unsigned char buffer[2048];
    memset(buffer, 0, sizeof(buffer));
    socklen_t len = sizeof(Addr);
    if (FD_ISSET(_pstRtspServer->server_Vfd[1], &rset))
    {
        ret = recvfrom(_pstRtspServer->server_Vfd[1], buffer, sizeof(buffer), 0, (struct sockaddr *)&Addr, &len); 
        if (ret < 0)
        {
            CHECK(errno == EINTR || errno == EAGAIN, -1, "select failed with: %s\n", strerror(errno));
            return 0;
        }
        else if (ret == 0)
        {
            DBG("remote closed. ret: %d\n", ret);
            return -1;
        }
        //DBG("recv video rtcp from: %s:%d\n", inet_ntoa(Addr.sin_addr), ntohs(Addr.sin_port));
        //to do
    }
    else if (FD_ISSET(_pstRtspServer->server_Afd[1], &rset))
    {
        ret = recvfrom(_pstRtspServer->server_Afd[1], buffer, sizeof(buffer), 0, (struct sockaddr *)&Addr, &len); 
        if (ret < 0)
        {
            CHECK(errno == EINTR || errno == EAGAIN, -1, "select failed with: %s\n", strerror(errno));
            return 0;
        }
        else if (ret == 0)
        {
            DBG("remote closed. ret: %d\n", ret);
            return -1;
        }
        //DBG("recv audio rtcp from: %s:%d\n", inet_ntoa(Addr.sin_addr), ntohs(Addr.sin_port));
        //to do
    }
    
    return 0;
}

void* rtsp_process(void* _pstSession)
{
    prctl(PR_SET_NAME, __FUNCTION__);
    int ret = -1;
    client_s* pclient = (client_s*)_pstSession;

    struct timeval abstime = {0, 0};
    util_time_abs(&abstime);
    srand(abstime.tv_usec);

    unsigned int i = 0;
    RTSP_SERVER_S stRtspServer;
    memset(&stRtspServer, 0, sizeof(RTSP_SERVER_S));

    stRtspServer.enStatus = RTSP_SERVER_STATUS_RTSP;
    for(i = 0; i < 16; i++)
    {
        sprintf(stRtspServer.szSession + strlen(stRtspServer.szSession), "%d", rand() % 10);
    }

    unsigned int u32ExpectTimeout = 0;
    unsigned int u32Timeout = 0;

    stRtspServer.hndSocket = select_init(_RtspOrRtcpComplete, &stRtspServer, 4*1024, pclient->fd);
    CHECK(stRtspServer.hndSocket, NULL, "Error with: %#x\n", stRtspServer.hndSocket);

    ret = select_debug(stRtspServer.hndSocket, 0);
    CHECK(ret == 0, NULL, "Error with: %#x\n", ret);
    
    stRtspServer.bAuthEnable = 0;
    stRtspServer.enAuthAlgo = 0; //0: Digest 1: Basic
    strcpy(stRtspServer.username, "admin");
    strcpy(stRtspServer.password, "admin");
    strcpy(stRtspServer.realm, "LIVE555 Streaming Media");
    for(i = 0; i < 16; i++)
    {
        sprintf(stRtspServer.nonce + strlen(stRtspServer.nonce), "%02x", rand() % 255);
    }
    
    //used for udp transfer
    strcpy(stRtspServer.szClientIP, pclient->szIP);
    int base_port = 60000 + (rand()%5000);
    stRtspServer.server_Vport[0] = base_port++;
    stRtspServer.server_Vport[1] = base_port++;
    stRtspServer.server_Aport[0] = base_port++;
    stRtspServer.server_Aport[1] = base_port++;
    
    stRtspServer.bVSupport = 1;
    stRtspServer.bASupport = 0;
    
    char* data = NULL;
    int len = 0;
    while (pclient->running)
    {
        if (stRtspServer.enStatus == RTSP_SERVER_STATUS_RTSP || stRtspServer.enStatus == RTSP_SERVER_STATUS_TEARDOWN)
        {
            u32ExpectTimeout = GENERAL_RWTIMEOUT;
            u32Timeout = select_rtimeout(stRtspServer.hndSocket);
            GOTO(u32Timeout < u32ExpectTimeout, _EXIT, "Read  Timeout %u, expect %u\n", u32Timeout, u32ExpectTimeout);

            u32ExpectTimeout = GENERAL_RWTIMEOUT;
            u32Timeout = select_wtimeout(stRtspServer.hndSocket);
            GOTO(u32Timeout < u32ExpectTimeout, _EXIT, "Write Timeout %u, expect %u\n", u32Timeout, u32ExpectTimeout);

            do
            {
                data = NULL;
                len = 0;
                ret = select_recv_get(stRtspServer.hndSocket, (void**)&data, &len);
                GOTO(ret == 0, _EXIT, "Error with: %#x\n", ret);
                if (data != NULL)
                {
                    GOTO((((unsigned int)data)%4) == 0, _EXIT, "pData %p is not align\n", data);

                    char szRtspReq[1024];
                    GOTO(len < sizeof(szRtspReq), _EXIT, "u32DataLen %u is too small, expect %u\n", len, sizeof(szRtspReq));
                    memcpy(szRtspReq, data, len);
                    szRtspReq[len - 1] = 0;
                    ret = _RecvRtspReq(szRtspReq, &stRtspServer);
                    GOTO(ret == 0, _EXIT, "Error with: %#x\n", ret);

                    ret = select_recv_release(stRtspServer.hndSocket, (void**)&data, &len);
                    GOTO(ret == 0, _EXIT, "Error with: %#x\n", ret);
                }
            }
            while (data != NULL && pclient->running);

            ret = select_rw(stRtspServer.hndSocket);
            GOTO(ret == 0, _EXIT, "Error with: %#x\n", ret);
        }
        else if (stRtspServer.enStatus == RTSP_SERVER_STATUS_RTP)
        {
            if (stRtspServer.enTranType == TRANS_TYPE_TCP)
            {
                u32ExpectTimeout = GENERAL_RWTIMEOUT;
                u32Timeout = select_wtimeout(stRtspServer.hndSocket);
                GOTO(u32Timeout < u32ExpectTimeout, _EXIT, "Write Timeout %u, expect %u\n", u32Timeout, u32ExpectTimeout);
            }
            else if (stRtspServer.enTranType == TRANS_TYPE_UDP)
            {
                ret = _RecvRtcpOverUdp(&stRtspServer);
                GOTO(ret == 0, _EXIT, "Error with: %#x\n", ret);
            }

            do
            {
                data = NULL;
                len = 0;
                ret = select_recv_get(stRtspServer.hndSocket, (void**)&data, &len);
                GOTO(ret == 0, _EXIT, "Error with: %#x\n", ret);
                if (data != NULL)
                {
                    GOTO((((unsigned int)data)%4) == 0, _EXIT, "pData %p is not align\n", data);

                    char szRtspReq[1024];
                    GOTO(len < sizeof(szRtspReq), _EXIT, "u32DataLen %u is too small, expect %u\n", len, sizeof(szRtspReq));
                    memcpy(szRtspReq, data, len);
                    szRtspReq[len - 1] = 0;
                    ret = _RecvRtspReq(szRtspReq, &stRtspServer);
                    GOTO(ret == 0, _EXIT, "Error with: %#x\n", ret);

                    ret = select_recv_release(stRtspServer.hndSocket, (void**)&data, &len);
                    GOTO(ret == 0, _EXIT, "Error with: %#x\n", ret);
                }
            }
            while (data != NULL && pclient->running);

            frame_info_s* frame = frame_pool_get(stRtspServer.hndReader);
            if (frame)
            {
                if (stRtspServer.bVEnable && frame->type == FRAME_TYPE_H264)
                {
                    ret = _SendVFrameH264(frame, &stRtspServer);
                    GOTO(ret == 0, _EXIT, "Error with: %#x\n", ret);
                }
                else if (stRtspServer.bVEnable &&  frame->type == FRAME_TYPE_H265)
                {
                    ret = _SendVFrameH265(frame, &stRtspServer);
                    GOTO(ret == 0, _EXIT, "Error with: %#x\n", ret);
                }
                else if (stRtspServer.bAEnable && frame->type == FRAME_TYPE_G711A)
                {
                    ret = _SendAFrameG711A(frame, &stRtspServer);
                    GOTO(ret == 0, _EXIT, "Error with: %#x\n", ret);
                }

                ret = frame_pool_release(stRtspServer.hndReader, frame);
                GOTO(ret == 0, _EXIT, "Error with: %#x\n", ret);
            }
            else
            {
                //WRN("Get AVframe failed\n");
                usleep(20*1000);
            }

            ret = select_rw(stRtspServer.hndSocket);
            GOTO(ret == 0, _EXIT, "Error with: %#x\n", ret);
        }
        else if (RTSP_SERVER_STATUS_ERROR == stRtspServer.enStatus)
        {
            WRN("EXIT: RTSP ERROR\n");
            while (pclient->running && !select_wlist_empty(stRtspServer.hndSocket))
            {
                ret = select_rw(stRtspServer.hndSocket);
                GOTO(ret == 0, _EXIT, "Error with: %#x\n", ret);
            }
            break;
        }
    }

_EXIT:
    DBG("rtsp client[%d] ready to exit\n", pclient->fd);
    if (stRtspServer.hndReader)
    {
        ret = frame_pool_unregister(stRtspServer.hndReader);
        CHECK(ret == 0, NULL, "Error with %#x\n", ret);
        stRtspServer.hndReader = NULL;
    }

    if (stRtspServer.hndSocket)
    {
        ret = select_destroy(stRtspServer.hndSocket);
        CHECK(ret == 0, NULL, "Error with %#x\n", ret);
        stRtspServer.hndSocket = NULL;
    }
    if (stRtspServer.enTranType == TRANS_TYPE_UDP)
    {
        int idx = 0;
        for (idx = 0; idx < 2; idx++)
        {
            if (stRtspServer.client_Vfd[idx] > 0)
            {
                close(stRtspServer.client_Vfd[idx]);
            }
            if (stRtspServer.server_Vfd[idx] > 0)
            {
                close(stRtspServer.server_Vfd[idx]);
            }
            if (stRtspServer.client_Afd[idx] > 0)
            {
                close(stRtspServer.client_Afd[idx]);
            }
            if (stRtspServer.server_Afd[idx] > 0)
            {
                close(stRtspServer.server_Afd[idx]);
            }
        }
    }

    pclient->running = 0;
    DBG("rtsp client[%d] exit done.\n", pclient->fd);

    return NULL;
}







#include "md5/md5.h"
#include "sal_md5.h"
#include "sal_debug.h"
#include "sal_util.h"


int md5_filesum(char* _szFilePath, unsigned char sum[64])
{
    unsigned char buf[1024 +1];
    int n;

    buf[1024] = '\0';

    FILE* pFile = fopen(_szFilePath, "r");
    CHECK(pFile, -1, "failed to fopen[%s] with: %s\n", _szFilePath, strerror(errno));

    MD5_CTX ctx;
    MD5Init(&ctx);
    int s32CheckingSize;
    int s32CheckedSize = 0;
    int _s32MaxSize = util_file_size(_szFilePath);
    while (s32CheckedSize < _s32MaxSize)
    {
        s32CheckingSize = (_s32MaxSize - s32CheckedSize) > (sizeof(buf) - 1) ? (sizeof(buf) - 1) : (_s32MaxSize - s32CheckedSize);
        n = fread(buf, 1, s32CheckingSize, pFile);
        if(n == s32CheckingSize)
        {
            buf[n] = '\0';
            MD5Update(&ctx, buf, n);
            s32CheckedSize += n;
        }
        else
        {
            break;
        }
    }

    fclose(pFile);

    unsigned char digest[16];
    memset(digest, 0, sizeof(digest));
    MD5Final(digest, &ctx);

    char szTmp[128] = {0};
    memset(szTmp, 0, sizeof(szTmp));
    int i = 0;
    for (i = 0; i < 16; i++)
    {
        sprintf(szTmp+strlen(szTmp), "%02x", digest[i]);
    }
    memcpy(sum, szTmp, strlen(szTmp));

    return 0;
}

int md5_memsum(char* input, int len, unsigned char sum[64])
{
    unsigned char buf[1024 +1];
    int n = 0;

    buf[1024] = '\0';

    MD5_CTX ctx;
    MD5Init(&ctx);
    int s32CheckingSize;
    int s32CheckedSize = 0;
    int _s32MaxSize = len;
    while (s32CheckedSize < _s32MaxSize)
    {
        s32CheckingSize = (_s32MaxSize - s32CheckedSize) > (sizeof(buf) - 1) ? (sizeof(buf) - 1) : (_s32MaxSize - s32CheckedSize);
        memcpy(buf, input+s32CheckedSize, s32CheckingSize);

        n = s32CheckingSize;
        buf[n] = '\0';
        MD5Update(&ctx, buf, n);
        s32CheckedSize += n;
    }

    unsigned char digest[16];
    memset(digest, 0, sizeof(digest));
    MD5Final(digest, &ctx);

    char szTmp[128] = {0};
    memset(szTmp, 0, sizeof(szTmp));
    int i = 0;
    for (i = 0; i < 16; i++)
    {
        sprintf(szTmp+strlen(szTmp), "%02x", digest[i]);
    }
    memcpy(sum, szTmp, strlen(szTmp));

    return 0;
}



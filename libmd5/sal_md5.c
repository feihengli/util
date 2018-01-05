#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <netinet/in.h>
#include <linux/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/prctl.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <malloc.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <mtd/mtd-user.h>

#include "md5.h"
#include "sal_md5.h"

static int file_size(char* path)
{
    struct stat stStat;
    int ret = stat(path, &stStat);
	if (ret < 0)
	{
		printf("error with %#x: %s\n", ret, strerror(errno));
		return -1;
	}

    return stStat.st_size;
}


int md5_filesum(char* _szFilePath, unsigned char sum[64])
{
    unsigned char buf[1024 +1];
    int n;

    buf[1024] = '\0';

    FILE* pFile = fopen(_szFilePath, "r");
	if (!pFile)
	{
		printf("failed to fopen[%s] with: %s\n", _szFilePath, strerror(errno));
		return -1;
	}

    MD5_CTX ctx;
    MD5Init(&ctx);
    int s32CheckingSize;
    int s32CheckedSize = 0;
    int _s32MaxSize = file_size(_szFilePath);
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



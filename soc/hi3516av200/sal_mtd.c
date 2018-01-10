
#include "sal_debug.h"
#include "sal_malloc.h"
#include "sal_util.h"
#include "sal_mtd.h"

#define BUFSIZE (8*1024)

static int mtd_progress_bar(int precent)
{
    if (precent < 0 || precent > 100)
    {
        return -1;
    }

    char str[32];
    memset(str, 0, sizeof(str));
    int valid_len = sizeof(str)-1;
    int bar = valid_len * precent / 100;

    memset(str, ' ', valid_len);
    memset(str, '#', bar);

    //实际就是转移符\r的使用，\r的作用是返回至行首而不换行
    fprintf(stdout, "\r%s|%03d%%", str, precent);
    fflush(stdout); //一定要fflush，否则不会会因为缓冲无法定时输出。

    return 0;
}

/*
static int mtd_show(struct mtd_info_user* mtd)
{
    DBG("type: %d\n", mtd->type);
    DBG("flags: %d\n", mtd->flags);
    DBG("size: %d\n", mtd->size);
    DBG("erasesize: %d\n", mtd->erasesize);
    DBG("writesize: %d\n", mtd->writesize);
    DBG("oobsize: %d\n", mtd->oobsize);
    //DBG("ecctype: %d\n", mtd->ecctype);
    //DBG("eccsize: %d\n", mtd->eccsize);

    return 0;
}
*/

int sal_mtd_erase(char *devicename)
{
    CHECK(devicename, -1, "invalid parameter %#x\n", devicename);

    int ret = -1;
    int i = 0;
    int erase_count = 0;
    int precent = 0;

    struct mtd_info_user mtd;
    memset(&mtd, 0, sizeof(mtd));
    struct erase_info_user e;
    memset(&e, 0, sizeof(e));

    int fd = open(devicename, O_SYNC | O_RDWR, 0666);
    CHECK(fd > 0, -1, "failed to open[%s] with: %s\n", devicename, strerror(errno));

    ret = ioctl(fd, MEMGETINFO, &mtd);
    CHECK(ret == 0, -1, "Error with: %s\n", strerror(errno));

    CHECK(mtd.erasesize == 64*1024, -1, "Error with: %d\n", mtd.erasesize);
    CHECK((mtd.size % mtd.erasesize) == 0, -1, "Error with: %d\n", mtd.size);

    DBG("mtd: %s, block size: %d, total size: %d\n", devicename, mtd.erasesize, mtd.size);

    fprintf(stdout, "start to erase ...\n");
    // always erase a complete block
    erase_count = (mtd.size + mtd.erasesize - 1) / mtd.erasesize;
    // erase 1 block at a time to be able to give verbose output
    e.length = mtd.erasesize;
    e.start = 0;
    for (i = 1; i <= erase_count; i++)
    {
        ret = ioctl(fd, MEMERASE, &e);
        CHECK(ret == 0, -1, "Error with: %s\n", strerror(errno));

        e.start += mtd.erasesize;

        precent = 100*i/erase_count;
        mtd_progress_bar(precent);
    }
    fprintf(stdout, "\n");

    close(fd);
    return 0;
}

int sal_mtd_rerase(char *devicename)
{
    CHECK(devicename, -1, "invalid parameter %#x\n", devicename);

    int ret = -1;
    int i = 0;
    int erase_count = 0;
    int precent = 0;

    struct mtd_info_user mtd;
    memset(&mtd, 0, sizeof(mtd));
    struct erase_info_user e;
    memset(&e, 0, sizeof(e));

    int fd = open(devicename, O_SYNC | O_RDWR, 0666);
    CHECK(fd > 0, -1, "failed to open[%s] with: %s\n", devicename, strerror(errno));

    ret = ioctl(fd, MEMGETINFO, &mtd);
    CHECK(ret == 0, -1, "Error with: %s\n", strerror(errno));

    CHECK(mtd.erasesize == 64*1024, -1, "Error with: %d\n", mtd.erasesize);
    CHECK((mtd.size % mtd.erasesize) == 0, -1, "Error with: %d\n", mtd.size);

    DBG("mtd: %s, block size: %d, total size: %d\n", devicename, mtd.erasesize, mtd.size);

    fprintf(stdout, "start to reverse erase ...\n");
    // always erase a complete block
    erase_count = (mtd.size + mtd.erasesize - 1) / mtd.erasesize;
    // erase 1 block at a time to be able to give verbose output
    e.length = mtd.erasesize;
    e.start = mtd.size - mtd.erasesize;
    for (i = 1; i <= erase_count; i++)
    {
        ret = ioctl(fd, MEMERASE, &e);
        CHECK(ret == 0, -1, "Error with: %s\n", strerror(errno));

        e.start -= mtd.erasesize;

        precent = 100*i/erase_count;
        mtd_progress_bar(precent);
    }
    fprintf(stdout, "\n");

    close(fd);
    return 0;
}


int sal_mtd_write(char *filebuf, int filesize, char *devicename)
{
    CHECK(filebuf, -1, "invalid parameter %#x\n", filebuf);
    CHECK(filesize > 0, -1, "invalid parameter %#x\n", filesize);
    CHECK(devicename, -1, "invalid parameter %#x\n", devicename);

    int ret = -1;
    int precent = 0;

    char* buf = mem_malloc(1024*64);
    CHECK(buf, -1, "failed to malloc[%d]\n", 1024*64);

    struct mtd_info_user mtd;
    memset(&mtd, 0, sizeof(mtd));
    struct erase_info_user e;
    memset(&e, 0, sizeof(e));

    int fd = open(devicename, O_SYNC | O_RDWR, 0666);
    CHECK(fd > 0, -1, "failed to open[%s] with: %s\n", devicename, strerror(errno));

    ret = ioctl(fd, MEMGETINFO, &mtd);
    CHECK(ret == 0, -1, "Error with: %s\n", strerror(errno));

    CHECK(mtd.erasesize == 64*1024, -1, "Error with: %d\n", mtd.erasesize);
    CHECK((mtd.size % mtd.erasesize) == 0, -1, "Error with: %d\n", mtd.size);
    CHECK(mtd.size >= filesize, -1, "Error with: %d\n", mtd.size);

    DBG("mtd: %s, block size: %d, total size: %d\n", devicename, mtd.erasesize, mtd.size);

    char * filecontent = filebuf;
    unsigned count = BUFSIZE;
    int offset = 0;

    fprintf(stdout, "start to write ...\n");
    ret = lseek(fd, 0, SEEK_SET);
    CHECK(ret == 0, -1, "Error with: %s\n", strerror(errno));

    while (offset < filesize)
    {
        int remain = filesize - offset;
        if (remain == 0)
            break;
        if (remain < BUFSIZE)
            count = remain;

        memcpy(buf, filecontent+offset, count);
        int nbytes = write(fd, buf, count);
        if (nbytes < 0)
        {
            CHECK(errno == EINTR, -1, "Error with: %s\n", strerror(errno));
        }
        else
        {
            offset += nbytes;
        }

        precent = 100*offset/filesize;
        mtd_progress_bar(precent);
    }
    fprintf(stdout, "\n");

    mem_free(buf);
    close(fd);
    return 0;
}

int sal_mtd_verify(char *filebuf, int filesize, char *devicename)
{
    CHECK(filebuf, -1, "invalid parameter %#x\n", filebuf);
    CHECK(filesize > 0, -1, "invalid parameter %#x\n", filesize);
    CHECK(devicename, -1, "invalid parameter %#x\n", devicename);

    int ret = -1;
    int precent = 0;

    char* buf = mem_malloc(1024*64);
    CHECK(buf, -1, "failed to malloc[%d]\n", 1024*64);

    struct mtd_info_user mtd;
    memset(&mtd, 0, sizeof(mtd));
    struct erase_info_user e;
    memset(&e, 0, sizeof(e));

    int fd = open(devicename, O_SYNC | O_RDWR, 0666);
    CHECK(fd > 0, -1, "failed to open[%s] with: %s\n", devicename, strerror(errno));

    ret = ioctl(fd, MEMGETINFO, &mtd);
    CHECK(ret == 0, -1, "Error with: %s\n", strerror(errno));

    CHECK(mtd.erasesize == 64*1024, -1, "Error with: %d\n", mtd.erasesize);
    CHECK((mtd.size % mtd.erasesize) == 0, -1, "Error with: %d\n", mtd.size);
    CHECK(mtd.size >= filesize, -1, "Error with: %d\n", mtd.size);

    DBG("mtd: %s, block size: %d, total size: %d\n", devicename, mtd.erasesize, mtd.size);

    char * filecontent = filebuf;
    unsigned count = BUFSIZE;
    int offset = 0;

    fprintf(stdout, "start to verify ...\n");
    ret = lseek(fd, 0, SEEK_SET);
    CHECK(ret == 0, -1, "Error with: %s\n", strerror(errno));

    while (offset < filesize)
    {
        int remain = filesize - offset;
        if (remain == 0)
            break;
        if (remain < BUFSIZE)
            count = remain;

        int nbytes = read(fd, buf, count);
        if (nbytes < 0)
        {
            CHECK(errno == EINTR, -1, "Error with: %s\n", strerror(errno));
        }
        else
        {
            ret = memcmp(buf, filecontent+offset, nbytes);
            if (ret == 0)
            {
                offset += nbytes;
            }
            else
            {
                ERR("verification mismatch at offset[%#x]\n", offset);
                return -1;
            }
        }

        precent = 100*offset/filesize;
        mtd_progress_bar(precent);
    }
    fprintf(stdout, "\n");

    mem_free(buf);
    close(fd);
    return 0;
}

int sal_mtd_readflag(char *devicename, char* output, int len)
{
    CHECK(devicename, -1, "invalid parameter %#x\n", devicename);
    CHECK(output, -1, "invalid parameter %#x\n", output);
    CHECK(len >= 4, -1, "invalid parameter %#x\n", len);

    int ret = -1;
    char flag[8];
    memset(flag, 0, sizeof(flag));

    struct mtd_info_user mtd;
    memset(&mtd, 0, sizeof(mtd));
    struct erase_info_user e;
    memset(&e, 0, sizeof(e));

    int fd = open(devicename, O_SYNC | O_RDWR, 0666);
    CHECK(fd > 0, -1, "failed to open[%s] with: %s\n", devicename, strerror(errno));

    ret = ioctl(fd, MEMGETINFO, &mtd);
    CHECK(ret == 0, -1, "Error with: %s\n", strerror(errno));

    //mtd_show(&mtd);

    CHECK(mtd.erasesize == 64*1024, -1, "Error with: %d\n", mtd.erasesize);
    CHECK((mtd.size % mtd.erasesize) == 0, -1, "Error with: %d\n", mtd.size);
    //CHECK(mtd.size >= filesize, -1, "Error with: %d\n", mtd.size);

    int offset = mtd.size-4;
    //int offset = 0;
    ret = lseek(fd, offset, SEEK_SET);
    CHECK(ret == offset, -1, "Error with: %s\n", strerror(errno));

    DBG("mtd: %s, block size: %d, total size: %d, offset: %d\n", devicename, mtd.erasesize, mtd.size, offset);

    int nbytes = read(fd, flag, 4);
    CHECK(nbytes == 4, -1, "Error with: %s\n", strerror(errno));

    memcpy(output, flag, 4);
    DBG("flag[%d]: %s\n", strlen(flag), flag);

    close(fd);
    return 0;
}

int sal_mtd_writeflag(char *devicename, char* input, int len)
{
    CHECK(devicename, -1, "invalid parameter %#x\n", devicename);
    CHECK(input, -1, "invalid parameter %#x\n", input);
    CHECK(len == 4, -1, "invalid parameter %#x\n", len);

    int ret = -1;
    char flag[8];
    memset(flag, 0, sizeof(flag));
    memcpy(flag, input, len);

    struct mtd_info_user mtd;
    memset(&mtd, 0, sizeof(mtd));
    struct erase_info_user e;
    memset(&e, 0, sizeof(e));

    int fd = open(devicename, O_SYNC | O_RDWR, 0666);
    CHECK(fd > 0, -1, "failed to open[%s] with: %s\n", devicename, strerror(errno));

    ret = ioctl(fd, MEMGETINFO, &mtd);
    CHECK(ret == 0, -1, "Error with: %s\n", strerror(errno));

    //mtd_show(&mtd);

    CHECK(mtd.erasesize == 64*1024, -1, "Error with: %d\n", mtd.erasesize);
    CHECK((mtd.size % mtd.erasesize) == 0, -1, "Error with: %d\n", mtd.size);
    //CHECK(mtd.size >= filesize, -1, "Error with: %d\n", mtd.size);

    int offset = mtd.size-strlen(flag);
    //int offset = 0;
    ret = lseek(fd, offset, SEEK_SET);
    CHECK(ret == offset, -1, "Error with: %s\n", strerror(errno));

    DBG("mtd: %s, block size: %d, total size: %d, offset: %d\n", devicename, mtd.erasesize, mtd.size, offset);


    DBG("flag[%d]: %s\n", strlen(flag), flag);
    int nbytes = write(fd, flag, strlen(flag));
    CHECK(nbytes == strlen(flag), -1, "Error with: %s\n", strerror(errno));

    close(fd);
    return 0;
}


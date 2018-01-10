/* vi: set sw=4 ts=4: */
/*
 * busybox reimplementation of flashcp
 *
 * (C) 2009 Stefan Seyfried <seife@sphairon.com>
 *
 * Licensed under GPLv2, see file LICENSE in this tarball for details.
 */

#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mtd/mtd-user.h>

#include "sal_flash.h"

#define OFF_FMT "l"
typedef unsigned long uoff_t;
#define BUFSIZE (8 * 1024)

static int  fflush_all(void)
{
    return fflush(NULL);
}


static void progress(int mode, unsigned long int count, unsigned long int total)
{
    unsigned int percent = 0;

    percent = count * 100;
    if (total)
        percent = (unsigned) (percent / total);
    printf("\r%s: %"OFF_FMT"u/%"OFF_FMT"u (%u%%) ",
        (mode == 0) ? "Erasing block" : ((mode == 1) ? "Writing kb" : "Verifying kb"),
        count, total, (unsigned)percent);
    fflush_all();
}


static int  bb_putchar(int ch)
{
    return putchar(ch);
}


static void progress_newline(void)
{
    bb_putchar('\n');
}

static int  xopen3(const char *pathname, int flags, int mode)
{
    int ret;

    ret = open(pathname, flags, mode);
    if (ret < 0) {
        printf("can't open '%s'", pathname);
    }
    return ret;
}

// Die if we can't open a file and return a fd.
static int  xopen(const char *pathname, int flags)
{
    return xopen3(pathname, flags, 0666);
}

static off_t  xlseek(int fd, off_t offset, int whence)
{
    off_t off = lseek(fd, offset, whence);
    if (off == (off_t)-1) {
        if (whence == SEEK_SET)
        {
            printf("lseek(%"OFF_FMT"u)", offset);
            return -1;

        }
        printf("lseek");
        return -1;

    }
    return off;
}

static ssize_t  safe_read(int fd, void *buf, size_t count)
{
    ssize_t n;

    do {
        n = read(fd, buf, count);
    } while (n < 0 && errno == EINTR);

    return n;
}


static ssize_t  full_read(int fd, void *buf, size_t len)
{
    ssize_t cc;
    ssize_t total;

    total = 0;

    while (len) {
        cc = safe_read(fd, buf, len);

        if (cc < 0) {
            if (total) {
                /* we already have some! */
                /* user can do another read to know the error code */
                return total;
            }
            return cc; /* read() returns -1 on failure. */
        }
        if (cc == 0)
            break;
        buf = ((char *)buf) + cc;
        total += cc;
        len -= cc;
    }

    return total;
}

static int  xread(int fd, void *buf, size_t count)
{
    if (count) {
        ssize_t size = full_read(fd, buf, count);
        if ((size_t)size != count)
        {
            printf("short read");
            return -1;
        }
    }

    return 0;
}


static ssize_t  safe_write(int fd, const void *buf, size_t count)
{
    ssize_t n;

    do {
        n = write(fd, buf, count);
    } while (n < 0 && errno == EINTR);

    return n;
}


static ssize_t  full_write(int fd, const void *buf, size_t len)
{
    ssize_t cc;
    ssize_t total;

    total = 0;

    while (len) {
        cc = safe_write(fd, buf, len);

        if (cc < 0) {
            if (total) {
                /* we already wrote some! */
                /* user can do another write to know the error code */
                return total;
            }
            return cc;  /* write() returns -1 on failure. */
        }

        total += cc;
        buf = ((const char *)buf) + cc;
        len -= cc;
    }

    return total;
}

int sal_flashcp(char *filebuf, int filesize,  char *devicename)
{
    printf("flashcp file(size:%d, devcename:%s)\n", filesize, devicename);
    static char buf[1024*64];
    static char buf2[1024*64];
    //int fd_f, fd_d; /* input file and mtd device file descriptors */
    int i;
    int erase_count;
    //unsigned opts;
    struct mtd_info_user mtd;
    struct erase_info_user e;

    char * filecontent = filebuf;

    int fd_d = xopen(devicename, O_SYNC | O_RDWR);
#if !MTD_DEBUG
    if (ioctl(fd_d, MEMGETINFO, &mtd) < 0) {
        printf("%s is not a MTD flash device", devicename);
        return -1;
    }
    if (filesize > mtd.size) {
        printf("file size bigger than %s", devicename);
        return -1;
    }
#else
    mtd.erasesize = 64 * 1024;
#endif

    /* always erase a complete block */
    erase_count = (uoff_t)(filesize + mtd.erasesize - 1) / mtd.erasesize;
    /* erase 1 block at a time to be able to give verbose output */
    e.length = mtd.erasesize;

    e.start = 0;
    for (i = 1; i <= erase_count; i++) {
        progress(0, i, erase_count);
        errno = 0;
#if !MTD_DEBUG
        if (ioctl(fd_d, MEMERASE, &e) < 0) {
            printf("erase error at 0x%llx on %s",
                (long long)e.start, devicename);
            return -1;
        }
#else
        usleep(100*1000);
#endif
        e.start += mtd.erasesize;
    }
    progress_newline();

    /* doing this outer loop gives significantly smaller code
     * than doing two separate loops for writing and verifying */

    int pPos = 0;

    for (i = 1; i <= 2; i++) {
        unsigned long int done;
        unsigned count;

        pPos = 0;
        xlseek(fd_d, 0, SEEK_SET);
        done = 0;
        count = BUFSIZE;
        while (1) {
            int rem = filesize - done;
            if (rem == 0)
                break;
            if (rem < BUFSIZE)
                count = rem;
            progress(i, done / 1024, (int)filesize / 1024);

            memcpy(buf, filecontent + pPos, count);
            pPos += count;
            if (i == 1) {
                int ret;
                errno = 0;
                ret = full_write(fd_d, buf, count);
                if (ret != count) {
                    printf("write error at 0x%"OFF_FMT"x on %s, "
                        "write returned %d",
                        done, devicename, ret);
                    return -1;
                }
            } else { /* i == 2 */
                xread(fd_d, buf2, count);
                if (memcmp(buf, buf2, count)) {
                    printf("verification mismatch at 0x%"OFF_FMT"x", done);
                    return -1;
                }
            }

            done += count;
        }

        progress_newline();
    }
    /* we won't come here if there was an error */

    return 0;

}


/*
int flashcpm_test()
{
    const char* path = "uImage";
    FILE* fp = fopen(path, "r");
    if (!fp)
    {
        printf("failed to fopen %s: %s\n", path, strerror(errno));
        return -1;
    }

    struct stat stStat;
    int ret = stat(path, &stStat);
    if (ret != 0)
    {
        printf("failed to stat %s: %s\n", path, strerror(errno));
        return -1;
    }

    int len = stStat.st_size;
    char* buf = malloc(len);
    if (!buf)
    {
        printf("failed to malloc %d: %s\n", len, strerror(errno));
        return -1;
    }

    int read_len = 0;
    while (read_len != len)
    {
        int tmp = fread(buf + read_len, 1, len-read_len, fp);
        if (tmp < 0)
        {
            printf("error with %#x: %s\n", tmp, strerror(errno));
            return -1;
        }
        read_len += tmp;
    }

    const char* mtd = "/dev/mtd1";

    //flash_eraseall
    char cmd[128];
    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd, "flash_eraseall %s", mtd);
    printf("run cmd: %s\n", cmd);
    ret = system(cmd);
    printf("done.ret: %d\n", ret);

    //flash_cp
    ret = flashcpm_main(buf, len, mtd);
    if(ret != 0)
    {
        printf("flash write fail, rebooting...\n");
    }

    free(buf);
    return 0;
}
*/



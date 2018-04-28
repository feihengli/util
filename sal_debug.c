
#include "sal_debug.h"

#define KEY (0x23581321)
#define TYPE (1)
#define MAXSENDSIZE (sizeof(Message)-sizeof(long int))

typedef struct msg_s
{
    int inited;
    int msgid;
    long int type;
    pthread_mutex_t mutex;
}msg_s;

static msg_s msg;

char* debug_time_str()
{
    int __errno_bak = errno;
    static char time[32] = "";
    struct timeval tv = {0, 0};
    gettimeofday(&tv, NULL);
    //strftime(time, sizeof(time), "%F %H:%M:%S", localtime(&tv.tv_sec));
    strftime(time, sizeof(time), "%Y/%m/%d %H:%M:%S", localtime(&tv.tv_sec));
    sprintf(time, "%s.%06ld", time, tv.tv_usec);
    errno = __errno_bak;
    return time;
}

static int debug_init()
{
    memset(&msg, 0, sizeof(msg));
    msg.type = TYPE;
    msg.msgid = msgget(KEY, 0666|IPC_CREAT);
    if (msg.msgid == -1)
    {
        perror("create_msg failed!\n");
        return -1;
    }
    
    struct msqid_ds buf;
    memset(&buf, 0, sizeof(buf));
    msgctl(msg.msgid, IPC_STAT, &buf);
    buf.msg_qbytes=32*1024;
    msgctl(msg.msgid, IPC_SET, &buf);
    pthread_mutex_init(&msg.mutex, NULL);
    msg.inited = 1;

    return 0;
}

int debug_print(const char* format, ...)
{
    if (!msg.inited)
    {
        debug_init();
    }
    
    pthread_mutex_lock(&msg.mutex);
    static Message send_buf;
    memset(&send_buf, 0, sizeof(send_buf));
    send_buf.type = msg.type;

    va_list ap;
    va_start(ap, format);
    vsnprintf(send_buf.buffer, sizeof(send_buf.buffer), format, ap);
    va_end(ap);

    fprintf(stdout, "%s", send_buf.buffer);
    int ret = msgsnd(msg.msgid, &send_buf, MAXSENDSIZE, IPC_NOWAIT);
    if (ret < 0 && errno == EAGAIN)
    {
        msgctl(msg.msgid, IPC_RMID, 0);
        debug_init();
    }
    pthread_mutex_unlock(&msg.mutex);

    return 0;
}

int debug_recv(Message* recv)
{
    if (!msg.inited)
    {
        debug_init();
    }
    memset(recv, 0, sizeof(*recv));
    int ret = msgrcv(msg.msgid, recv, MAXSENDSIZE, msg.type, 0);
    if (ret < 0)
    {
        perror("error\n");
        return -1;
    }

    return 0;
}



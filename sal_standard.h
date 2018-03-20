#ifndef __SAL_STANDARD_H__
#define __SAL_STANDARD_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

/*
**此头文件只包含标准的头文件
*/

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



typedef void* handle;

////用于单独抠出某个模块编译
//#define CHECK 
//#define mem_malloc malloc
//#define mem_free free
//#define DBG printf
//#define ERR DBG
//#define WRN DBG

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif




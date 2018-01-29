#ifndef __SAL_DEBUG_H__
#define __SAL_DEBUG_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#define USE_MESSAGE 1

#include "sal_standard.h"

#define NONE "\033[m"
#define RED "\033[0;32;31m"
#define LIGHT_RED "\033[1;31m"
#define GREEN "\033[0;32;32m"
#define LIGHT_GREEN "\033[1;32m"
#define BLUE "\033[0;32;34m"
#define LIGHT_BLUE "\033[1;34m"
#define DARY_GRAY "\033[1;30m"
#define CYAN "\033[0;36m"
#define LIGHT_CYAN "\033[1;36m"
#define PURPLE "\033[0;35m"
#define LIGHT_PURPLE "\033[1;35m"
#define BROWN "\033[0;33m"
#define YELLOW "\033[1;33m"
#define LIGHT_GRAY "\033[0;37m"
#define WHITE "\033[1;37m"

#define SIZE (1024)

typedef struct
{
    long int type;
    char buffer[SIZE];
}Message;

int debug_print(const char* format, ...);
int debug_recv(Message* recv);
char* debug_time_str();


#if USE_MESSAGE

#define DBG(fmt, ...) do\
{\
    debug_print(GREEN"%s debug %s: %d: "NONE""fmt"", debug_time_str(), __FUNCTION__, __LINE__, ##__VA_ARGS__);\
}while(0)

#define ERR(fmt, ...) do\
{\
    debug_print(RED"%s error %s: %d: "NONE""fmt"", debug_time_str(), __FUNCTION__, __LINE__, ##__VA_ARGS__);\
}while(0)

#define WRN(fmt, ...) do\
{\
    debug_print(YELLOW"%s warn %s: %d: "NONE""fmt"", debug_time_str(), __FUNCTION__, __LINE__, ##__VA_ARGS__);\
}while(0)

#else

#define DBG(fmt, ...) do\
{\
    fprintf(stdout, GREEN"%s debug %s: %d: "NONE""fmt"", debug_time_str(), __FUNCTION__, __LINE__, ##__VA_ARGS__);\
}while(0)

#define ERR(fmt, ...) do\
{\
    fprintf(stdout, RED"%s error %s: %d: "NONE""fmt"", debug_time_str(), __FUNCTION__, __LINE__, ##__VA_ARGS__);\
}while(0)

#define WRN(fmt, ...) do\
{\
    fprintf(stdout, YELLOW"%s warn %s: %d: "NONE""fmt"", debug_time_str(), __FUNCTION__, __LINE__, ##__VA_ARGS__);\
}while(0)

#endif

#define CHECK(exp, ret, fmt...) do\
{\
    if (!(exp))\
    {\
        ERR(fmt);\
        return ret;\
    }\
}while(0)

#define ASSERT(exp, fmt...) do\
{\
    if (!(exp))\
    {\
        ERR(fmt);\
        assert(exp);\
    }\
}while(0)


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif



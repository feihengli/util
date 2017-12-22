
#include "sal_debug.h"
#include "sal_list.h"
#include "sal_malloc.h"
#include "sal_rtsp_server.h"
#include "sal_rtsp_process.h"

typedef struct rtsps_s
{
    int ListenFd;// 用于监听
    int Status; // connect status
    handle hndClientList; // 存放已连接上的客户端
    int running;
    pthread_t pid;
}rtsps_s;

static int rtsps_accept(rtsps_s* pRtsps)
{
    int ret = -1;

    struct sockaddr_in RemoteAddr;
    socklen_t Len = sizeof(struct sockaddr_in);
    memset(&RemoteAddr, 0, Len);

    int s32ConFd = accept(pRtsps->ListenFd, (struct sockaddr*)&RemoteAddr, &Len);
    if (s32ConFd < 0)
    {
       ERR("bad accept, s32ConFd: %d\n", s32ConFd);
       close(s32ConFd);
       return 0;
    }
    else
    {
        DBG("Accept Connected, s32ConFd: %d, ip: %s, port: %d\n", s32ConFd, inet_ntoa(RemoteAddr.sin_addr), ntohs(RemoteAddr.sin_port));
    }

    client_s stClient;
    memset(&stClient, 0, sizeof(client_s));
    //先存入链表，后再取pclient指针，用于线程传递
    ret= list_push_back(pRtsps->hndClientList, &stClient, sizeof(client_s));
    CHECK(ret == 0, -1, "error with %#x\n", ret);

    client_s* pclient = list_back(pRtsps->hndClientList);
    CHECK(pclient, -1, "error with %#x\n", pclient);

    pclient->fd = s32ConFd;

    pclient->running = 1;
    ret = pthread_create(&pclient->pid, NULL, rtsp_process, pclient);
    CHECK(ret == 0, -1, "error with %#x: %s\n", ret, strerror(errno));

    return 0;
}

static void* rtsps_listen(void* hndRtsps)
{
    prctl(PR_SET_NAME, __FUNCTION__);
    rtsps_s* pRtsps = (rtsps_s*)hndRtsps;
    int ret = -1;

    fd_set rset;
    FD_ZERO(&rset);

    while (pRtsps->running)
    {
        //回收客户端线程
        client_s* pdelete = NULL;
        client_s* pclient = list_front(pRtsps->hndClientList);
        while (pclient)
        {
            pdelete = NULL;
            if (!pclient->running)
            {
                pthread_join(pclient->pid, NULL);
                close(pclient->fd);
                pdelete = pclient;
                DBG("release fd[%d] ok.\n", pclient->fd);
            }
            pclient = list_next(pRtsps->hndClientList, pclient);
            if (pdelete)
            {
                ret = list_del(pRtsps->hndClientList, pdelete);
                CHECK(ret == 0, NULL, "error with %#x\n", ret);
            }
        }

        //WRN("already has %d client.\n", list_size(pRtsps->hndClientList));

        FD_ZERO(&rset);
        FD_SET(pRtsps->ListenFd, &rset);
        struct timeval TimeoutVal = {0, 10*1000};
        ret = select(pRtsps->ListenFd + 1, &rset, NULL, NULL, &TimeoutVal);
        if (ret < 0)
        {
            if (errno != EINTR && errno != EAGAIN)
            {
                ERR("select failed with: %s\n", strerror(errno));
                break;
            }
            continue;
        }
        else if (ret == 0)
        {
            //DBG("time out.\n");
            continue;
        }

        if (FD_ISSET(pRtsps->ListenFd, &rset))//新的客户端连接
        {
            ret = rtsps_accept(pRtsps);
            CHECK(ret == 0, NULL, "error with %#x\n", ret);
        }

    }

    return NULL;
}

static int rtsps_setup_tcp(int port)
{
    int ret = -1;
    int ListenFd = socket(AF_INET, SOCK_STREAM, 0);
    CHECK(ListenFd > 0, -1, "error with %#x: %s\n", ListenFd, strerror(errno));

    int Open = 1;
    ret = setsockopt(ListenFd, SOL_SOCKET, SO_REUSEADDR, &Open, sizeof(int));
    CHECK(ret == 0, -1, "error with %#x: %s\n", ret, strerror(errno));

    struct sockaddr_in Addr;
    memset((char *)&Addr, 0, sizeof(struct sockaddr_in));
    Addr.sin_family = AF_INET;
    Addr.sin_port = htons(port);
    Addr.sin_addr.s_addr = htonl(INADDR_ANY);//允许连接到所有本地地址上
    ret = bind(ListenFd, (struct sockaddr *)&Addr, sizeof(struct sockaddr));
    CHECK(ret == 0, -1, "error with %#x: %s\n", ret, strerror(errno));

    ret = listen(ListenFd, 1024);
    CHECK(ret == 0, -1, "error with %#x: %s\n", ret, strerror(errno));

    return ListenFd;

}

handle rtsps_init(int port)
{
    CHECK(port > 0, NULL, "invalid parameter.\n");

    int ret = -1;
    rtsps_s* pRtsps = (rtsps_s*)mem_malloc(sizeof(rtsps_s));
    CHECK(pRtsps, NULL, "error with %#x\n", pRtsps);
    memset(pRtsps, 0, sizeof(rtsps_s));

    pRtsps->ListenFd = rtsps_setup_tcp(port);
    CHECK(pRtsps->ListenFd > 0, NULL, "error with %#x\n", pRtsps->ListenFd);

    pRtsps->hndClientList = list_init(sizeof(client_s));
    CHECK(pRtsps->hndClientList, NULL, "error with %#x\n", pRtsps->hndClientList);

    pRtsps->running = 1;
    ret = pthread_create(&pRtsps->pid, NULL, rtsps_listen, pRtsps);
    CHECK(ret == 0, NULL, "error with %#x: %s\n", ret, strerror(errno));

    return pRtsps;
}

int rtsps_destroy(handle hndRtsps)
{
    CHECK(hndRtsps, -1, "invalid parameter.\n");

    int ret = -1;
    rtsps_s* pRtsps = (rtsps_s*)hndRtsps;

    if (pRtsps->running)
    {
        pRtsps->running = 0;
        pthread_join(pRtsps->pid, NULL);
        close(pRtsps->ListenFd);
    }

    client_s* pclient = list_front(pRtsps->hndClientList);
    while (pclient)
    {
        if (pclient->running)
        {
            pclient->running = 0;
            pthread_join(pclient->pid, NULL);
            close(pclient->fd);
        }
        pclient = list_next(pRtsps->hndClientList, pclient);
    }
    ret = list_destroy(pRtsps->hndClientList);
    CHECK(ret == 0, -1, "error with %#x\n", ret);

    mem_free(pRtsps);
    return 0;
}





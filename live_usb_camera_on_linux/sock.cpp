/*
 * =====================================================================================
 *
 *       Filename:  sock.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/13/2014 02:13:47 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  xieminchen (mtk), hs_xieminchen@mediatek.com
 *        Company:  Mediatek
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <memory.h>
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <errno.h>
#include <assert.h>
#include <fcntl.h>
#include <pthread.h>

#include "sock.h"
#include "log.h"

typedef struct SocketContext
{
    int socket;
    struct sockaddr_in sockaddr;

    // accept thread
    pthread_t pid;
    SocketAcceptHandler cb;
    void *tag;
    struct SocketContext *recvSocket;
} SocketContext;

static void *thread_hook(void *tag);
static PSocketContext sock_new();

PSocketContext sock_open(const char *addr, int port)
{
    PSocketContext ps = NULL;
    do {
        ps = sock_new();
        if (NULL == ps) {
            LOGE("Cannot malloc socket context");
            break;
        }
        memset(ps, 0, sizeof(*ps));

        ps->socket = socket(AF_INET , SOCK_STREAM , 0);
        if (-1 == ps->socket) {
            LOGE("Cannot create socket: %d", errno);
            break;
        }
        // set non block
        int flags = fcntl(ps->socket, F_GETFL, 0);
        flags = flags | O_NONBLOCK;
        fcntl(ps->socket, F_SETFL, flags);

        // Prepare the sockaddr_in structure
        ps->sockaddr.sin_family = AF_INET;
        if (NULL != addr)
            ps->sockaddr.sin_addr.s_addr = inet_addr(addr);
        else
            ps->sockaddr.sin_addr.s_addr = INADDR_ANY;
        ps->sockaddr.sin_port = htons(port);

        return ps;
    } while(0);

    if (NULL != ps)
        sock_close(&ps);
    return NULL;
}

void sock_close(PSocketContext *pps)
{
    if (NULL == pps || NULL == *pps)
        return;

    PSocketContext ps = *pps;
    if (NULL != ps->recvSocket)
        sock_close(&ps->recvSocket);
    if (-1 != ps->socket) {
        LOGD("Close socket %d", ps->socket);
        close(ps->socket);
    }
    if (0 != ps->pid) {
        LOGD("Join thread %d", (int)ps->pid);
        pthread_join(ps->pid, NULL);
    }

    free(ps);
    *pps = NULL;
}

int sock_accpet(PSocketContext ps, SocketAcceptHandler cb, void *tag)
{
    int res = bind(ps->socket, (struct sockaddr *)&ps->sockaddr,
            sizeof(ps->sockaddr));
    if (0 != res) {
        LOGE("Cannot bind socket %d: %d", ps->socket, errno);
        return -1;
    }

    res = listen(ps->socket, 1);
    if (0 != res) {
        LOGE("Cannot listen: %d", errno);
        return -1;
    }

    ps->cb = cb;
    ps->tag = tag;
    // start thread
    res = pthread_create(&ps->pid, NULL, thread_hook, ps);
    if (0 != res) {
        LOGE("Cannot create thread: %d", res);
        return -1;
    }

    return 0;
}

int sock_connect(PSocketContext ps)
{
    return connect(ps->socket, (struct sockaddr *)&ps->sockaddr,
            sizeof(ps->sockaddr));
}

int sock_send(PSocketContext ps, const char *buf, int len)
{
    int sendlen = 0;
    int res = 0;
    while (sendlen < len) {
        res = send(ps->socket, buf + res, len - res, 0);
        if (res < 0)
            return -1;
        sendlen += res;
    }

    return 0;
}

int sock_recv(PSocketContext ps, char *buf, int len)
{
    return recv(ps->socket, buf, len, 0);
}

int sock_fd(PSocketContext ps)
{
    return ps->socket;
}

static void *thread_hook(void *tag)
{
    PSocketContext ps = (PSocketContext)tag;
    struct sockaddr_in client;
    int c = sizeof(struct sockaddr_in);
    while (1) {
        // LOGD("before accept socket: %d", ps->socket);
        int fd = accept4(ps->socket, (struct sockaddr *)&client,
                (socklen_t *)&c, SOCK_NONBLOCK);
        /*
        LOGD("accept socket: %d EAGIN %d ENOBLOCK %d errno %d",
                fd, EAGAIN, EWOULDBLOCK, errno);
        */
        if (fd < 0 && (EAGAIN == errno || EWOULDBLOCK == errno)) {
            // sleep for a while
            usleep(100 * 1000);
            continue;
        }
        if (fd < 0)
            break;

        if (NULL != ps->recvSocket) {
            LOGE("close accept fd %d, already accept %d", fd,
                    ps->recvSocket->socket);
            close(fd);
            continue;
        }

        // new
        ps->recvSocket = sock_new();
        if (NULL == ps->recvSocket) {
            LOGE("Cannot create socket");
            continue;
        }
        ps->recvSocket->socket = fd;
        ps->recvSocket->sockaddr = client;

        // notify
        if (NULL != ps->cb)
            ps->cb(ps->recvSocket, &client, ps->tag);
    }
}

static PSocketContext sock_new()
{
    PSocketContext ps = (PSocketContext)malloc(sizeof(SocketContext));
    if (NULL == ps)
        return NULL;

    memset(ps, 0, sizeof(*(ps->recvSocket)));
    ps->socket = -1;
    ps->pid = 0;

    return ps;
}


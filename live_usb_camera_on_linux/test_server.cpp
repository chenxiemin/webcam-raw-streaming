/*
 * =====================================================================================
 *
 *       Filename:  rend_server.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/13/2014 02:12:57 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  xieminchen (mtk), hs_xieminchen@mediatek.com
 *        Company:  Mediatek
 *
 * =====================================================================================
 */

#include <stdio.h>

#include "sock.h"
#include "log.h"

void accept_cb(PSocketContext ps, void *client, void *tag)
{
    LOGD("Accept fd: %d", sock_fd(ps));
    while (1) {
        char buf[255] = { 0 };
        int recvlen = sock_recv(ps, buf, 264);
        if (recvlen < 0)
            break; // socket close
        if (recvlen == 0)
            continue; // non block

        LOGD("recv buffer len %d string %s", recvlen, buf);
    }
}

int main()
{
    PSocketContext ps = sock_open(NULL, 5000);
    if (NULL == ps) {
        LOGE("Cannot create socket");
        return -1;
    }

    int res = sock_accpet(ps, accept_cb, NULL);
    if (0 != res)
        LOGE("Cannot accept: %d", res);

    // wait
    scanf("%c", (char *)&res);
    
    sock_close(&ps);
    return 0;
}


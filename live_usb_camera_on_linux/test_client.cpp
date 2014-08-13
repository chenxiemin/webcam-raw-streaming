/*
 * =====================================================================================
 *
 *       Filename:  rend_client.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/13/2014 02:54:31 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  xieminchen (mtk), hs_xieminchen@mediatek.com
 *        Company:  Mediatek
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <string.h>

#include "sock.h"
#include "log.h"

int main()
{
    PSocketContext ps = sock_open("127.0.0.1", 5000);
    if (NULL == ps) {
        LOGE("Cannot create socket");
        return -1;
    }

    int res = sock_connect(ps);
    if (0 != res)
        LOGE("Cannot connect socket: %d", res);

    const char *buf = "chenxiemin";
    sock_send(ps, buf, strlen(buf));

    sock_close(&ps);
    return 0;
}


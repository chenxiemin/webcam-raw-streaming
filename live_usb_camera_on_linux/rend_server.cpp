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
#include <iostream>
#include <SDL/SDL.h>
#include <chrono>

#include "types.h"
#include "capture.h"
#include "vcompress.h"
#include "decode_context.h"
#include "sock.h"
#include "log.h"


using namespace std;
using namespace std::chrono;

#define WIDTH 640
#define HEIGHT 480
#define FMT PIX_FMT_YUV420P
#define FPS 30
#define FF_REFRESH_EVENT (SDL_USEREVENT + 1)

// codecs
void *mp_capture = NULL;
void *mp_compress = NULL;
// socket
PSocketContext ps = NULL;

void accept_cb(PSocketContext ps, void *client, void *tag)
{
    LOGD("Accept fd: %d", sock_fd(ps));

    char buf[255] = { 0 };
    Picture pic;
    while (1) {
        int recvlen = sock_recv(ps, buf, 264);
        if (recvlen < 0)
            break; // socket close
        if (recvlen == 0)
            continue; // non block

        // LOGD("recv buffer len %d string %s", recvlen, buf);

        // prepare vide data
		int res = capture_get_picture(mp_capture, &pic);
        if (1 != res) {
            cout << "Cannot get picture: " << res << endl;
            continue;
        }
		const void *outbuf;
		int outlen;
		res = vc_compress(mp_compress, pic.data, pic.stride, &outbuf, &outlen);
        if (1 != res) {
            cout << "Cannot compress pic: " << res << endl;
            continue;
        }

        // send
        res = sock_send(ps, (char *)outbuf, outlen);
        LOGD("Send buffer %p len %d res %d", outbuf, outlen, res);
    }
}

int main()
{
    do {
        PSocketContext ps = sock_open(NULL, 5000);
        if (NULL == ps) {
            LOGE("Cannot create socket");
            break;
        }
        int res = sock_accpet(ps, accept_cb, NULL);
        if (0 != res) {
            LOGE("Cannot accept: %d", res);
            break;
        }

        mp_capture = capture_open("/dev/video0", WIDTH, HEIGHT, FMT);
        if (NULL == mp_capture) {
            cout << "cannot open capture" << endl;
            break;
        }
		mp_compress = vc_open(WIDTH, HEIGHT, FPS);
        if (NULL == mp_compress) {
            cout << "Cannot open compress" << endl;
            break;
        }

        // wait
        scanf("%c", (char *)&res);
    } while (0);
    
    // free
    if (NULL != mp_compress)
        vc_close(mp_compress);
    if (NULL != mp_capture)
        capture_close(mp_capture);
    if (NULL != ps)
        sock_close(&ps);

    return 0;
}


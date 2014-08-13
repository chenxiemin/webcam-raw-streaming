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
#include <iostream>
#include <string.h>
#include <SDL/SDL.h>

#include "sock.h"
#include "log.h"
#include "types.h"
#include "capture.h"
#include "vcompress.h"
#include "decode_context.h"

using namespace std;

#define WIDTH 640
#define HEIGHT 480
#define FMT PIX_FMT_YUV420P
#define FPS 30
#define FF_REFRESH_EVENT (SDL_USEREVENT + 1)

PSocketContext ps = NULL;
PDecodeContext pdc = NULL;
// thread
volatile bool isrun = true;
pthread_t pid = 0;
// sdl
SDL_Surface *screen = NULL;
SDL_Overlay *bmp = NULL;
struct SwsContext *sws_ctx = NULL;
// sync
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void *thd_hook(void *tag)
{
    const char *buf = "chenxiemin";
    char recvbuf[65535];
    while (isrun) {
        // recv data
        sock_send(ps, buf, strlen(buf));
        int recvlen = 0;
        while (1) {
            recvlen = sock_recv(ps, recvbuf, 65535);
            if (recvlen < 0)
                continue;
            break;
        }
        LOGD("Receive buffer %d", recvlen);

        // decode
        int res = dc_decode(pdc, (uint8_t *)recvbuf, recvlen);
        if (1 != res) {
            cout << "no decodeced frame: " << res << endl;
            continue;
        }
        // show
        // lock first
        pthread_mutex_lock(&mutex);
        // notify to show
        SDL_Event event;
        event.type = FF_REFRESH_EVENT;
        SDL_PushEvent(&event);
        // wait consume
        cout << "Waiting for data consume" << endl;
        pthread_cond_wait(&cond, &mutex);
        pthread_mutex_unlock(&mutex);
        cout << "After waiting for data consume isrun " << isrun << endl;
    }
}

void sdl_show(AVFrame *frame)
{
    // show
    SDL_LockYUVOverlay(bmp);

    AVPicture pict;
    pict.data[0] = bmp->pixels[0];
    pict.data[1] = bmp->pixels[2];
    pict.data[2] = bmp->pixels[1];

    pict.linesize[0] = bmp->pitches[0];
    pict.linesize[1] = bmp->pitches[2];
    pict.linesize[2] = bmp->pitches[1];
    sws_scale(sws_ctx, (uint8_t const * const *)frame->data,
            frame->linesize, 0, HEIGHT, pict.data,
            pict.linesize);

    SDL_UnlockYUVOverlay(bmp);

    SDL_Rect rect;
    rect.x = 0;
    rect.y = 0;
    rect.w = WIDTH;
    rect.h = HEIGHT;
    SDL_DisplayYUVOverlay(bmp, &rect);
}

void sdl_loop()
{
    SDL_Event event;
    bool needQuit = false;
    while (true) {
        SDL_WaitEvent(&event);
        switch (event.type) {
        case SDL_QUIT:
            needQuit = true; // delay quit
            break;
        case FF_REFRESH_EVENT:
        {
            cout << "Got FF_REFRESH_EVENT" << endl;
            if (needQuit) {
                cout << "Quit in FF_REFRESH_EVENT" << endl;
                if (0 != pid)
                    isrun = false;
            }

            AVFrame *frame = dc_get_frame(pdc);
            sdl_show(frame);

            // lock to prevent signal before wait
            pthread_mutex_lock(&mutex);
            pthread_cond_signal(&cond);
            pthread_mutex_unlock(&mutex);
            // quit
            if (needQuit) {
                if (0 != pid) {
                    cout << "join thread: " << pid << endl;
                    pthread_join(pid, NULL);
                }
                SDL_Quit();
                return;
            }
            break;
        }
        default:
            break;
        }
    }
}

int main()
{
    do {
        // init socket
        ps = sock_open("127.0.0.1", 5000);
        if (NULL == ps) {
            LOGE("Cannot create socket");
            break;
        }
        int res = sock_connect(ps);
        if (0 != res) {
            LOGE("Cannot connect socket: %d", res);
            break;
        }
        // init dc
        pdc = dc_open(WIDTH, HEIGHT, FMT);
        if (NULL == pdc) {
            cout << "Cannot open dc" << endl;
            break;
        }
        // int sdl
        if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
            cout << "Could not initialize SDL: " << SDL_GetError() << endl;
            break;
        }
        screen = SDL_SetVideoMode(WIDTH, HEIGHT, 0, 0);
        if (NULL == screen) {
            cout << "Cannot create sdl screen" << endl;
            break;
        }
        bmp = SDL_CreateYUVOverlay(WIDTH, HEIGHT, SDL_YV12_OVERLAY, screen);
        sws_ctx = sws_getContext(WIDTH, HEIGHT, FMT, WIDTH, HEIGHT, FMT,
                SWS_BILINEAR, NULL, NULL, NULL);

        // codec loop
        res = pthread_create(&pid, NULL, thd_hook, NULL);
        if (0 != res) {
            cout << "Cannot create thread: " << res << endl;
            break;
        }

        // wait
        // scanf("%c", (char *)&res);
        sdl_loop();

        /*
        for (int i = 0; i < 10; i++) {
            const char *buf = "chenxiemin";
            sock_send(ps, buf, strlen(buf));

            while (1) {
                res = sock_recv(ps, recvbuf, 65535);
                if (res < 0)
                    continue;
                LOGD("Receive buffer %d", res);
                break;
            }
        }
        */
    } while (0);

    if (NULL != pdc)
        dc_close(&pdc);
    sock_close(&ps);
    return 0;
}


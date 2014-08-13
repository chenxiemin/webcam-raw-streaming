/*
 * =====================================================================================
 *
 *       Filename:  direct_rnd.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/12/2014 06:48:13 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  xieminchen (mtk), hs_xieminchen@mediatek.com
 *        Company:  Mediatek
 *
 * =====================================================================================
 */

#include <iostream>
#include <SDL/SDL.h>

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

// codecs
void *mp_capture = NULL;
void *mp_compress = NULL;
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
    Picture pic;

    while (isrun) {
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

        res = dc_decode(pdc, (uint8_t *)outbuf, outlen);
        if (1 != res) {
            cout << "no decodeced frame: " << res << endl;
            continue;
        }

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
    return NULL;
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
        // init
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
        int res = pthread_create(&pid, NULL, thd_hook, NULL);
        if (0 != res) {
            cout << "Cannot create thread: " << res << endl;
            break;
        }

        // wait
        // scanf("%c", (char *)&res);
        sdl_loop();
    } while (0);

    // free
    if (NULL != mp_compress)
        vc_close(mp_compress);
    if (NULL != mp_capture)
        capture_close(mp_capture);
    if (NULL != pdc)
        dc_close(&pdc);

    return 0;
}


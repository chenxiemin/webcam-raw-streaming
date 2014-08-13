/*
 * =====================================================================================
 *
 *       Filename:  decode_context.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/12/2014 08:55:00 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  xieminchen (mtk), hs_xieminchen@mediatek.com
 *        Company:  Mediatek
 *
 * =====================================================================================
 */
#include <iostream>

using namespace std;

#define LOGE printf;

#include "decode_context.h"

struct DecodeContext {
    AVCodecContext *av_codec_context_;
    AVCodec *codec;
    AVFrame *picture;
    uint8_t *picbuf;
};

PDecodeContext dc_open(int width, int height, PixelFormat fmt)
{
    avcodec_register_all();

    PDecodeContext pdc = NULL;
    do {
        // alloc context
        pdc = (PDecodeContext)malloc(sizeof(DecodeContext));
        if (NULL == pdc) {
            LOGE("Cannot alloc context");
            break;
        }
        memset(pdc, 0, sizeof(*pdc));

        // init context
        pdc->codec = avcodec_find_decoder(CODEC_ID_H264);
        if (NULL == pdc->codec) {
            LOGE("Cannot create codec");
            break;
        }
        pdc->av_codec_context_ = avcodec_alloc_context3(pdc->codec);
        if (NULL == pdc->av_codec_context_) {
            LOGE("Cannot create codec context");
            break;
        }
        pdc->av_codec_context_->width = width;
        pdc->av_codec_context_->height = height;
        pdc->av_codec_context_->extradata = NULL;
        pdc->av_codec_context_->pix_fmt = fmt;
        int res = avcodec_open2(pdc->av_codec_context_, pdc->codec, NULL);
        if (0 != res) {
            LOGE("Cannot open codec: ");
            break;
        }

        // init picture
        pdc->picture = av_frame_alloc();
        if (NULL == pdc->picture) {
            LOGE("Cannot alloc frame");
            break;
        }
        int size = avpicture_get_size(fmt, width, height);
        pdc->picbuf = (uint8_t *)av_malloc(size);
        avpicture_fill((AVPicture *)pdc->picture, pdc->picbuf, fmt, width, height);

        return pdc;
    } while (0);

    if (NULL != pdc)
        dc_close(&pdc);
    return NULL;
}

void dc_close(PDecodeContext *ppdc)
{
    if (NULL == ppdc || NULL == *ppdc)
        return;

    PDecodeContext pdc = *ppdc;
    if (NULL != pdc->av_codec_context_) {
        avcodec_close(pdc->av_codec_context_);
        av_free(pdc->av_codec_context_);
    }
    if (NULL != pdc->picture)
        av_free(pdc->picture);
    if (NULL != pdc->picbuf)
        av_free(pdc->picbuf);

    free(pdc);
    *ppdc = NULL;
}

int dc_decode(PDecodeContext pdc, uint8_t *buffer, int len)
{
    AVPacket pkt;
    av_new_packet(&pkt, len);
    pkt.data = buffer;
    pkt.size = len;

    int frameFinished = 0;
    int res = avcodec_decode_video2(pdc->av_codec_context_,
            pdc->picture, &frameFinished, &pkt);
    if (res < 0) {
        cout << "cannot decode video2: " << res << endl;
        return -1;
    }

    return frameFinished;
}

AVFrame *dc_get_frame(PDecodeContext pdc)
{
    return pdc->picture;
}


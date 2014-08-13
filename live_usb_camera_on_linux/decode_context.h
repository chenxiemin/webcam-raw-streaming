/*
 * =====================================================================================
 *
 *       Filename:  decode_context.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/12/2014 08:53:53 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  xieminchen (mtk), hs_xieminchen@mediatek.com
 *        Company:  Mediatek
 *
 * =====================================================================================
 */

#ifndef _LIVE_WEBCAM_DECODE_CONTEXT_H
#define _LIVE_WEBCAM_DECODE_CONTEXT_H

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

typedef struct DecodeContext *PDecodeContext;

PDecodeContext dc_open(int width, int height, PixelFormat fmt);
void dc_close(PDecodeContext *pdc);
int dc_decode(PDecodeContext pdc, uint8_t *buffer, int len);
AVFrame *dc_get_frame(PDecodeContext pdc);

#endif


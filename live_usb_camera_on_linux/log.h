/*
 * =====================================================================================
 *
 *       Filename:  log.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/13/2014 02:06:07 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  xieminchen (mtk), hs_xieminchen@mediatek.com
 *        Company:  Mediatek
 *
 * =====================================================================================
 */

#ifndef _LIVE_WEBCAM_LOG_H
#define _LIVE_WEBCAM_LOG_H

#include <stdio.h>

#define LOGD(...) do { \
                        fprintf(stderr, __VA_ARGS__); \
                        fprintf(stderr, "\n"); \
                        fflush(stderr); \
                    } while (0)

#define LOGE(...) do { \
                        fprintf(stderr, __VA_ARGS__); \
                        fprintf(stderr, "\n"); \
                        fflush(stderr); \
                    } while (0)

#define LOGI(...) do { \
                        fprintf(stderr, __VA_ARGS__); \
                        fprintf(stderr, "\n"); \
                        fflush(stderr); \
                    } while (0)

#endif


/*
 * =====================================================================================
 *
 *       Filename:  sock.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/13/2014 02:13:15 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  xieminchen (mtk), hs_xieminchen@mediatek.com
 *        Company:  Mediatek
 *
 * =====================================================================================
 */

#ifndef _LIVE_WEBCAM_SOCK_H
#define _LIVE_WEBCAM_SOCK_H
// #ifdef __cplusplus
// extern "C" {
// #endif

typedef struct SocketContext *PSocketContext;
typedef void (*SocketAcceptHandler)(PSocketContext sock , void *client, void *tag);

PSocketContext sock_open(const char *add, int port);
void sock_close(PSocketContext *pps);
int sock_accpet(PSocketContext ps, SocketAcceptHandler cb, void *tag);
int sock_connect(PSocketContext ps);
int sock_send(PSocketContext ps, const char *buf, int len);
int sock_recv(PSocketContext ps, char *buf, int len);
int sock_fd(PSocketContext ps);

// #ifdef __cplusplus
// }
// #endif

#endif


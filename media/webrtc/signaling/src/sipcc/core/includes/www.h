






































#ifndef _WWW_H_
#define _WWW_H_

#define MAXCHARSPERLINE 400

typedef struct {
    int soc;
    unsigned char buf[MAXCHARSPERLINE];
    unsigned char text[MAXCHARSPERLINE];
    int idx, n, clen, send;
} WWWRBuf;

extern int Connect2WWW(char *hostName);
extern int Connect2WWWIPPort(unsigned long nIPAddr, unsigned short nPort,
                             char *hostName);
extern void wwwSend(WWWRBuf *wrb, char *fmt, ...);
extern boolean www_proxy_used(int connection);
extern boolean www_socket_open(int connection);
extern void HTTP_Init_ConnTbl();

#endif

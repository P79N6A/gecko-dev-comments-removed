






































#ifndef _NS_SOCKET_H_
#define _NS_SOCKET_H_

#ifndef _WINDOWS
#include <sys/time.h>
#endif

class nsSocket
{
public:
    nsSocket(char *aHost, int aPort);
    nsSocket(char *aHost, int aPort, int (*aEventPumpCB)(void) );
    ~nsSocket();




    enum
    {
        OK              = 0,
        E_USER_CANCEL   = -813, 
        E_PARAM         = -1001,
        E_MEM           = -1002,
        E_INVALID_HOST  = -1003,
        E_SOCK_OPEN     = -1004,
        E_SOCK_CLOSE    = -1005,
        E_TIMEOUT       = -1006,
        E_WRITE         = -1007,
        E_READ_MORE     = -1008,
        E_READ          = -1009,
        E_SMALL_BUF     = -1010,
        E_EOF_FOUND     = -1011,
        E_BIND          = -1012,
        E_LISTEN        = -1014,
        E_ACCEPT        = -1015,
        E_GETSOCKNAME   = -1016,
        E_WINSOCK       = -1017,
        E_INVALID_ADDR  = -1018
    };




    int Open();
    int SrvOpen(); 
    int SrvAccept(); 
    int Send(unsigned char *aBuf, int *aBufSize);
    int Recv(unsigned char *aBuf, int *aBufSize);
    int Recv(unsigned char *aBuf, int *aBufSize, int aTimeoutThresholdUsecs);
    int Close();

    int GetHostPortString(char **aHostPort); 

    static 
    float CalcRate(struct timeval *aPre, struct timeval *aPost, int aBytes);

private:
    int   (*mEventPumpCB)(void);
    char  *mHost;
    int   mPort;
    int   mFd; 
    int   mListenFd; 

    int   IsIPAddress(char *aAddress);
};




#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE 
#define FALSE 0
#endif

#ifndef NULL
#define NULL 0
#endif

#ifdef DUMP
#undef DUMP
#endif
#if defined(DEBUG) || defined(DEBUG_sgehani)
#define DUMP(_vargs)                       \
do {                                       \
    printf("%s %d: ", __FILE__, __LINE__); \
    printf _vargs;                         \
} while (0);
#else
#define DUMP(_vargs) 
#endif

#endif 

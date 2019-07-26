






































#ifndef _csi_platform_h
#define _csi_platform_h

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400  // This prevents weird "'TryEnterCriticalSection': identifier not found"
                             
#endif

#define UINT8 UBLAH_IGNORE_ME_PLEASE
#define INT8 BLAH_IGNORE_ME_PLEASE
#include <winsock2.h>
#undef UINT8
#undef INT8
#include <r_types.h>

#define strcasecmp _stricmp
#define strncasecmp _strnicmp

#define strcasestr stristr


size_t strlcat(char *dst, const char *src, size_t siz);


int getopt(int argc, char *argv[], char *opstring);
extern char *optarg;
extern int optind;
extern int opterr;


int gettimeofday(struct timeval *tv, void *tz);

#ifdef NR_SOCKET_IS_VOID_PTR
typedef void* NR_SOCKET;
#else
typedef SOCKET NR_SOCKET;
#define NR_SOCKET_READ(sock,buf,count)   recv((sock),(buf),(count),0)
#define NR_SOCKET_WRITE(sock,buf,count)  send((sock),(buf),(count),0)
#define NR_SOCKET_CLOSE(sock)            closesocket(sock)
#endif

#define EHOSTUNREACH    WSAEHOSTUNREACH

#define LOG_EMERG       0
#define LOG_ALERT       1
#define LOG_CRIT        2
#define LOG_ERR         3
#define LOG_WARNING     4
#define LOG_NOTICE      5
#define LOG_INFO        6
#define LOG_DEBUG       7



#define IFNAMSIZ        256  /* big enough for FriendlyNames */
#define in_addr_t   UINT4

#ifndef strlcpy
#define strlcpy(a,b,c) \
        (strncpy((a),(b),(c)), \
         ((c)<= 0 ? 0 : ((a)[(c)-1]='\0')), \
         strlen((b)))
#endif

#endif  


































































#ifndef prinet_h__
#define prinet_h__

#if defined(XP_UNIX) || defined(XP_OS2) || defined(XP_BEOS)
#include <sys/types.h>
#include <sys/socket.h>		
#include <netinet/in.h>         
#ifdef XP_OS2
#include <sys/ioctl.h>
#endif
#ifdef XP_UNIX
#ifdef AIX






struct ether_addr;
struct sockaddr_dl;
#endif 
#include <arpa/inet.h>
#endif 
#include <netdb.h>

#if defined(FREEBSD) || defined(BSDI) || defined(QNX)
#include <rpc/types.h> 
#endif





#if defined(OS2) && !defined(INADDR_LOOPBACK)
#define INADDR_LOOPBACK 0x7f000001
#endif





#if defined(BSDI) || defined(OSF1)
#include <machine/endian.h>
#endif

#elif defined(WIN32)










#else

#error Unknown platform

#endif

#endif 

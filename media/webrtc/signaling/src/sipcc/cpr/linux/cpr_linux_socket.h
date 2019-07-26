



#ifndef _CPR_LINUX_SOCKET_H_
#define _CPR_LINUX_SOCKET_H_

#include "cpr_types.h"
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <arpa/inet.h>




#ifdef CPR_USE_SOCKETPAIR
#undef CPR_USE_SOCKETPAIR
#endif
#define SUPPORT_CONNECT_CONST const




#define SOCKET_ERROR   (-1)




#define INVALID_SOCKET (-1)




typedef int cpr_socket_t;




typedef socklen_t cpr_socklen_t;











#ifndef AF_UNIX
#define AF_UNIX  AF_LOCAL
#endif


































#define SO_DONTLINGER       ((int)(~SO_LINGER))
#define SO_EXCLUSIVEADDRUSE ((int)(~SO_REUSEADDR))
































#ifndef IPPROTO_IPV4
#define IPPROTO_IPV4         4
#endif

#ifndef IPPROTO_IPIP
#define IPPROTO_IPIP  IPPROTO_IPV4
#endif


#define IPPROTO_IGRP        88  /* Cisco/GXS IGRP */
#define IPPROTO_EIGRP       88
#define IPPROTO_IPCOMP     108  /* IP payload compression */





#ifndef IPPROTO_IPV6
#define IPPROTO_HOPOPTS      0  /* IPv6 hop-by-hop options */
#define IPPROTO_IPV6        41  /* IPv6 */
#define IPPROTO_ROUTING     43  /* IPv6 routing header */
#define IPPROTO_FRAGMENT    44  /* IPv6 fragmentation header */
#define IPPROTO_ICMPV6      58  /* ICMPv6 */
#define IPPROTO_NONE        59  /* IPv6 no next header */
#define IPPROTO_DSTOPTS     60  /* IPv6 destination options */
#endif





































































#define IPPORT_SCCP       2000





#define CIPPORT_EPH_LOW         0xC000
#define CIPPORT_EPH_HI          0xCFFF




































typedef uint16_t        sa_family_t;

typedef struct
{
    sa_family_t sun_family;  
    char        sun_path[108];
} cpr_sockaddr_un_t;





#define _SS_MAXSIZE     256     /* Implementation specific max size */

#define cpr_sun_len(a) sizeof(a)
void cpr_set_sockun_addr(cpr_sockaddr_un_t *addr, const char *name, pid_t pid);














typedef double          sockaddr_maxalign_t;

#define _SS_ALIGNSIZE   (sizeof (sockaddr_maxalign_t))




#define _SS_PAD1SIZE    (_SS_ALIGNSIZE - sizeof (sa_family_t))
#define _SS_PAD2SIZE    (_SS_MAXSIZE - (sizeof (sa_family_t)+ \
                        _SS_PAD1SIZE + _SS_ALIGNSIZE))

#ifndef __cplusplus
typedef struct cpr_sockaddr_storage sockaddr_storage;
#endif

























#define SO_MAX_MSG_SIZE   0x2003    /* maximum message size */


#define SO_NBIO                 0x0400  /* Nonblocking socket I/O operation */
#define SO_ASYNC                0x0800  /* should send asyn notification of
                                         * I/O events */
#define SO_VRFTABLEID           0x1000  /* set VRF routing table id */
#define SO_SRC_SPECIFIED        0x2000  /* Specified Source Address to be used */
#define SO_STRICT_ADDR_BIND     0x4000  /* Accept only those packets that have
                                         * been sent to the address that this
                                         * socket is bound to */



#define TCP_PORT_RETRY_CNT      5
#define TCP_PORT_MASK           0xfff

#endif

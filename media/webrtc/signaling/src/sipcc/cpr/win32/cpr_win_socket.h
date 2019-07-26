



#ifndef _CPR_WIN_SOCKET_H_
#define _CPR_WIN_SOCKET_H_

#include "cpr_types.h"








#define SOL_TCP                 0x02
#define SOL_IP                  0x03
#define SOL_UDP                 0x04






typedef uint16_t        sa_family_t;








#ifndef AF_LOCAL
#define AF_LOCAL 1
#endif
typedef struct
{
    sa_family_t	    sun_family;
    int8_t	    sun_path[108];
} cpr_sockaddr_un_t;

#define cpr_sun_len(a) sizeof(a)

void cpr_set_sockun_addr(cpr_sockaddr_un_t *addr, const char *name, pid_t pid);











#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)

typedef unsigned int    u_int;
#define Socket SOCKET
typedef int cpr_socklen_t;
typedef SOCKET cpr_socket_t;
typedef unsigned long u_long;
typedef unsigned short u_short;

#define SUPPORT_CONNECT_CONST const

#ifdef CPR_USE_SOCKETPAIR
#undef CPR_USE_SOCKETPAIR
#endif

#define MAX_RETRY_FOR_EAGAIN 10
#endif

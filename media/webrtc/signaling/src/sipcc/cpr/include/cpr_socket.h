



#ifndef _CPR_SOCKET_H_
#define _CPR_SOCKET_H_

#include "cpr_types.h"
#include "cpr_time.h"

__BEGIN_DECLS




#define CPR_USE_SOCKETS

#if defined SIP_OS_LINUX
#include "../linux/cpr_linux_socket.h"
#elif defined SIP_OS_WINDOWS
#include "../win32/cpr_win_socket.h"
#elif defined SIP_OS_OSX
#include "../darwin/cpr_darwin_socket.h"
#endif





typedef enum
{
    CPR_SHUTDOWN_RECEIVE,
    CPR_SHUTDOWN_SEND,
    CPR_SHUTDOWN_BOTH
} cpr_shutdown_e;





typedef struct
{
    uint16_t sa_family;
    int8_t   sa_data[14];
} cpr_sockaddr_t;

typedef struct
{
    uint16_t         sin_family;
    uint16_t         sin_port;
    struct in_addr_s sin_addr;
    
    uint8_t          sin_zero[sizeof(cpr_sockaddr_t) -
                              sizeof(struct in_addr_s) -
                              sizeof(uint16_t) * 2];
} cpr_sockaddr_in_t;


typedef struct
{
     uint16_t       sin6_family;
     uint16_t       sin6_port;
     uint32_t       sin6_flowinfo;
     cpr_in6_addr_t sin6_addr;
     uint32_t       sin6_scope_id;
} cpr_sockaddr_in6_t;

typedef struct sockaddr_storage cpr_sockaddr_storage; 





#ifdef CPR_USE_SOCKETS

























cpr_status_e
cprBind(cpr_socket_t socket,
        CONST cpr_sockaddr_t * RESTRICT addr,
        cpr_socklen_t addr_len);



















cpr_status_e
cprCloseSocket(cpr_socket_t socket);




































cpr_status_e
cprConnect(cpr_socket_t socket,
           SUPPORT_CONNECT_CONST cpr_sockaddr_t * RESTRICT addr,
           cpr_socklen_t addr_len);





























cpr_status_e
cprGetSockName(cpr_socket_t socket,
               cpr_sockaddr_t * RESTRICT addr,
               cpr_socklen_t * RESTRICT addr_len);





























cpr_status_e
cprListen(cpr_socket_t socket,
          uint16_t backlog);













































ssize_t
cprRecv(cpr_socket_t socket,
        void * RESTRICT buf,
        size_t len,
        int32_t flags);















































ssize_t
cprRecvFrom(cpr_socket_t socket,
            void * RESTRICT buf,
            size_t len,
            int32_t flags,
            cpr_sockaddr_t * RESTRICT from,
            cpr_socklen_t * RESTRICT from_len);












































































int16_t
cprSelect(uint32_t nfds,
          fd_set * RESTRICT read_fds,
          fd_set * RESTRICT write_fds,
          fd_set * RESTRICT except_fds,
          struct cpr_timeval * RESTRICT timeout);























































































ssize_t
cprSend(cpr_socket_t socket,
        CONST void *buf,
        size_t len,
        int32_t flags);











































ssize_t
cprSendTo(cpr_socket_t socket,
          CONST void *msg,
          size_t len,
          int32_t flags,
          CONST cpr_sockaddr_t *dest_addr,
          cpr_socklen_t dest_len);




















































cpr_status_e
cprSetSockOpt(cpr_socket_t socket,
              uint32_t level,
              uint32_t opt_name,
              CONST void *opt_val,
              cpr_socklen_t opt_len);

















cpr_status_e
cprSetSockNonBlock(cpr_socket_t socket);











































cpr_socket_t
cprSocket(uint32_t domain,
          uint32_t type,
          uint32_t protocol);












int
cpr_inet_pton (int af, const char *src, void *dst);


#endif 

__END_DECLS

#endif

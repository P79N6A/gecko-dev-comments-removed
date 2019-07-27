



#ifndef __SIP_SOCKET_API_H__
#define __SIP_SOCKET_API_H__

#include "cpr.h"
#include "cpr_socket.h"



















ssize_t
sipSocketSend (cpr_socket_t soc,
         CONST void *buf,
         size_t len,
         int32_t flags,
         boolean secure);

















ssize_t
sipSocketRecv (cpr_socket_t soc,
         void * RESTRICT buf,
         size_t len,
         int32_t flags,
         boolean secure);



















cpr_status_e
sipSocketClose (cpr_socket_t soc,
                boolean secure);
#endif

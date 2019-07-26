






































#include "cpr.h"
#include "cpr_socket.h"
#include "errno.h"
#include "plat_api.h"



















ssize_t
sipSocketSend (cpr_socket_t soc,
         CONST void *buf,
         size_t len, 
         int32_t flags,
         boolean secure)
{



      return cprSend(soc, buf, len, flags);

}

















ssize_t
sipSocketRecv (cpr_socket_t soc,
         void * RESTRICT buf,
         size_t len,
         int32_t flags,
         boolean secure)
{



      return cprRecv(soc, buf, len, flags);

}



















cpr_status_e
sipSocketClose (cpr_socket_t soc,
                boolean secure)
{



      return cprCloseSocket(soc);

}









































#include "cpr.h"
#include "cpr_socket.h"
#include "errno.h"
#include "platform_api.h"



















ssize_t
platSecSocSend (cpr_socket_t soc,
         CONST void *buf,
         size_t len )
{
    return cprSend(soc, buf, len, 0);
}





























ssize_t
platSecSocRecv (cpr_socket_t soc,
         void * RESTRICT buf,
         size_t len)
{
    return cprRecv(soc, buf, len, 0);
}
















cpr_status_e
platSecSocClose (cpr_socket_t soc)
{
    return cprCloseSocket(soc);
}

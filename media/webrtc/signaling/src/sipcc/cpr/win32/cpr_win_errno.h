






































#ifndef _CPR_CNU_ERRNO_H_
#define _CPR_CNU_ERRNO_H_

#include <errno.h>


#ifndef EWOULDBLOCK
#define EWOULDBLOCK             WSAEWOULDBLOCK
#endif
#ifndef EINPROGRESS
#define EINPROGRESS             WSAEINPROGRESS
#endif
#ifndef ENOTCONN
#define ENOTCONN                WSAENOTCONN
#endif




#define cpr_errno cprTranslateErrno()

int16_t cprTranslateErrno(void);

#endif





#ifndef _CPR_LINUX_ERRNO_H_
#define _CPR_LINUX_ERRNO_H_

#include <errno.h>






#define cpr_errno cprTranslateErrno()

int16_t cprTranslateErrno(void);

#endif





#ifndef _CPR_DARWIN_ERRNO_H_
#define _CPR_DARWIN_ERRNO_H_

#include <errno.h>






#define cpr_errno cprTranslateErrno()

int16_t cprTranslateErrno(void);

#endif

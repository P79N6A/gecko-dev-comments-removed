



#ifndef BASE_OS_COMPAT_NACL_H_
#define BASE_OS_COMPAT_NACL_H_

#include <sys/types.h>

#if !defined (__GLIBC__)

extern "C" time_t timegm(struct tm* const t);
#endif  

#endif  


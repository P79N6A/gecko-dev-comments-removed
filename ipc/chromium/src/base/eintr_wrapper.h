








#ifndef BASE_EINTR_WRAPPER_H_
#define BASE_EINTR_WRAPPER_H_

#include "build/build_config.h"

#if defined(OS_POSIX)

#include <errno.h>

#define HANDLE_EINTR(x) ({ \
  typeof(x) __eintr_result__; \
  do { \
    __eintr_result__ = x; \
  } while (__eintr_result__ == -1 && errno == EINTR); \
  __eintr_result__;\
})

#else

#define HANDLE_EINTR(x) x

#endif  

#endif  

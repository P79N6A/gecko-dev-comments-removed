




























#ifndef COMMON_LINUX_EINTR_WRAPPER_H_
#define COMMON_LINUX_EINTR_WRAPPER_H_

#include <errno.h>





#define HANDLE_EINTR(x) ({ \
  typeof(x) __eintr_result__; \
  do { \
    __eintr_result__ = x; \
  } while (__eintr_result__ == -1 && errno == EINTR); \
  __eintr_result__;\
})

#endif  

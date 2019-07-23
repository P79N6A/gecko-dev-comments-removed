



































#ifndef config_h___
#define config_h___

#define MAX_STACK_CRAWL 200

#include <malloc.h>

#if defined(linux) || defined(NTO)
#define USE_BFD
#undef NEED_WRAPPERS

#define REAL_MALLOC(_x) __libc_malloc(_x)
#define REAL_REALLOC(_x,_y) __libc_realloc(_x,_y)
#define REAL_FREE(_x) __libc_free(_x)

extern "C" {
  void* __libc_malloc(size_t);
  void* __libc_realloc(void*, size_t);
  void __libc_free(void*);
}

#endif 

#endif 




































#ifndef libmalloc_h___
#define libmalloc_h___

#include <sys/types.h>
#include <malloc.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"

typedef unsigned long u_long;



struct malloc_log_entry {
  u_long delTime;
  u_long numpcs;
  char* pcs[MAX_STACK_CRAWL];
};


#define malloc_log_stack   7



struct malloc_map_entry {
  u_long nameLen;
  u_long address;		
};




extern u_long SetMallocFlags(u_long flags);





#define LIBMALLOC_LOG    0x1


#define LIBMALLOC_NOFREE 0x2


#define LIBMALLOC_CHECK  0x4


#define LIBMALLOC_LOG_RC 0x8


#define LIBMALLOC_LOG_TRACE 0x10

void __log_addref(void* p, int oldrc, int newrc);
void __log_release(void* p, int oldrc, int newrc);

#ifdef __cplusplus
} 
#endif

#endif

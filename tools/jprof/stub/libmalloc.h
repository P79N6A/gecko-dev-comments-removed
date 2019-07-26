



#ifndef libmalloc_h___
#define libmalloc_h___

#include <sys/types.h>
#include <malloc.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"

typedef unsigned long u_long;


#define JP_FIRST_AFTER_PAUSE 1





struct malloc_log_entry {
  u_long delTime;
  u_long numpcs;
  unsigned int flags;
  int thread;
  char* pcs[MAX_STACK_CRAWL];
};



struct malloc_map_entry {
  u_long nameLen;
  u_long address;		
};

#ifdef __cplusplus
} 
#endif

#endif

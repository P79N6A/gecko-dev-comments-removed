






































#ifndef _r_memory_h
#define _r_memory_h

#define R_MALLOC_X 2

#include "r_types.h"

void *r_malloc(int type, size_t size);
void *r_malloc_compat(size_t size);
void *r_calloc(int type,size_t number,size_t size);
void r_free   (void *ptr);
void *r_realloc(void *ptr,size_t size);
char *r_strdup(const char *str);
int r_mem_get_usage(UINT4 *usage);
int r_memory_dump_stats(void);

#ifdef NO_MALLOC_REPLACE

#ifndef RMALLOC
#define RMALLOC(a) malloc(a)
#endif

#ifndef RCALLOC
#define RCALLOC(a) calloc(1,a)
#endif

#ifndef RFREE
#define RFREE(a) if(a) free(a)
#endif

#ifndef RREALLOC
#define RREALLOC(a,b) realloc(a,b)
#endif

#else


#ifndef R_MALLOC_TYPE
#define R_MALLOC_TYPE   0
#endif

#ifndef RMALLOC
#define RMALLOC(a) r_malloc(R_MALLOC_TYPE,a)
#endif

#ifndef RCALLOC
#define RCALLOC(a) r_calloc(R_MALLOC_TYPE,1,a)
#endif

#ifndef RFREE
#define RFREE(a) if(a) r_free(a)
#endif

#ifndef RREALLOC
#define RREALLOC(a,b) r_realloc(a,b)
#endif

#endif


#endif


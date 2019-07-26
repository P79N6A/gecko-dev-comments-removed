






































#ifndef _CPR_LINUX_MEMORY_API_H_
#define _CPR_LINUX_MEMORY_API_H_
















#define MALLOC   malloc
#define CALLOC   calloc
#define REALLOC  realloc
#define FREE     free

#ifndef memalign
#define MEMALIGN(align, sz) malloc(sz)
#else
#define MEMALIGN(align, sz) memalign(align, sz)
#endif








void cpr_crashdump(void);

#endif

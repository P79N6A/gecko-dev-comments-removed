









































#ifndef NSTRACEMALLOCCALLBACKS_H
#define NSTRACEMALLOCCALLBACKS_H

#include <stdlib.h>

PR_BEGIN_EXTERN_C


typedef struct stack_buffer_info {
    void **buffer;
    size_t size;
    size_t entries;
} stack_buffer_info;

typedef struct tm_thread tm_thread;
struct tm_thread {
    



    uint32 suppress_tracing;

    
    stack_buffer_info backtrace_buf;
};


tm_thread * tm_get_thread(void);

#ifdef XP_WIN32

PR_EXTERN(void) StartupHooker();
PR_EXTERN(void) ShutdownHooker();


PR_EXTERN(void) MallocCallback(void *aPtr, size_t aSize, PRUint32 start, PRUint32 end, tm_thread *t);
PR_EXTERN(void) CallocCallback(void *aPtr, size_t aCount, size_t aSize, PRUint32 start, PRUint32 end, tm_thread *t);
PR_EXTERN(void) ReallocCallback(void *aPin, void* aPout, size_t aSize, PRUint32 start, PRUint32 end, tm_thread *t);
PR_EXTERN(void) FreeCallback(void *aPtr, PRUint32 start, PRUint32 end, tm_thread *t);


void* dhw_orig_malloc(size_t);
void* dhw_orig_calloc(size_t, size_t);
void* dhw_orig_realloc(void*, size_t);
void dhw_orig_free(void*);

#endif 

PR_END_EXTERN_C

#endif 

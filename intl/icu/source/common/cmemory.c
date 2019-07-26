



















#include "unicode/uclean.h"
#include "cmemory.h"
#include "putilimp.h"
#include "uassert.h"
#include <stdlib.h>


static const int32_t zeroMem[] = {0, 0, 0, 0, 0, 0};


static const void     *pContext;
static UMemAllocFn    *pAlloc;
static UMemReallocFn  *pRealloc;
static UMemFreeFn     *pFree;



static UBool   gHeapInUse;

#if U_DEBUG && defined(UPRV_MALLOC_COUNT)
#include <stdio.h>
static int n=0;
static long b=0; 
#endif

#if U_DEBUG

static char gValidMemorySink = 0;

U_CAPI void uprv_checkValidMemory(const void *p, size_t n) {
    






    const char *s = (const char *)p;
    char c = gValidMemorySink;
    size_t i;
    U_ASSERT(p != NULL);
    for(i = 0; i < n; ++i) {
        c ^= s[i];
    }
    gValidMemorySink = c;
}

#endif  

U_CAPI void * U_EXPORT2
uprv_malloc(size_t s) {
#if U_DEBUG && defined(UPRV_MALLOC_COUNT)
#if 1
  putchar('>');
  fflush(stdout);
#else
  fprintf(stderr,"MALLOC\t#%d\t%ul bytes\t%ul total\n", ++n,s,(b+=s)); fflush(stderr);
#endif
#endif
    if (s > 0) {
        gHeapInUse = TRUE;
        if (pAlloc) {
            return (*pAlloc)(pContext, s);
        } else {
            return uprv_default_malloc(s);
        }
    } else {
        return (void *)zeroMem;
    }
}

U_CAPI void * U_EXPORT2
uprv_realloc(void * buffer, size_t size) {
#if U_DEBUG && defined(UPRV_MALLOC_COUNT)
  putchar('~');
  fflush(stdout);
#endif
    if (buffer == zeroMem) {
        return uprv_malloc(size);
    } else if (size == 0) {
        if (pFree) {
            (*pFree)(pContext, buffer);
        } else {
            uprv_default_free(buffer);
        }
        return (void *)zeroMem;
    } else {
        gHeapInUse = TRUE;
        if (pRealloc) {
            return (*pRealloc)(pContext, buffer, size);
        } else {
            return uprv_default_realloc(buffer, size);
        }
    }
}

U_CAPI void U_EXPORT2
uprv_free(void *buffer) {
#if U_DEBUG && defined(UPRV_MALLOC_COUNT)
  putchar('<');
  fflush(stdout);
#endif
    if (buffer != zeroMem) {
        if (pFree) {
            (*pFree)(pContext, buffer);
        } else {
            uprv_default_free(buffer);
        }
    }
}

U_CAPI void * U_EXPORT2
uprv_calloc(size_t num, size_t size) {
    void *mem = NULL;
    size *= num;
    mem = uprv_malloc(size);
    if (mem) {
        uprv_memset(mem, 0, size);
    }
    return mem;
}

U_CAPI void U_EXPORT2
u_setMemoryFunctions(const void *context, UMemAllocFn *a, UMemReallocFn *r, UMemFreeFn *f,  UErrorCode *status)
{
    if (U_FAILURE(*status)) {
        return;
    }
    if (a==NULL || r==NULL || f==NULL) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    if (gHeapInUse) {
        *status = U_INVALID_STATE_ERROR;
        return;
    }
    pContext  = context;
    pAlloc    = a;
    pRealloc  = r;
    pFree     = f;
}


U_CFUNC UBool cmemory_cleanup(void) {
    pContext   = NULL;
    pAlloc     = NULL;
    pRealloc   = NULL;
    pFree      = NULL;
    gHeapInUse = FALSE;
    return TRUE;
}








U_CFUNC UBool cmemory_inUse() {
    return gHeapInUse;
}


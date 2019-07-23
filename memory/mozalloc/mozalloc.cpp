







































#include <errno.h>
#include <new>                  
#include <string.h>

#if defined(MALLOC_H)
#  include MALLOC_H             
#endif 
#include <stddef.h>             
#include <stdlib.h>             


#define MOZALLOC_DONT_DEFINE_MACRO_WRAPPERS
#include "mozilla/mozalloc.h"
#include "mozilla/mozalloc_oom.h"  


#if defined(__GNUC__) && (__GNUC__ > 2)
#define LIKELY(x)    (__builtin_expect(!!(x), 1))
#define UNLIKELY(x)  (__builtin_expect(!!(x), 0))
#else
#define LIKELY(x)    (x)
#define UNLIKELY(x)  (x)
#endif


void*
moz_xmalloc(size_t size)
{
    void* ptr = malloc(size);
    if (UNLIKELY(!ptr)) {
        mozalloc_handle_oom();
        return moz_xmalloc(size);
    }
    return ptr;
}
void*
moz_malloc(size_t size)
{
    return malloc(size);
}

void*
moz_xcalloc(size_t nmemb, size_t size)
{
    void* ptr = calloc(nmemb, size);
    if (UNLIKELY(!ptr)) {
        mozalloc_handle_oom();
        return moz_xcalloc(nmemb, size);
    }
    return ptr;
}
void*
moz_calloc(size_t nmemb, size_t size)
{
    return calloc(nmemb, size);
}

void*
moz_xrealloc(void* ptr, size_t size)
{
    void* newptr = realloc(ptr, size);
    if (UNLIKELY(!newptr)) {
        mozalloc_handle_oom();
        return moz_xrealloc(ptr, size);
    }
    return newptr;
}
void*
moz_realloc(void* ptr, size_t size)
{
    return realloc(ptr, size);
}

char*
moz_xstrdup(const char* str)
{
    char* dup = strdup(str);
    if (UNLIKELY(!dup)) {
        mozalloc_handle_oom();
        return moz_xstrdup(str);
    }
    return dup;
}
char*
moz_strdup(const char* str)
{
    return strdup(str);
}

#if defined(HAVE_STRNDUP)
char*
moz_xstrndup(const char* str, size_t strsize)
{
    char* dup = strndup(str, strsize);
    if (UNLIKELY(!dup)) {
        mozalloc_handle_oom();
        return moz_xstrndup(str, strsize);
    }
    return dup;
}
char*
moz_strndup(const char* str, size_t strsize)
{
    return strndup(str, strsize);
}
#endif  

#if defined(HAVE_POSIX_MEMALIGN)
int
moz_xposix_memalign(void **ptr, size_t alignment, size_t size)
{
    int err = posix_memalign(ptr, alignment, size);
    if (UNLIKELY(err && ENOMEM == err)) {
        mozalloc_handle_oom();
        return moz_xposix_memalign(ptr, alignment, size);
    }
    
    return err;
}
int
moz_posix_memalign(void **ptr, size_t alignment, size_t size)
{
    return posix_memalign(ptr, alignment, size);
}
#endif 

#if defined(HAVE_MEMALIGN)
void*
moz_xmemalign(size_t boundary, size_t size)
{
    void* ptr = memalign(boundary, size);
    if (UNLIKELY(!ptr && EINVAL != errno)) {
        mozalloc_handle_oom();
        return moz_xmemalign(boundary, size);
    }
    
    return ptr;
}
void*
moz_memalign(size_t boundary, size_t size)
{
    return memalign(boundary, size);
}
#endif 

#if defined(HAVE_VALLOC)
void*
moz_xvalloc(size_t size)
{
    void* ptr = valloc(size);
    if (UNLIKELY(!ptr)) {
        mozalloc_handle_oom();
        return moz_xvalloc(size);
    }
    return ptr;
}
void*
moz_valloc(size_t size)
{
    return valloc(size);
}
#endif 


namespace mozilla {

const fallible_t fallible = fallible_t();

} 

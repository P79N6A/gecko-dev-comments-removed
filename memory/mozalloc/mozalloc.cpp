







































#include <errno.h>
#include <new>                  
#include <string.h>

#include <sys/types.h>

#if defined(MALLOC_H)
#  include MALLOC_H             
#endif 
#include <stddef.h>             
#include <stdlib.h>             
#if defined(XP_UNIX)
#  include <unistd.h>           
#endif 

#if defined(MOZ_MEMORY)

#  include "jemalloc.h"
#endif

#if defined(XP_WIN) || (defined(XP_OS2) && defined(__declspec))
#  define MOZALLOC_EXPORT __declspec(dllexport)
#endif


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

#ifdef MOZ_MEMORY_DARWIN
#include "jemalloc.h"
#define malloc(a)               je_malloc(a)
#define posix_memalign(a, b, c) je_posix_memalign(a, b, c)
#define valloc(a)               je_valloc(a)
#define calloc(a, b)            je_calloc(a, b)
#define memalign(a, b)          je_memalign(a, b)
#define strdup(a)               je_strdup(a)
#define strndup(a, b)           je_strndup(a, b)




#endif

void
moz_free(void* ptr)
{
    free(ptr);
}

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

#if defined(HAVE_POSIX_MEMALIGN) || defined(HAVE_JEMALLOC_POSIX_MEMALIGN)
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
    int code = posix_memalign(ptr, alignment, size);
    if (code)
        return code;

#if defined(XP_MACOSX)
    
    
    size_t mask = alignment - 1;
    if (((size_t)(*ptr) & mask) != 0) {
        void* old = *ptr;
        code = moz_posix_memalign(ptr, alignment, size);
        free(old);
    }
#endif

    return code;

}
#endif 

#if defined(HAVE_MEMALIGN) || defined(HAVE_JEMALLOC_MEMALIGN)
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

#if defined(HAVE_VALLOC) || defined(HAVE_JEMALLOC_VALLOC)
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

size_t
moz_malloc_usable_size(void *ptr)
{
    if (!ptr)
        return 0;

#if defined(XP_MACOSX)
    return malloc_size(ptr);
#elif defined(MOZ_MEMORY)
    return malloc_usable_size(ptr);
#elif defined(XP_WIN)
    return _msize(ptr);
#else
    return 0;
#endif
}

size_t moz_malloc_size_of(const void *ptr, size_t computedSize)
{
    size_t usable = moz_malloc_usable_size((void *)ptr);
    return usable ? usable : computedSize;
}

namespace mozilla {

const fallible_t fallible = fallible_t();

} 

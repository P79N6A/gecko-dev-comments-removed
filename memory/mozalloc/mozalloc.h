







































#ifndef mozilla_mozalloc_h
#define mozilla_mozalloc_h





#include <stdlib.h>
#include <string.h>
#if defined(__cplusplus)
#  include <new>
#endif
#include "xpcom-config.h"

#define MOZALLOC_HAVE_XMALLOC

#if defined(MOZALLOC_EXPORT)


#elif defined(XP_WIN) || (defined(XP_OS2) && defined(__declspec))
#  define MOZALLOC_EXPORT __declspec(dllimport)
#elif defined(HAVE_VISIBILITY_ATTRIBUTE)


#  define MOZALLOC_EXPORT __attribute__ ((visibility ("default")))
#else
#  define MOZALLOC_EXPORT
#endif


#if defined(NS_ALWAYS_INLINE)
#  define MOZALLOC_INLINE NS_ALWAYS_INLINE inline
#elif defined(HAVE_FORCEINLINE)
#  define MOZALLOC_INLINE __forceinline
#else
#  define MOZALLOC_INLINE inline
#endif


#if defined(__SUNPRO_C) || defined(__SUNPRO_CC)
#  undef NS_WARN_UNUSED_RESULT
#  define NS_WARN_UNUSED_RESULT
#  undef NS_ATTR_MALLOC
#  define NS_ATTR_MALLOC
#endif

#if defined(__cplusplus)
extern "C" {
#endif















MOZALLOC_EXPORT
void moz_free(void* ptr);

MOZALLOC_EXPORT void* moz_xmalloc(size_t size)
    NS_ATTR_MALLOC NS_WARN_UNUSED_RESULT;

MOZALLOC_EXPORT
void* moz_malloc(size_t size)
    NS_ATTR_MALLOC NS_WARN_UNUSED_RESULT;


MOZALLOC_EXPORT void* moz_xcalloc(size_t nmemb, size_t size)
    NS_ATTR_MALLOC NS_WARN_UNUSED_RESULT;

MOZALLOC_EXPORT void* moz_calloc(size_t nmemb, size_t size)
    NS_ATTR_MALLOC NS_WARN_UNUSED_RESULT;


MOZALLOC_EXPORT void* moz_xrealloc(void* ptr, size_t size)
    NS_ATTR_MALLOC NS_WARN_UNUSED_RESULT;

MOZALLOC_EXPORT void* moz_realloc(void* ptr, size_t size)
    NS_ATTR_MALLOC NS_WARN_UNUSED_RESULT;


MOZALLOC_EXPORT char* moz_xstrdup(const char* str)
    NS_ATTR_MALLOC NS_WARN_UNUSED_RESULT;

MOZALLOC_EXPORT char* moz_strdup(const char* str)
    NS_ATTR_MALLOC NS_WARN_UNUSED_RESULT;

MOZALLOC_EXPORT size_t moz_malloc_usable_size(void *ptr);

#if defined(HAVE_STRNDUP)
MOZALLOC_EXPORT char* moz_xstrndup(const char* str, size_t strsize)
    NS_ATTR_MALLOC NS_WARN_UNUSED_RESULT;

MOZALLOC_EXPORT char* moz_strndup(const char* str, size_t strsize)
    NS_ATTR_MALLOC NS_WARN_UNUSED_RESULT;
#endif 


#if defined(HAVE_POSIX_MEMALIGN) || defined(HAVE_JEMALLOC_POSIX_MEMALIGN)
MOZALLOC_EXPORT int moz_xposix_memalign(void **ptr, size_t alignment, size_t size)
    NS_WARN_UNUSED_RESULT;

MOZALLOC_EXPORT int moz_posix_memalign(void **ptr, size_t alignment, size_t size)
    NS_WARN_UNUSED_RESULT;
#endif 


#if defined(HAVE_MEMALIGN) || defined(HAVE_JEMALLOC_MEMALIGN)
MOZALLOC_EXPORT void* moz_xmemalign(size_t boundary, size_t size)
    NS_ATTR_MALLOC NS_WARN_UNUSED_RESULT;

MOZALLOC_EXPORT void* moz_memalign(size_t boundary, size_t size)
    NS_ATTR_MALLOC NS_WARN_UNUSED_RESULT;
#endif 


#if defined(HAVE_VALLOC)
MOZALLOC_EXPORT void* moz_xvalloc(size_t size)
    NS_ATTR_MALLOC NS_WARN_UNUSED_RESULT;

MOZALLOC_EXPORT void* moz_valloc(size_t size)
    NS_ATTR_MALLOC NS_WARN_UNUSED_RESULT;
#endif 


#ifdef __cplusplus
} 
#endif 


#ifdef __cplusplus























#if defined(XP_MACOSX)
#  define MOZALLOC_EXPORT_NEW MOZALLOC_EXPORT
#else
#  define MOZALLOC_EXPORT_NEW
#endif

#if defined(ANDROID) || defined(_MSC_VER)





#define MOZALLOC_THROW_IF_HAS_EXCEPTIONS
#else
#define MOZALLOC_THROW_IF_HAS_EXCEPTIONS throw()
#endif

#ifdef MOZ_CPP_EXCEPTIONS
#define MOZALLOC_THROW_BAD_ALLOC throw(std::bad_alloc)
#else
#define MOZALLOC_THROW_BAD_ALLOC MOZALLOC_THROW_IF_HAS_EXCEPTIONS
#endif

MOZALLOC_EXPORT_NEW MOZALLOC_INLINE
void* operator new(size_t size) MOZALLOC_THROW_BAD_ALLOC
{
    return moz_xmalloc(size);
}

MOZALLOC_EXPORT_NEW MOZALLOC_INLINE
void* operator new(size_t size, const std::nothrow_t&) MOZALLOC_THROW_IF_HAS_EXCEPTIONS
{
    return moz_malloc(size);
}

MOZALLOC_EXPORT_NEW MOZALLOC_INLINE
void* operator new[](size_t size) MOZALLOC_THROW_BAD_ALLOC
{
    return moz_xmalloc(size);
}

MOZALLOC_EXPORT_NEW MOZALLOC_INLINE
void* operator new[](size_t size, const std::nothrow_t&) MOZALLOC_THROW_IF_HAS_EXCEPTIONS
{
    return moz_malloc(size);
}

MOZALLOC_EXPORT_NEW MOZALLOC_INLINE
void operator delete(void* ptr) MOZALLOC_THROW_IF_HAS_EXCEPTIONS
{
    return moz_free(ptr);
}

MOZALLOC_EXPORT_NEW MOZALLOC_INLINE
void operator delete(void* ptr, const std::nothrow_t&) MOZALLOC_THROW_IF_HAS_EXCEPTIONS
{
    return moz_free(ptr);
}

MOZALLOC_EXPORT_NEW MOZALLOC_INLINE
void operator delete[](void* ptr) MOZALLOC_THROW_IF_HAS_EXCEPTIONS
{
    return moz_free(ptr);
}

MOZALLOC_EXPORT_NEW MOZALLOC_INLINE
void operator delete[](void* ptr, const std::nothrow_t&) MOZALLOC_THROW_IF_HAS_EXCEPTIONS
{
    return moz_free(ptr);
}





















namespace mozilla {

struct MOZALLOC_EXPORT fallible_t { };

} 

MOZALLOC_INLINE
void* operator new(size_t size, const mozilla::fallible_t&) MOZALLOC_THROW_IF_HAS_EXCEPTIONS
{
    return moz_malloc(size);
}

MOZALLOC_INLINE
void* operator new[](size_t size, const mozilla::fallible_t&) MOZALLOC_THROW_IF_HAS_EXCEPTIONS
{
    return moz_malloc(size);
}

MOZALLOC_INLINE
void operator delete(void* ptr, const mozilla::fallible_t&) MOZALLOC_THROW_IF_HAS_EXCEPTIONS
{
    moz_free(ptr);
}

MOZALLOC_INLINE
void operator delete[](void* ptr, const mozilla::fallible_t&) MOZALLOC_THROW_IF_HAS_EXCEPTIONS
{
    moz_free(ptr);
}

#endif  


#endif

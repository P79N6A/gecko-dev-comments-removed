







































#ifndef mozilla_mozalloc_h
#define mozilla_mozalloc_h














#if defined(XP_WIN) || (defined(XP_OS2) && defined(__declspec))
#  define MOZALLOC_EXPORT __declspec(dllexport)
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


#if defined(__cplusplus)
extern "C" {
#endif









#if 0
#if !defined(HAVE_POSIX_MEMALIGN) && defined(HAVE_JEMALLOC_POSIX_MEMALIGN)
MOZALLOC_IMPORT int posix_memalign(void **, size_t, size_t)
    NS_WARN_UNUSED_RESULT;
#endif
#endif
















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


#if defined(HAVE_STRNDUP)
MOZALLOC_EXPORT char* moz_xstrndup(const char* str, size_t strsize)
    NS_ATTR_MALLOC NS_WARN_UNUSED_RESULT;

MOZALLOC_EXPORT char* moz_strndup(const char* str, size_t strsize)
    NS_ATTR_MALLOC NS_WARN_UNUSED_RESULT;
#endif 


#if defined(HAVE_POSIX_MEMALIGN)
MOZALLOC_EXPORT int moz_xposix_memalign(void **ptr, size_t alignment, size_t size)
    NS_WARN_UNUSED_RESULT;

MOZALLOC_EXPORT int moz_posix_memalign(void **ptr, size_t alignment, size_t size)
    NS_WARN_UNUSED_RESULT;
#endif 


#if defined(HAVE_MEMALIGN)
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




















MOZALLOC_INLINE
void* operator new(size_t size) throw(std::bad_alloc)
{
    return moz_xmalloc(size);
}

MOZALLOC_INLINE
void* operator new[](size_t size) throw(std::bad_alloc)
{
    return moz_xmalloc(size);
}

MOZALLOC_INLINE
void operator delete(void* ptr) throw()
{
    return free(ptr);
}

MOZALLOC_INLINE
void operator delete[](void* ptr) throw()
{
    return free(ptr);
}





















namespace mozilla {

struct MOZALLOC_EXPORT fallible_t { };

} 

MOZALLOC_INLINE
void* operator new(size_t size, const mozilla::fallible_t&) throw()
{
    return malloc(size);
}

MOZALLOC_INLINE
void* operator new[](size_t size, const mozilla::fallible_t&) throw()
{
    return malloc(size);
}

MOZALLOC_INLINE
void operator delete(void* ptr, const mozilla::fallible_t&) throw()
{
    free(ptr);
}

MOZALLOC_INLINE
void operator delete[](void* ptr, const mozilla::fallible_t&) throw()
{
    free(ptr);
}

#endif  


#endif

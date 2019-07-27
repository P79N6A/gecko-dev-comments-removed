






#ifndef mozilla_mozalloc_h
#define mozilla_mozalloc_h





#include <stdlib.h>
#include <string.h>
#if defined(__cplusplus)
#  include <new>
#endif
#include "xpcom-config.h"

#if defined(__cplusplus)
#include "mozilla/fallible.h"
#include "mozilla/TemplateLib.h"
#endif
#include "mozilla/Attributes.h"
#include "mozilla/Types.h"

#define MOZALLOC_HAVE_XMALLOC

#if defined(MOZ_ALWAYS_INLINE_EVEN_DEBUG)
#  define MOZALLOC_INLINE MOZ_ALWAYS_INLINE_EVEN_DEBUG
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







#ifndef free_impl
#define free_impl free
#define free_impl_
#endif
#ifndef malloc_impl
#define malloc_impl malloc
#define malloc_impl_
#endif













MFBT_API void* moz_xmalloc(size_t size)
    NS_ATTR_MALLOC NS_WARN_UNUSED_RESULT;

MFBT_API void* moz_xcalloc(size_t nmemb, size_t size)
    NS_ATTR_MALLOC NS_WARN_UNUSED_RESULT;

MFBT_API void* moz_xrealloc(void* ptr, size_t size)
    NS_ATTR_MALLOC NS_WARN_UNUSED_RESULT;

MFBT_API char* moz_xstrdup(const char* str)
    NS_ATTR_MALLOC NS_WARN_UNUSED_RESULT;

MFBT_API size_t moz_malloc_usable_size(void *ptr);

MFBT_API size_t moz_malloc_size_of(const void *ptr);

#if defined(HAVE_STRNDUP)
MFBT_API char* moz_xstrndup(const char* str, size_t strsize)
    NS_ATTR_MALLOC NS_WARN_UNUSED_RESULT;
#endif 


#if defined(HAVE_POSIX_MEMALIGN)
MFBT_API int moz_xposix_memalign(void **ptr, size_t alignment, size_t size)
    NS_WARN_UNUSED_RESULT;

MFBT_API int moz_posix_memalign(void **ptr, size_t alignment, size_t size)
    NS_WARN_UNUSED_RESULT;
#endif 


#if defined(HAVE_MEMALIGN)
MFBT_API void* moz_xmemalign(size_t boundary, size_t size)
    NS_ATTR_MALLOC NS_WARN_UNUSED_RESULT;
#endif 


#if defined(HAVE_VALLOC)
MFBT_API void* moz_xvalloc(size_t size)
    NS_ATTR_MALLOC NS_WARN_UNUSED_RESULT;
#endif 


#ifdef __cplusplus
} 
#endif 


#ifdef __cplusplus























#if defined(XP_MACOSX)
#  define MOZALLOC_EXPORT_NEW MFBT_API
#else
#  define MOZALLOC_EXPORT_NEW
#endif

#if defined(ANDROID)






#define MOZALLOC_THROW_IF_HAS_EXCEPTIONS throw()
#define MOZALLOC_THROW_BAD_ALLOC_IF_HAS_EXCEPTIONS
#elif defined(_MSC_VER)



#define MOZALLOC_THROW_IF_HAS_EXCEPTIONS
#define MOZALLOC_THROW_BAD_ALLOC_IF_HAS_EXCEPTIONS
#else
#define MOZALLOC_THROW_IF_HAS_EXCEPTIONS throw()
#define MOZALLOC_THROW_BAD_ALLOC_IF_HAS_EXCEPTIONS throw(std::bad_alloc)
#endif

#define MOZALLOC_THROW_BAD_ALLOC MOZALLOC_THROW_BAD_ALLOC_IF_HAS_EXCEPTIONS

MOZALLOC_EXPORT_NEW
#if defined(__GNUC__) && !defined(__clang__) && defined(__SANITIZE_ADDRESS__)

__attribute__((gnu_inline)) inline
#else
MOZALLOC_INLINE
#endif
void* operator new(size_t size) MOZALLOC_THROW_BAD_ALLOC
{
    return moz_xmalloc(size);
}

MOZALLOC_EXPORT_NEW MOZALLOC_INLINE
void* operator new(size_t size, const std::nothrow_t&) MOZALLOC_THROW_IF_HAS_EXCEPTIONS
{
    return malloc_impl(size);
}

MOZALLOC_EXPORT_NEW MOZALLOC_INLINE
void* operator new[](size_t size) MOZALLOC_THROW_BAD_ALLOC
{
    return moz_xmalloc(size);
}

MOZALLOC_EXPORT_NEW MOZALLOC_INLINE
void* operator new[](size_t size, const std::nothrow_t&) MOZALLOC_THROW_IF_HAS_EXCEPTIONS
{
    return malloc_impl(size);
}

MOZALLOC_EXPORT_NEW MOZALLOC_INLINE
void operator delete(void* ptr) MOZALLOC_THROW_IF_HAS_EXCEPTIONS
{
    return free_impl(ptr);
}

MOZALLOC_EXPORT_NEW MOZALLOC_INLINE
void operator delete(void* ptr, const std::nothrow_t&) MOZALLOC_THROW_IF_HAS_EXCEPTIONS
{
    return free_impl(ptr);
}

MOZALLOC_EXPORT_NEW MOZALLOC_INLINE
void operator delete[](void* ptr) MOZALLOC_THROW_IF_HAS_EXCEPTIONS
{
    return free_impl(ptr);
}

MOZALLOC_EXPORT_NEW MOZALLOC_INLINE
void operator delete[](void* ptr, const std::nothrow_t&) MOZALLOC_THROW_IF_HAS_EXCEPTIONS
{
    return free_impl(ptr);
}





















MOZALLOC_INLINE
void* operator new(size_t size, const mozilla::fallible_t&) MOZALLOC_THROW_IF_HAS_EXCEPTIONS
{
    return malloc_impl(size);
}

MOZALLOC_INLINE
void* operator new[](size_t size, const mozilla::fallible_t&) MOZALLOC_THROW_IF_HAS_EXCEPTIONS
{
    return malloc_impl(size);
}

MOZALLOC_INLINE
void operator delete(void* ptr, const mozilla::fallible_t&) MOZALLOC_THROW_IF_HAS_EXCEPTIONS
{
    free_impl(ptr);
}

MOZALLOC_INLINE
void operator delete[](void* ptr, const mozilla::fallible_t&) MOZALLOC_THROW_IF_HAS_EXCEPTIONS
{
    free_impl(ptr);
}







class InfallibleAllocPolicy
{
public:
    template <typename T>
    T* pod_malloc(size_t aNumElems)
    {
        if (aNumElems & mozilla::tl::MulOverflowMask<sizeof(T)>::value) {
            return nullptr;
        }
        return static_cast<T*>(moz_xmalloc(aNumElems * sizeof(T)));
    }

    template <typename T>
    T* pod_calloc(size_t aNumElems)
    {
        return static_cast<T*>(moz_xcalloc(aNumElems, sizeof(T)));
    }

    template <typename T>
    T* pod_realloc(T* aPtr, size_t aOldSize, size_t aNewSize)
    {
        if (aNewSize & mozilla::tl::MulOverflowMask<sizeof(T)>::value) {
            return nullptr;
        }
        return static_cast<T*>(moz_xrealloc(aPtr, aNewSize * sizeof(T)));
    }

    void free_(void* aPtr)
    {
        free_impl(aPtr);
    }

    void reportAllocOverflow() const
    {
    }
};

#endif  

#ifdef malloc_impl_
#undef malloc_impl_
#undef malloc_impl
#endif
#ifdef free_impl_
#undef free_impl_
#undef free_impl
#endif

#endif 

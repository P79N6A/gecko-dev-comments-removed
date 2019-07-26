



#ifndef mozmemory_wrap_h
#define mozmemory_wrap_h












































































































#ifndef MOZ_MEMORY
#  error Should only include mozmemory_wrap.h when MOZ_MEMORY is set.
#endif

#if defined(MOZ_JEMALLOC_IMPL) && !defined(MOZ_MEMORY_IMPL)
#  define MOZ_MEMORY_IMPL
#endif
#if defined(MOZ_MEMORY_IMPL) && !defined(IMPL_MFBT)
#  ifdef MFBT_API 
#    error mozmemory_wrap.h has to be included before mozilla/Types.h when MOZ_MEMORY_IMPL is set and IMPL_MFBT is not.
#  endif
#  define IMPL_MFBT
#endif

#include "mozilla/Types.h"

#if !defined(MOZ_NATIVE_JEMALLOC)
#  ifdef MOZ_MEMORY_IMPL
#    if defined(MOZ_JEMALLOC_IMPL) && defined(MOZ_REPLACE_MALLOC)
#      define mozmem_malloc_impl(a)     je_ ## a
#      define mozmem_jemalloc_impl(a)   je_ ## a
#    else
#      define MOZ_JEMALLOC_API MFBT_API
#      if (defined(XP_WIN) || defined(XP_DARWIN))
#        if defined(MOZ_REPLACE_MALLOC)
#          define mozmem_malloc_impl(a)   a ## _impl
#        else
#          define mozmem_malloc_impl(a)   je_ ## a
#        endif
#      else
#        define MOZ_MEMORY_API MFBT_API
#        if defined(MOZ_WIDGET_ANDROID) || defined(MOZ_WIDGET_GONK)
#          define MOZ_WRAP_NEW_DELETE
#        endif
#      endif
#    endif
#    ifdef XP_WIN
#      define mozmem_dup_impl(a)      wrap_ ## a
#    endif
#  endif

#  if defined(MOZ_WIDGET_ANDROID)
#    ifndef mozmem_malloc_impl
#      define mozmem_malloc_impl(a)   __wrap_ ## a
#    endif
#    define mozmem_dup_impl(a)      __wrap_ ## a
#  endif



#  define je_(a) je_ ## a
#else 
#  define je_(a) a
#endif

#if !defined(MOZ_MEMORY_IMPL) || defined(MOZ_NATIVE_JEMALLOC)
#  define MOZ_MEMORY_API MFBT_API
#  define MOZ_JEMALLOC_API MFBT_API
#endif

#ifndef MOZ_MEMORY_API
#  define MOZ_MEMORY_API
#endif
#ifndef MOZ_JEMALLOC_API
#  define MOZ_JEMALLOC_API
#endif

#ifndef mozmem_malloc_impl
#  define mozmem_malloc_impl(a)   a
#endif
#ifndef mozmem_dup_impl
#  define mozmem_dup_impl(a)      a
#endif
#ifndef mozmem_jemalloc_impl
#  define mozmem_jemalloc_impl(a) a
#endif


#define malloc_impl              mozmem_malloc_impl(malloc)
#define posix_memalign_impl      mozmem_malloc_impl(posix_memalign)
#define aligned_alloc_impl       mozmem_malloc_impl(aligned_alloc)
#define calloc_impl              mozmem_malloc_impl(calloc)
#define realloc_impl             mozmem_malloc_impl(realloc)
#define free_impl                mozmem_malloc_impl(free)
#define memalign_impl            mozmem_malloc_impl(memalign)
#define valloc_impl              mozmem_malloc_impl(valloc)
#define malloc_usable_size_impl  mozmem_malloc_impl(malloc_usable_size)
#define malloc_good_size_impl    mozmem_malloc_impl(malloc_good_size)


#define strndup_impl   mozmem_dup_impl(strndup)
#define strdup_impl    mozmem_dup_impl(strdup)
#ifdef XP_WIN
#  define wcsdup_impl  mozmem_dup_impl(wcsdup)
#endif


#define jemalloc_stats_impl              mozmem_jemalloc_impl(jemalloc_stats)
#define jemalloc_purge_freed_pages_impl  mozmem_jemalloc_impl(jemalloc_purge_freed_pages)
#define jemalloc_free_dirty_pages_impl   mozmem_jemalloc_impl(jemalloc_free_dirty_pages)

#endif 

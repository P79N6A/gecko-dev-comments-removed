






























#ifndef _JEMALLOC_H_
#define _JEMALLOC_H_

#if defined(MOZ_MEMORY_DARWIN)
#include <malloc/malloc.h>
#endif
#include "jemalloc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(MOZ_MEMORY_LINUX)
__attribute__((weak))
#endif
void	jemalloc_stats(jemalloc_stats_t *stats);


#if !defined(MOZ_MEMORY_DARWIN)
#if defined(MOZ_MEMORY_LINUX)
__attribute__((weak))
#endif
#if defined(MOZ_JEMALLOC)
int je_nallocm(size_t *rsize, size_t size, int flags);
#else
size_t je_malloc_good_size(size_t size);
#endif
#endif

static inline size_t je_malloc_usable_size_in_advance(size_t size) {
#if defined(MOZ_MEMORY_DARWIN)
  return malloc_good_size(size);
#elif defined(MOZ_JEMALLOC)
  if (je_nallocm) {
    size_t ret;
    if (size == 0)
      size = 1;
    if (!je_nallocm(&ret, size, 0))
      return ret;
  }
  return size;
#else
  if (je_malloc_good_size)
    return je_malloc_good_size(size);
  else
    return size;
#endif
}


























#if defined(MOZ_MEMORY_LINUX) || defined(MOZ_JEMALLOC)
static inline void jemalloc_purge_freed_pages() { }
#else
void    jemalloc_purge_freed_pages();
#endif

#ifdef __cplusplus
} 
#endif

#endif 

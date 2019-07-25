






























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
size_t je_malloc_good_size(size_t size);
#endif

static inline size_t je_malloc_usable_size_in_advance(size_t size) {
#if defined(MOZ_MEMORY_DARWIN)
  return malloc_good_size(size);
#else
  if (je_malloc_good_size)
    return je_malloc_good_size(size);
  else
    return size;
#endif
}


























#if defined(MOZ_MEMORY_LINUX)
static inline void jemalloc_purge_freed_pages() { }
#else
void    jemalloc_purge_freed_pages();
#endif

#ifdef __cplusplus
} 
#endif

#endif 

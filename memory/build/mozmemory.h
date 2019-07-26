



#ifndef mozmemory_h
#define mozmemory_h










#ifndef MOZ_MEMORY
#  error Should not include mozmemory.h when MOZ_MEMORY is not set
#endif

#include "mozmemory_wrap.h"
#include "mozilla/Attributes.h"
#include "mozilla/Types.h"
#include "jemalloc_types.h"

MOZ_BEGIN_EXTERN_C





#ifdef XP_DARWIN
#  include <malloc/malloc.h>
#else
MOZ_MEMORY_API size_t malloc_good_size_impl(size_t size);





static inline size_t _malloc_good_size(size_t size) {
#  if defined(MOZ_GLUE_IN_PROGRAM) && !defined(IMPL_MFBT)
  if (!malloc_good_size)
    return size;
#  endif
  return malloc_good_size_impl(size);
}

#  define malloc_good_size _malloc_good_size
#endif

MOZ_JEMALLOC_API void jemalloc_stats(jemalloc_stats_t *stats);


























MOZ_JEMALLOC_API void jemalloc_purge_freed_pages();







MOZ_JEMALLOC_API void jemalloc_free_dirty_pages();

MOZ_END_EXTERN_C

#endif 

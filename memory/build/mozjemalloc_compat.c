



#ifndef MOZ_JEMALLOC3
#  error Should only compile this file when building with jemalloc 3
#endif

#define MOZ_JEMALLOC_IMPL

#include "mozmemory_wrap.h"
#include "jemalloc_types.h"
#include "mozilla/Types.h"

#if defined(MOZ_NATIVE_JEMALLOC)

MOZ_IMPORT_API int
je_(mallctl)(const char*, void*, size_t*, void*, size_t);
MOZ_IMPORT_API int
je_(mallctlnametomib)(const char *name, size_t *mibp, size_t *miblenp);
MOZ_IMPORT_API int
je_(mallctlbymib)(const size_t *mib, size_t miblen, void *oldp, size_t *oldlenp, void *newp, size_t newlen);
MOZ_IMPORT_API int
je_(nallocm)(size_t *rsize, size_t size, int flags);

#else
#  include "jemalloc/jemalloc.h"
#endif







#define	CTL_GET(n, v) do {						\
	size_t sz = sizeof(v);						\
	je_(mallctl)(n, &v, &sz, NULL, 0);				\
} while (0)

#define	CTL_I_GET(n, v, i) do {						\
	size_t mib[6];							\
	size_t miblen = sizeof(mib) / sizeof(mib[0]);			\
	size_t sz = sizeof(v);						\
	je_(mallctlnametomib)(n, mib, &miblen);			\
	mib[2] = i;							\
	je_(mallctlbymib)(mib, miblen, &v, &sz, NULL, 0);		\
} while (0)

MOZ_MEMORY_API size_t
malloc_good_size_impl(size_t size)
{
  size_t ret;
  


  if (size == 0)
    size = 1;
  if (!je_(nallocm)(&ret, size, 0))
    return ret;
  return size;
}

MOZ_JEMALLOC_API void
jemalloc_stats_impl(jemalloc_stats_t *stats)
{
  unsigned narenas;
  size_t active, allocated, mapped, page, pdirty;
  size_t lg_chunk;

  CTL_GET("arenas.narenas", narenas);
  CTL_GET("arenas.page", page);
  CTL_GET("stats.active", active);
  CTL_GET("stats.allocated", allocated);
  CTL_GET("stats.mapped", mapped);
  CTL_GET("opt.lg_chunk", lg_chunk);

  
  CTL_I_GET("stats.arenas.0.pdirty", pdirty, narenas);

  stats->chunksize = 1 << lg_chunk;
  stats->mapped = mapped;
  stats->allocated = allocated;
  stats->waste = active - allocated;
  stats->page_cache = pdirty * page;

  
  
  stats->bookkeeping = 0;
  stats->bin_unused = 0;
}

MOZ_JEMALLOC_API void
jemalloc_purge_freed_pages_impl()
{
}

MOZ_JEMALLOC_API void
jemalloc_free_dirty_pages_impl()
{
  je_(mallctl)("arenas.purge", NULL, 0, NULL, 0);
}

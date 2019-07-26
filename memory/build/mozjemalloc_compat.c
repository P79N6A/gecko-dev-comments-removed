



#include "mozilla/Types.h"
#include "jemalloc_types.h"

#if defined(MOZ_NATIVE_JEMALLOC)
#define wrap(a) a
#else
#define wrap(a) je_ ## a
#endif







#define	CTL_GET(n, v) do {						\
	size_t sz = sizeof(v);						\
	wrap(mallctl)(n, &v, &sz, NULL, 0);				\
} while (0)

#define	CTL_I_GET(n, v, i) do {						\
	size_t mib[6];							\
	size_t miblen = sizeof(mib) / sizeof(mib[0]);			\
	size_t sz = sizeof(v);						\
	wrap(mallctlnametomib)(n, mib, &miblen);			\
	mib[2] = i;							\
	wrap(mallctlbymib)(mib, miblen, &v, &sz, NULL, 0);		\
} while (0)

MOZ_IMPORT_API int
wrap(mallctl)(const char*, void*, size_t*, void*, size_t);
MOZ_IMPORT_API int
wrap(mallctlnametomib)(const char *name, size_t *mibp, size_t *miblenp);
MOZ_IMPORT_API int
wrap(mallctlbymib)(const size_t *mib, size_t miblen, void *oldp, size_t *oldlenp, void *newp, size_t newlen);

MOZ_EXPORT void
jemalloc_stats(jemalloc_stats_t *stats)
{
  unsigned narenas;
  size_t active, allocated, mapped, page, pdirty;

  CTL_GET("arenas.narenas", narenas);
  CTL_GET("arenas.page", page);
  CTL_GET("stats.active", active);
  CTL_GET("stats.allocated", allocated);
  CTL_GET("stats.mapped", mapped);

  
  CTL_I_GET("stats.arenas.0.pdirty", pdirty, narenas);

  stats->allocated = allocated;
  stats->mapped = mapped;
  stats->dirty = pdirty * page;
  stats->committed = active + stats->dirty;
}

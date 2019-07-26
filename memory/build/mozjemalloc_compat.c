



#include "mozilla/Types.h"
#include "jemalloc_types.h"

#if defined(MOZ_NATIVE_JEMALLOC)
#define wrap(a) a
#else
#define wrap(a) je_ ## a
#endif

extern MOZ_IMPORT_API(int)
wrap(mallctl)(const char*, void*, size_t*, void*, size_t);

MOZ_EXPORT_API(void)
jemalloc_stats(jemalloc_stats_t *stats)
{
  size_t size = sizeof(stats->mapped);
  wrap(mallctl)("stats.mapped", &stats->mapped, &size, NULL, 0);
  wrap(mallctl)("stats.allocated", &stats->allocated, &size, NULL, 0);
  stats->committed = -1;
  stats->dirty = -1;
}

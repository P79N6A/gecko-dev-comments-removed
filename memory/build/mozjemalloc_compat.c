



#include "mozilla/Types.h"
#include "jemalloc_types.h"

extern int je_mallctl(const char*, void*, size_t*, void*, size_t);

MOZ_EXPORT_API (void)
jemalloc_stats(jemalloc_stats_t *stats)
{
  size_t size = sizeof(stats->mapped);
  je_mallctl("stats.mapped", &stats->mapped, &size, NULL, 0);
  je_mallctl("stats.allocated", &stats->allocated, &size, NULL, 0);
  stats->committed = -1;
  stats->dirty = -1;
}

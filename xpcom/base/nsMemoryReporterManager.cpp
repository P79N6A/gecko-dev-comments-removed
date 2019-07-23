





































#include "nsCOMPtr.h"
#include "nsServiceManagerUtils.h"
#include "nsMemoryReporterManager.h"

#include "nsArrayEnumerator.h"











#if defined(MOZ_MEMORY) && defined(XP_WIN)
#define HAVE_JEMALLOC_STATS 1
#else
#undef HAVE_JEMALLOC_STATS
#endif

#if defined(HAVE_JEMALLOC_STATS)
#define HAVE_MALLOC_REPORTERS 1
#include "jemalloc.h"

PRInt64 getMallocMapped(void *) {
    jemalloc_stats_t stats;
    jemalloc_stats(&stats);
    return (PRInt64) stats.mapped;
}

PRInt64 getMallocAllocated(void *) {
    jemalloc_stats_t stats;
    jemalloc_stats(&stats);
    return (PRInt64) stats.allocated;
}

PRInt64 getMallocCommitted(void *) {
    jemalloc_stats_t stats;
    jemalloc_stats(&stats);
    return (PRInt64) stats.committed;
}

PRInt64 getMallocDirty(void *) {
    jemalloc_stats_t stats;
    jemalloc_stats(&stats);
    return (PRInt64) stats.dirty;
}

#elif defined(XP_MACOSX) && !defined(MOZ_MEMORY)
#define HAVE_MALLOC_REPORTERS 1
#include <malloc/malloc.h>

static PRInt64 getMallocAllocated(void *) {
    struct mstats stats = mstats();
    return (PRInt64) stats.bytes_used;
}

static PRInt64 getMallocMapped(void *) {
    struct mstats stats = mstats();
    return (PRInt64) stats.bytes_total;
}

static PRInt64 getMallocDefaultCommitted(void *) {
    malloc_statistics_t stats;
    malloc_zone_statistics(malloc_default_zone(), &stats);
    return stats.size_in_use;
}

static PRInt64 getMallocDefaultAllocated(void *) {
    malloc_statistics_t stats;
    malloc_zone_statistics(malloc_default_zone(), &stats);
    return stats.size_allocated;
}

#endif


#ifdef HAVE_MALLOC_REPORTERS
NS_MEMORY_REPORTER_IMPLEMENT(MallocAllocated,
                             "malloc/allocated",
                             "Malloc bytes allocated (in use by application)",
                             getMallocAllocated,
                             NULL)

NS_MEMORY_REPORTER_IMPLEMENT(MallocMapped,
                             "malloc/mapped",
                             "Malloc bytes mapped (not necessarily committed)",
                             getMallocMapped,
                             NULL)

#if defined(HAVE_JEMALLOC_STATS)
NS_MEMORY_REPORTER_IMPLEMENT(MallocCommitted,
                             "malloc/committed",
                             "Malloc bytes committed (readable/writable)",
                             getMallocCommitted,
                             NULL)

NS_MEMORY_REPORTER_IMPLEMENT(MallocDirty,
                             "malloc/dirty",
                             "Malloc bytes dirty (committed unused pages)",
                             getMallocDirty,
                             NULL)
#elif defined(XP_MACOSX) && !defined(MOZ_MEMORY)
NS_MEMORY_REPORTER_IMPLEMENT(MallocDefaultCommitted,
                             "malloc/zone0/committed",
                             "Malloc bytes committed (r/w) in default zone",
                             getMallocDefaultCommitted,
                             NULL)

NS_MEMORY_REPORTER_IMPLEMENT(MallocDefaultAllocated,
                             "malloc/zone0/allocated",
                             "Malloc bytes allocated (in use) in default zone",
                             getMallocDefaultAllocated,
                             NULL)
#endif

#endif





NS_IMPL_ISUPPORTS1(nsMemoryReporterManager, nsIMemoryReporterManager)

NS_IMETHODIMP
nsMemoryReporterManager::Init()
{
    nsCOMPtr<nsIMemoryReporter> mr;

    


#ifdef HAVE_MALLOC_REPORTERS
    mr = new NS_MEMORY_REPORTER_NAME(MallocAllocated);
    RegisterReporter(mr);

    mr = new NS_MEMORY_REPORTER_NAME(MallocMapped);
    RegisterReporter(mr);

#if defined(HAVE_JEMALLOC_STATS)
    mr = new NS_MEMORY_REPORTER_NAME(MallocCommitted);
    RegisterReporter(mr);

    mr = new NS_MEMORY_REPORTER_NAME(MallocDirty);
    RegisterReporter(mr);
#elif defined(XP_MACOSX) && !defined(MOZ_MEMORY)
    mr = new NS_MEMORY_REPORTER_NAME(MallocDefaultCommitted);
    RegisterReporter(mr);

    mr = new NS_MEMORY_REPORTER_NAME(MallocDefaultAllocated);
    RegisterReporter(mr);
#endif
#endif

    return NS_OK;
}

NS_IMETHODIMP
nsMemoryReporterManager::EnumerateReporters(nsISimpleEnumerator **result)
{
    return NS_NewArrayEnumerator(result, mReporters);
}

NS_IMETHODIMP
nsMemoryReporterManager::RegisterReporter(nsIMemoryReporter *reporter)
{
    if (mReporters.IndexOf(reporter) != -1)
        return NS_ERROR_FAILURE;

    mReporters.AppendObject(reporter);
    return NS_OK;
}

NS_IMETHODIMP
nsMemoryReporterManager::UnregisterReporter(nsIMemoryReporter *reporter)
{
    if (!mReporters.RemoveObject(reporter))
        return NS_ERROR_FAILURE;

    return NS_OK;
}

NS_COM nsresult
NS_RegisterMemoryReporter (nsIMemoryReporter *reporter)
{
    nsCOMPtr<nsIMemoryReporterManager> mgr = do_GetService("@mozilla.org/memory-reporter-manager;1");
    if (mgr == nsnull)
        return NS_ERROR_FAILURE;
    return mgr->RegisterReporter(reporter);
}

NS_COM nsresult
NS_UnregisterMemoryReporter (nsIMemoryReporter *reporter)
{
    nsCOMPtr<nsIMemoryReporterManager> mgr = do_GetService("@mozilla.org/memory-reporter-manager;1");
    if (mgr == nsnull)
        return NS_ERROR_FAILURE;
    return mgr->UnregisterReporter(reporter);
}


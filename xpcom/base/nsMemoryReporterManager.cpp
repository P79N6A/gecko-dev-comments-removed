





































#include "nsCOMPtr.h"
#include "nsServiceManagerUtils.h"
#include "nsMemoryReporterManager.h"
#include "nsArrayEnumerator.h"







#if defined(MOZ_MEMORY)
#  if defined(XP_WIN) || defined(SOLARIS) || defined(ANDROID)
#    define HAVE_JEMALLOC_STATS 1
#    include "jemalloc.h"
#  elif defined(XP_LINUX)
#    define HAVE_JEMALLOC_STATS 1
#    include "jemalloc_types.h"




extern "C" {
extern void jemalloc_stats(jemalloc_stats_t* stats)
  NS_VISIBILITY_DEFAULT __attribute__((weak));
}
#  endif  
#endif  

#if HAVE_JEMALLOC_STATS
#  define HAVE_MALLOC_REPORTERS 1

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

#ifdef XP_WIN
#include <windows.h>
#include <psapi.h>

static PRInt64 GetWin32PrivateBytes(void *) {
#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_LONGHORN
  PROCESS_MEMORY_COUNTERS_EX pmcex;
  pmcex.cb = sizeof(PROCESS_MEMORY_COUNTERS_EX);

  if (!GetProcessMemoryInfo(GetCurrentProcess(),
                            (PPROCESS_MEMORY_COUNTERS) &pmcex,
                            sizeof(PROCESS_MEMORY_COUNTERS_EX)))
    return 0;

  return pmcex.PrivateUsage;
#else
  return 0;
#endif
}

static PRInt64 GetWin32WorkingSetSize(void *) {
  PROCESS_MEMORY_COUNTERS pmc;
  pmc.cb = sizeof(PROCESS_MEMORY_COUNTERS);

  if (!GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
      return 0;

  return pmc.WorkingSetSize;
}

NS_MEMORY_REPORTER_IMPLEMENT(Win32WorkingSetSize,
                             "win32/workingset",
                             "Win32 working set size",
                             GetWin32WorkingSetSize,
                             nsnull);

NS_MEMORY_REPORTER_IMPLEMENT(Win32PrivateBytes,
                             "win32/privatebytes",
                             "Win32 private bytes (cannot be shared with other processes).  (Available only on Windows XP SP2 or later.)",
                             GetWin32PrivateBytes,
                             nsnull);
#endif





NS_IMPL_THREADSAFE_ISUPPORTS1(nsMemoryReporterManager, nsIMemoryReporterManager)

NS_IMETHODIMP
nsMemoryReporterManager::Init()
{
#if HAVE_JEMALLOC_STATS && defined(XP_LINUX)
    if (!jemalloc_stats)
        return NS_ERROR_FAILURE;
#endif
    


#define REGISTER(_x)  RegisterReporter(new NS_MEMORY_REPORTER_NAME(_x))

    


#ifdef HAVE_MALLOC_REPORTERS
    REGISTER(MallocAllocated);
    REGISTER(MallocMapped);

#if defined(HAVE_JEMALLOC_STATS)
    REGISTER(MallocCommitted);
    REGISTER(MallocDirty);
#elif defined(XP_MACOSX) && !defined(MOZ_MEMORY)
    REGISTER(MallocDefaultCommitted);
    REGISTER(MallocDefaultAllocated);
#endif
#endif

#ifdef XP_WIN
#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_LONGHORN
    REGISTER(Win32PrivateBytes);
#endif
    REGISTER(Win32WorkingSetSize);
#endif

    return NS_OK;
}

nsMemoryReporterManager::nsMemoryReporterManager()
  : mMutex("nsMemoryReporterManager::mMutex")
{
}

nsMemoryReporterManager::~nsMemoryReporterManager()
{
}

NS_IMETHODIMP
nsMemoryReporterManager::EnumerateReporters(nsISimpleEnumerator **result)
{
    nsresult rv;
    mozilla::MutexAutoLock autoLock(mMutex); 
    rv = NS_NewArrayEnumerator(result, mReporters);
    return rv;
}

NS_IMETHODIMP
nsMemoryReporterManager::RegisterReporter(nsIMemoryReporter *reporter)
{
    mozilla::MutexAutoLock autoLock(mMutex); 
    if (mReporters.IndexOf(reporter) != -1)
        return NS_ERROR_FAILURE;

    mReporters.AppendObject(reporter);
    return NS_OK;
}

NS_IMETHODIMP
nsMemoryReporterManager::UnregisterReporter(nsIMemoryReporter *reporter)
{
    mozilla::MutexAutoLock autoLock(mMutex); 
    if (!mReporters.RemoveObject(reporter))
        return NS_ERROR_FAILURE;

    return NS_OK;
}

NS_IMPL_ISUPPORTS1(nsMemoryReporter, nsIMemoryReporter)

nsMemoryReporter::nsMemoryReporter(nsCString& prefix,
                                   nsCString& path,
                                   nsCString& desc,
                                   PRInt64 memoryUsed)
: mDesc(desc)
, mMemoryUsed(memoryUsed) 
{
  if (!prefix.IsEmpty()) {
    mPath.Append(prefix);
    mPath.Append(NS_LITERAL_CSTRING(" - "));
  }
  mPath.Append(path);
}

nsMemoryReporter::~nsMemoryReporter()
{
}

NS_IMETHODIMP nsMemoryReporter::GetPath(char **aPath)
{
  *aPath = strdup(mPath.get());
  return NS_OK;
}

NS_IMETHODIMP nsMemoryReporter::GetDescription(char **aDescription)
{
  *aDescription = strdup(mDesc.get());
  return NS_OK;
}

NS_IMETHODIMP nsMemoryReporter::GetMemoryUsed(PRInt64 *aMemoryUsed)
{
  *aMemoryUsed = mMemoryUsed;
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








































#include "nsCOMPtr.h"
#include "nsServiceManagerUtils.h"
#include "nsMemoryReporterManager.h"
#include "nsArrayEnumerator.h"

#if defined(XP_LINUX)

#include <unistd.h>
static PRInt64 GetProcSelfStatmField(int n)
{
    
    
    static const int MAX_FIELD = 2;
    size_t fields[MAX_FIELD];
    NS_ASSERTION(n < MAX_FIELD, "bad field number");
    FILE *f = fopen("/proc/self/statm", "r");
    if (f) {
        int nread = fscanf(f, "%lu %lu", &fields[0], &fields[1]);
        fclose(f);
        return (PRInt64) ((nread == MAX_FIELD) ? fields[n]*getpagesize() : -1);
    }
    return (PRInt64) -1;
}

static PRInt64 GetMapped(void *)
{
    return GetProcSelfStatmField(0);
}

static PRInt64 GetResident(void *)
{
    return GetProcSelfStatmField(1);
}

#elif defined(XP_MACOSX)

#include <mach/mach_init.h>
#include <mach/task.h>

static bool GetTaskBasicInfo(struct task_basic_info *ti)
{
    mach_msg_type_number_t count = TASK_BASIC_INFO_COUNT;
    kern_return_t kr = task_info(mach_task_self(), TASK_BASIC_INFO,
                                 (task_info_t)ti, &count);
    return kr == KERN_SUCCESS;
}





static PRInt64 GetMapped(void *)
{
    task_basic_info ti;
    return (PRInt64) (GetTaskBasicInfo(&ti) ? ti.virtual_size : -1);
}

static PRInt64 GetResident(void *)
{
    task_basic_info ti;
    return (PRInt64) (GetTaskBasicInfo(&ti) ? ti.resident_size : -1);
}

#elif defined(XP_WIN)

#include <windows.h>
#include <psapi.h>

static PRInt64 GetMapped(void *)
{
#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_LONGHORN
  PROCESS_MEMORY_COUNTERS_EX pmcex;
  pmcex.cb = sizeof(PROCESS_MEMORY_COUNTERS_EX);

  if (!GetProcessMemoryInfo(GetCurrentProcess(),
                            (PPROCESS_MEMORY_COUNTERS) &pmcex, sizeof(pmcex)))
      return (PRInt64) -1;

  return pmcex.PrivateUsage;
#else
  return (PRInt64) -1;
#endif
}

static PRInt64 GetResident(void *)
{
  PROCESS_MEMORY_COUNTERS pmc;
  pmc.cb = sizeof(PROCESS_MEMORY_COUNTERS);

  if (!GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
      return (PRInt64) -1;

  return pmc.WorkingSetSize;
}

#else

static PRInt64 GetMapped(void *)
{
    return (PRInt64) -1;
}

static PRInt64 GetResident(void *)
{
    return (PRInt64) -1;
}

#endif



NS_MEMORY_REPORTER_IMPLEMENT(Mapped,
    "mapped",
    "Memory mapped by the process, including the heap, code and data segments, "
    "thread stacks, and memory explicitly mapped by the process via "
    "mmap, VirtualAlloc and similar operations. "
    "Note that 'resident' is a better measure of memory resources used by the "
    "process. "
    "On Windows (XP SP2 or later only) this is the private usage and does not "
    "include memory shared with other processes. "
    "On Mac and Linux this is the vsize figure as reported by 'top' or 'ps' "
    "and includes memory shared with other processes;  on Mac the amount of "
    "shared memory can be very high and so this figure is of limited use.",
    GetMapped,
    NULL)

NS_MEMORY_REPORTER_IMPLEMENT(Resident,
    "resident",
    "Memory mapped by the process that is present in physical memory, "
    "also known as the resident set size (RSS).  This is the best single "
    "figure to use when considering the memory resources used by the process, "
    "but it depends both on other processes being run and details of the OS "
    "kernel and so is best used for comparing the memory usage of a single "
    "process at different points in time.",
    GetResident,
    NULL)







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

static PRInt64 GetMappedHeapUsed(void *)
{
    jemalloc_stats_t stats;
    jemalloc_stats(&stats);
    return (PRInt64) stats.allocated;
}

static PRInt64 GetMappedHeapUnused(void *)
{
    jemalloc_stats_t stats;
    jemalloc_stats(&stats);
    return (PRInt64) (stats.mapped - stats.allocated);
}

static PRInt64 GetHeapCommitted(void *)
{
    jemalloc_stats_t stats;
    jemalloc_stats(&stats);
    return (PRInt64) stats.committed;
}

static PRInt64 GetHeapDirty(void *)
{
    jemalloc_stats_t stats;
    jemalloc_stats(&stats);
    return (PRInt64) stats.dirty;
}

NS_MEMORY_REPORTER_IMPLEMENT(HeapCommitted,
                             "heap-committed",
                             "Memory mapped by the heap allocator that is "
                             "committed, i.e. in physical memory or paged to "
                             "disk.",
                             GetHeapCommitted,
                             NULL)

NS_MEMORY_REPORTER_IMPLEMENT(HeapDirty,
                             "heap-dirty",
                             "Memory mapped by the heap allocator that is "
                             "committed but unused.",
                             GetHeapDirty,
                             NULL)

#elif defined(XP_MACOSX) && !defined(MOZ_MEMORY)
#include <malloc/malloc.h>

static PRInt64 GetMappedHeapUsed(void *)
{
    struct mstats stats = mstats();
    return (PRInt64) stats.bytes_used;
}

static PRInt64 GetMappedHeapUnused(void *)
{
    struct mstats stats = mstats();
    return (PRInt64) (stats.bytes_total - stats.bytes_used);
}

static PRInt64 GetHeapZone0Committed(void *)
{
    malloc_statistics_t stats;
    malloc_zone_statistics(malloc_default_zone(), &stats);
    return stats.size_in_use;
}

static PRInt64 GetHeapZone0Used(void *)
{
    malloc_statistics_t stats;
    malloc_zone_statistics(malloc_default_zone(), &stats);
    return stats.size_allocated;
}

NS_MEMORY_REPORTER_IMPLEMENT(HeapZone0Committed,
                             "heap-zone0-committed",
                             "Memory mapped by the heap allocator that is "
                             "committed in the default zone.",
                             GetHeapZone0Committed,
                             NULL)

NS_MEMORY_REPORTER_IMPLEMENT(HeapZone0Used,
                             "heap-zone0-used",
                             "Memory mapped by the heap allocator in the "
                             "default zone that is available for use by the "
                             "application.",
                             GetHeapZone0Used,
                             NULL)
#else

static PRInt64 GetMappedHeapUsed(void *)
{
    return (PRInt64) -1;
}

static PRInt64 GetMappedHeapUnused(void *)
{
    return (PRInt64) -1;
}

#endif



NS_MEMORY_REPORTER_IMPLEMENT(MappedHeapUsed,
    "mapped/heap/used",
    "Memory mapped by the heap allocator that is available for use by the "
    "application.  This may exceed the amount of memory requested by the "
    "application due to the allocator rounding up request sizes.  "
    "(The exact amount requested is not measured.) "
    "This is usually the best figure for developers to focus on when trying "
    "to reduce memory consumption.",
    GetMappedHeapUsed,
    NULL)

NS_MEMORY_REPORTER_IMPLEMENT(MappedHeapUnused,
    "mapped/heap/unused",
    "Memory mapped by the heap allocator and not available for use by the "
    "application.  This can grow large if the heap allocator is holding onto "
    "memory that the application has freed.",
    GetMappedHeapUnused,
    NULL)





NS_IMPL_THREADSAFE_ISUPPORTS1(nsMemoryReporterManager, nsIMemoryReporterManager)

NS_IMETHODIMP
nsMemoryReporterManager::Init()
{
#if HAVE_JEMALLOC_STATS && defined(XP_LINUX)
    if (!jemalloc_stats)
        return NS_ERROR_FAILURE;
#endif

#define REGISTER(_x)  RegisterReporter(new NS_MEMORY_REPORTER_NAME(_x))

    REGISTER(Mapped);
    REGISTER(MappedHeapUsed);
    REGISTER(MappedHeapUnused);
    REGISTER(Resident);

#if defined(HAVE_JEMALLOC_STATS)
    REGISTER(HeapCommitted);
    REGISTER(HeapDirty);
#elif defined(XP_MACOSX) && !defined(MOZ_MEMORY)
    REGISTER(HeapZone0Committed);
    REGISTER(HeapZone0Used);
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
      mPath.Append(NS_LITERAL_CSTRING(":"));
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


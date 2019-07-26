





#include "nsAtomTable.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "nsDirectoryServiceUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsMemoryReporterManager.h"
#include "nsArrayEnumerator.h"
#include "nsISimpleEnumerator.h"
#include "nsIFile.h"
#include "nsIFileStreams.h"
#include "nsPrintfCString.h"
#include "nsThreadUtils.h"
#include "nsIObserverService.h"
#include "nsThread.h"
#include "nsMemoryInfoDumper.h"
#include "mozilla/Telemetry.h"
#include "mozilla/Attributes.h"
#include "mozilla/Services.h"

#ifndef XP_WIN
#include <unistd.h>
#endif

using namespace mozilla;

#if defined(MOZ_MEMORY)
#  define HAVE_JEMALLOC_STATS 1
#  include "mozmemory.h"
#endif  

#if defined(XP_LINUX)

#include <unistd.h>
static nsresult GetProcSelfStatmField(int aField, int64_t* aN)
{
    
    
    static const int MAX_FIELD = 2;
    size_t fields[MAX_FIELD];
    MOZ_ASSERT(aField < MAX_FIELD, "bad field number");
    FILE* f = fopen("/proc/self/statm", "r");
    if (f) {
        int nread = fscanf(f, "%zu %zu", &fields[0], &fields[1]);
        fclose(f);
        if (nread == MAX_FIELD) {
            *aN = fields[aField] * getpagesize();
            return NS_OK;
        }
    }
    return NS_ERROR_FAILURE;
}

#define HAVE_VSIZE_AND_RESIDENT_REPORTERS 1
static nsresult GetVsize(int64_t* aN)
{
    return GetProcSelfStatmField(0, aN);
}

static nsresult GetResident(int64_t* aN)
{
    return GetProcSelfStatmField(1, aN);
}

static nsresult GetResidentFast(int64_t* aN)
{
    return GetResident(aN);
}

#elif defined(__DragonFly__) || defined(__FreeBSD__) \
    || defined(__NetBSD__) || defined(__OpenBSD__)

#include <sys/param.h>
#include <sys/sysctl.h>
#if defined(__DragonFly__) || defined(__FreeBSD__)
#include <sys/user.h>
#endif

#include <unistd.h>

#if defined(__NetBSD__)
#undef KERN_PROC
#define KERN_PROC KERN_PROC2
#define KINFO_PROC struct kinfo_proc2
#else
#define KINFO_PROC struct kinfo_proc
#endif

#if defined(__DragonFly__)
#define KP_SIZE(kp) (kp.kp_vm_map_size)
#define KP_RSS(kp) (kp.kp_vm_rssize * getpagesize())
#elif defined(__FreeBSD__)
#define KP_SIZE(kp) (kp.ki_size)
#define KP_RSS(kp) (kp.ki_rssize * getpagesize())
#elif defined(__NetBSD__)
#define KP_SIZE(kp) (kp.p_vm_msize * getpagesize())
#define KP_RSS(kp) (kp.p_vm_rssize * getpagesize())
#elif defined(__OpenBSD__)
#define KP_SIZE(kp) ((kp.p_vm_dsize + kp.p_vm_ssize                     \
                      + kp.p_vm_tsize) * getpagesize())
#define KP_RSS(kp) (kp.p_vm_rssize * getpagesize())
#endif

static nsresult GetKinfoProcSelf(KINFO_PROC* aProc)
{
    int mib[] = {
        CTL_KERN,
        KERN_PROC,
        KERN_PROC_PID,
        getpid(),
#if defined(__NetBSD__) || defined(__OpenBSD__)
        sizeof(KINFO_PROC),
        1,
#endif
    };
    u_int miblen = sizeof(mib) / sizeof(mib[0]);
    size_t size = sizeof(KINFO_PROC);
    if (sysctl(mib, miblen, aProc, &size, NULL, 0))
        return NS_ERROR_FAILURE;

    return NS_OK;
}

#define HAVE_VSIZE_AND_RESIDENT_REPORTERS 1
static nsresult GetVsize(int64_t* aN)
{
    KINFO_PROC proc;
    nsresult rv = GetKinfoProcSelf(&proc);
    if (NS_SUCCEEDED(rv))
        *aN = KP_SIZE(proc);

    return rv;
}

static nsresult GetResident(int64_t* aN)
{
    KINFO_PROC proc;
    nsresult rv = GetKinfoProcSelf(&proc);
    if (NS_SUCCEEDED(rv))
        *aN = KP_RSS(proc);

    return rv;
}

static nsresult GetResidentFast(int64_t* aN)
{
    return GetResident(aN);
}

#elif defined(SOLARIS)

#include <procfs.h>
#include <fcntl.h>
#include <unistd.h>

static void XMappingIter(int64_t& vsize, int64_t& resident)
{
    vsize = -1;
    resident = -1;
    int mapfd = open("/proc/self/xmap", O_RDONLY);
    struct stat st;
    prxmap_t* prmapp = NULL;
    if (mapfd >= 0) {
        if (!fstat(mapfd, &st)) {
            int nmap = st.st_size / sizeof(prxmap_t);
            while (1) {
                
                
                
                nmap *= 2;
                prmapp = (prxmap_t*)malloc((nmap + 1) * sizeof(prxmap_t));
                if (!prmapp) {
                    
                    break;
                }
                int n = pread(mapfd, prmapp, (nmap + 1) * sizeof(prxmap_t), 0);
                if (n < 0) {
                    break;
                }
                if (nmap >= n / sizeof (prxmap_t)) {
                    vsize = 0;
                    resident = 0;
                    for (int i = 0; i < n / sizeof (prxmap_t); i++) {
                        vsize += prmapp[i].pr_size;
                        resident += prmapp[i].pr_rss * prmapp[i].pr_pagesize;
                    }
                    break;
                }
                free(prmapp);
            }
            free(prmapp);
        }
        close(mapfd);
    }
}

#define HAVE_VSIZE_AND_RESIDENT_REPORTERS 1
static nsresult GetVsize(int64_t* aN)
{
    int64_t vsize, resident;
    XMappingIter(vsize, resident);
    if (vsize == -1) {
        return NS_ERROR_FAILURE;
    }
    *aN = vsize;
    return NS_OK;
}

static nsresult GetResident(int64_t* aN)
{
    int64_t vsize, resident;
    XMappingIter(vsize, resident);
    if (resident == -1) {
        return NS_ERROR_FAILURE;
    }
    *aN = resident;
    return NS_OK;
}

static nsresult GetResidentFast(int64_t* aN)
{
    return GetResident(aN);
}

#elif defined(XP_MACOSX)

#include <mach/mach_init.h>
#include <mach/task.h>

static bool GetTaskBasicInfo(struct task_basic_info* aTi)
{
    mach_msg_type_number_t count = TASK_BASIC_INFO_COUNT;
    kern_return_t kr = task_info(mach_task_self(), TASK_BASIC_INFO,
                                 (task_info_t)aTi, &count);
    return kr == KERN_SUCCESS;
}




#define HAVE_VSIZE_AND_RESIDENT_REPORTERS 1
static nsresult GetVsize(int64_t* aN)
{
    task_basic_info ti;
    if (!GetTaskBasicInfo(&ti))
        return NS_ERROR_FAILURE;

    *aN = ti.virtual_size;
    return NS_OK;
}








static nsresult GetResident(int64_t* aN, bool aDoPurge)
{
#ifdef HAVE_JEMALLOC_STATS
    if (aDoPurge) {
      Telemetry::AutoTimer<Telemetry::MEMORY_FREE_PURGED_PAGES_MS> timer;
      jemalloc_purge_freed_pages();
    }
#endif

    task_basic_info ti;
    if (!GetTaskBasicInfo(&ti))
        return NS_ERROR_FAILURE;

    *aN = ti.resident_size;
    return NS_OK;
}

static nsresult GetResidentFast(int64_t* aN)
{
    return GetResident(aN,  false);
}

static nsresult GetResident(int64_t* aN)
{
    return GetResident(aN,  true);
}

#elif defined(XP_WIN)

#include <windows.h>
#include <psapi.h>

#define HAVE_VSIZE_AND_RESIDENT_REPORTERS 1
static nsresult GetVsize(int64_t* aN)
{
    MEMORYSTATUSEX s;
    s.dwLength = sizeof(s);

    if (!GlobalMemoryStatusEx(&s)) {
        return NS_ERROR_FAILURE;
    }

    *aN = s.ullTotalVirtual - s.ullAvailVirtual;
    return NS_OK;
}

static nsresult GetResident(int64_t* aN)
{
    PROCESS_MEMORY_COUNTERS pmc;
    pmc.cb = sizeof(PROCESS_MEMORY_COUNTERS);

    if (!GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return NS_ERROR_FAILURE;
    }

    *aN = pmc.WorkingSetSize;
    return NS_OK;
}

static nsresult GetResidentFast(int64_t* aN)
{
    return GetResident(aN);
}

#define HAVE_PRIVATE_REPORTER
class PrivateReporter MOZ_FINAL : public MemoryReporterBase
{
public:
    PrivateReporter()
      : MemoryReporterBase("private", KIND_OTHER, UNITS_BYTES,
"Memory that cannot be shared with other processes, including memory that is "
"committed and marked MEM_PRIVATE, data that is not mapped, and executable "
"pages that have been written to.")
    {}

    NS_IMETHOD GetAmount(int64_t* aAmount)
    {
        PROCESS_MEMORY_COUNTERS_EX pmcex;
        pmcex.cb = sizeof(PROCESS_MEMORY_COUNTERS_EX);

        if (!GetProcessMemoryInfo(
                GetCurrentProcess(),
                (PPROCESS_MEMORY_COUNTERS) &pmcex, sizeof(pmcex))) {
            return NS_ERROR_FAILURE;
        }

        *aAmount = pmcex.PrivateUsage;
        return NS_OK;
    }
};

#endif  

#ifdef HAVE_VSIZE_AND_RESIDENT_REPORTERS
class VsizeReporter MOZ_FINAL : public MemoryReporterBase
{
public:
    VsizeReporter()
      : MemoryReporterBase("vsize", KIND_OTHER, UNITS_BYTES,
"Memory mapped by the process, including code and data segments, the heap, "
"thread stacks, memory explicitly mapped by the process via mmap and similar "
"operations, and memory shared with other processes. This is the vsize figure "
"as reported by 'top' and 'ps'.  This figure is of limited use on Mac, where "
"processes share huge amounts of memory with one another.  But even on other "
"operating systems, 'resident' is a much better measure of the memory "
"resources used by the process.")
    {}

    NS_IMETHOD GetAmount(int64_t* aAmount) { return GetVsize(aAmount); }
};

class ResidentReporter MOZ_FINAL : public MemoryReporterBase
{
public:
    ResidentReporter()
      : MemoryReporterBase("resident", KIND_OTHER, UNITS_BYTES,
"Memory mapped by the process that is present in physical memory, also known "
"as the resident set size (RSS).  This is the best single figure to use when "
"considering the memory resources used by the process, but it depends both on "
"other processes being run and details of the OS kernel and so is best used "
"for comparing the memory usage of a single process at different points in "
"time.")
    {}

    NS_IMETHOD GetAmount(int64_t* aAmount) { return GetResident(aAmount); }
};

class ResidentFastReporter MOZ_FINAL : public MemoryReporterBase
{
public:
    ResidentFastReporter()
      : MemoryReporterBase("resident-fast", KIND_OTHER, UNITS_BYTES,
"This is the same measurement as 'resident', but it tries to be as fast as "
"possible at the expense of accuracy.  On most platforms this is identical to "
"the 'resident' measurement, but on Mac it may over-count.  You should use "
"'resident-fast' where you care about latency of collection (e.g. in "
"telemetry).  Otherwise you should use 'resident'.")
    {}

    NS_IMETHOD GetAmount(int64_t* aAmount) { return GetResidentFast(aAmount); }
};
#endif  

#ifdef XP_UNIX

#include <sys/time.h>
#include <sys/resource.h>

#define HAVE_PAGE_FAULT_REPORTERS 1

class PageFaultsSoftReporter MOZ_FINAL : public MemoryReporterBase
{
public:
    PageFaultsSoftReporter()
      : MemoryReporterBase("page-faults-soft", KIND_OTHER,
                           UNITS_COUNT_CUMULATIVE,
"The number of soft page faults (also known as 'minor page faults') that "
"have occurred since the process started.  A soft page fault occurs when the "
"process tries to access a page which is present in physical memory but is "
"not mapped into the process's address space.  For instance, a process might "
"observe soft page faults when it loads a shared library which is already "
"present in physical memory. A process may experience many thousands of soft "
"page faults even when the machine has plenty of available physical memory, "
"and because the OS services a soft page fault without accessing the disk, "
"they impact performance much less than hard page faults.")
    {}

    NS_IMETHOD GetAmount(int64_t* aAmount)
    {
        struct rusage usage;
        int err = getrusage(RUSAGE_SELF, &usage);
        if (err != 0) {
            return NS_ERROR_FAILURE;
        }
        *aAmount = usage.ru_minflt;
        return NS_OK;
    }
};

class PageFaultsHardReporter MOZ_FINAL : public MemoryReporterBase
{
public:
    PageFaultsHardReporter()
      : MemoryReporterBase("page-faults-hard", KIND_OTHER,
                           UNITS_COUNT_CUMULATIVE,
"The number of hard page faults (also known as 'major page faults') that have "
"occurred since the process started.  A hard page fault occurs when a process "
"tries to access a page which is not present in physical memory. The "
"operating system must access the disk in order to fulfill a hard page fault. "
"When memory is plentiful, you should see very few hard page faults. But if "
"the process tries to use more memory than your machine has available, you "
"may see many thousands of hard page faults. Because accessing the disk is up "
"to a million times slower than accessing RAM, the program may run very "
"slowly when it is experiencing more than 100 or so hard page faults a second.")
    {}

    NS_IMETHOD GetAmount(int64_t* aAmount)
    {
        struct rusage usage;
        int err = getrusage(RUSAGE_SELF, &usage);
        if (err != 0) {
            return NS_ERROR_FAILURE;
        }
        *aAmount = usage.ru_majflt;
        return NS_OK;
    }
};
#endif  







#ifdef HAVE_JEMALLOC_STATS

class HeapAllocatedReporter MOZ_FINAL : public MemoryReporterBase
{
public:
    HeapAllocatedReporter()
      : MemoryReporterBase("heap-allocated", KIND_OTHER, UNITS_BYTES,
"Memory mapped by the heap allocator that is currently allocated to the "
"application.  This may exceed the amount of memory requested by the "
"application because the allocator regularly rounds up request sizes. (The "
"exact amount requested is not recorded.)")
    {}
private:
    int64_t Amount()
    {
        jemalloc_stats_t stats;
        jemalloc_stats(&stats);
        return (int64_t) stats.allocated;
    }
};

class HeapOverheadWasteReporter MOZ_FINAL : public MemoryReporterBase
{
public:
    
    
    
    HeapOverheadWasteReporter()
      : MemoryReporterBase("explicit/heap-overhead/waste",
                           KIND_NONHEAP, UNITS_BYTES,
"Committed bytes which do not correspond to an active allocation and which the "
"allocator is not intentionally keeping alive (i.e., not 'heap-bookkeeping' or "
"'heap-page-cache').  Although the allocator will waste some space under any "
"circumstances, a large value here may indicate that the heap is highly "
"fragmented, or that allocator is performing poorly for some other reason.")
    {}
private:
    int64_t Amount()
    {
        jemalloc_stats_t stats;
        jemalloc_stats(&stats);
        return stats.waste;
    }
};

class HeapOverheadBookkeepingReporter MOZ_FINAL : public MemoryReporterBase
{
public:
    HeapOverheadBookkeepingReporter()
      : MemoryReporterBase("explicit/heap-overhead/bookkeeping",
                           KIND_NONHEAP, UNITS_BYTES,
"Committed bytes which the heap allocator uses for internal data structures.")
    {}
private:
    int64_t Amount()
    {
        jemalloc_stats_t stats;
        jemalloc_stats(&stats);
        return stats.bookkeeping;
    }
};

class HeapOverheadPageCacheReporter MOZ_FINAL : public MemoryReporterBase
{
public:
    HeapOverheadPageCacheReporter()
      : MemoryReporterBase("explicit/heap-overhead/page-cache",
                           KIND_NONHEAP, UNITS_BYTES,
"Memory which the allocator could return to the operating system, but hasn't. "
"The allocator keeps this memory around as an optimization, so it doesn't "
"have to ask the OS the next time it needs to fulfill a request. This value "
"is typically not larger than a few megabytes.")
    {}
private:
    int64_t Amount()
    {
        jemalloc_stats_t stats;
        jemalloc_stats(&stats);
        return (int64_t) stats.page_cache;
    }
};

class HeapCommittedReporter MOZ_FINAL : public MemoryReporterBase
{
public:
    HeapCommittedReporter()
      : MemoryReporterBase("heap-committed", KIND_OTHER, UNITS_BYTES,
"Memory mapped by the heap allocator that is committed, i.e. in physical "
"memory or paged to disk.  This value corresponds to 'heap-allocated' + "
"'heap-waste' + 'heap-bookkeeping' + 'heap-page-cache', but because "
"these values are read at different times, the result probably won't match "
"exactly.")
    {}
private:
    int64_t Amount()
    {
        jemalloc_stats_t stats;
        jemalloc_stats(&stats);
        return (int64_t) (stats.allocated + stats.waste +
                          stats.bookkeeping + stats.page_cache);
    }
};

class HeapOverheadRatioReporter MOZ_FINAL : public MemoryReporterBase
{
public:
    HeapOverheadRatioReporter()
      : MemoryReporterBase("heap-overhead-ratio", KIND_OTHER,
                           UNITS_PERCENTAGE,
"Ratio of committed, unused bytes to allocated bytes; i.e., "
"'heap-overhead' / 'heap-allocated'.  This measures the overhead of "
"the heap allocator relative to amount of memory allocated.")
    {}
private:
    int64_t Amount()
    {
        jemalloc_stats_t stats;
        jemalloc_stats(&stats);
        return (int64_t) 10000 *
          (stats.waste + stats.bookkeeping + stats.page_cache) /
          ((double)stats.allocated);
    }
};
#endif  






class AtomTablesReporter MOZ_FINAL : public MemoryReporterBase
{
public:
    AtomTablesReporter()
      : MemoryReporterBase("explicit/atom-tables", KIND_HEAP, UNITS_BYTES,
"Memory used by the dynamic and static atoms tables.")
    {}
private:
    int64_t Amount() { return NS_SizeOfAtomTablesIncludingThis(MallocSizeOf); }
};

#ifdef MOZ_DMD

namespace mozilla {
namespace dmd {

class DMDMultiReporter MOZ_FINAL : public nsIMemoryMultiReporter
{
public:
  DMDMultiReporter()
  {}

  NS_DECL_ISUPPORTS

  NS_IMETHOD GetName(nsACString& aName)
  {
    aName.Assign("dmd");
    return NS_OK;
  }

  NS_IMETHOD CollectReports(nsIMemoryMultiReporterCallback* aCallback,
                            nsISupports* aClosure)
  {
    dmd::Sizes sizes;
    dmd::SizeOf(&sizes);

#define REPORT(_path, _amount, _desc)                                         \
    do {                                                                      \
      nsresult rv;                                                            \
      rv = aCallback->Callback(EmptyCString(), NS_LITERAL_CSTRING(_path),     \
                               nsIMemoryReporter::KIND_HEAP,                  \
                               nsIMemoryReporter::UNITS_BYTES, _amount,       \
                               NS_LITERAL_CSTRING(_desc), aClosure);          \
      NS_ENSURE_SUCCESS(rv, rv);                                              \
    } while (0)

    REPORT("explicit/dmd/stack-traces/used",
           sizes.mStackTracesUsed,
           "Memory used by stack traces which correspond to at least "
           "one heap block DMD is tracking.");

    REPORT("explicit/dmd/stack-traces/unused",
           sizes.mStackTracesUnused,
           "Memory used by stack traces which don't correspond to any heap "
           "blocks DMD is currently tracking.");

    REPORT("explicit/dmd/stack-traces/table",
           sizes.mStackTraceTable,
           "Memory used by DMD's stack trace table.");

    REPORT("explicit/dmd/block-table",
           sizes.mBlockTable,
           "Memory used by DMD's live block table.");

#undef REPORT

    return NS_OK;
  }
};

NS_IMPL_ISUPPORTS1(DMDMultiReporter, nsIMemoryMultiReporter)

} 
} 

#endif  





NS_IMPL_ISUPPORTS1(nsMemoryReporterManager, nsIMemoryReporterManager)

NS_IMETHODIMP
nsMemoryReporterManager::Init()
{
#if defined(HAVE_JEMALLOC_STATS) && defined(XP_LINUX)
    if (!jemalloc_stats)
        return NS_ERROR_FAILURE;
#endif

#ifdef HAVE_JEMALLOC_STATS
    RegisterReporter(new HeapAllocatedReporter);
    RegisterReporter(new HeapOverheadWasteReporter);
    RegisterReporter(new HeapOverheadBookkeepingReporter);
    RegisterReporter(new HeapOverheadPageCacheReporter);
    RegisterReporter(new HeapCommittedReporter);
    RegisterReporter(new HeapOverheadRatioReporter);
#endif

#ifdef HAVE_VSIZE_AND_RESIDENT_REPORTERS
    RegisterReporter(new VsizeReporter);
    RegisterReporter(new ResidentReporter);
    RegisterReporter(new ResidentFastReporter);
#endif

#ifdef HAVE_PAGE_FAULT_REPORTERS
    RegisterReporter(new PageFaultsSoftReporter);
    RegisterReporter(new PageFaultsHardReporter);
#endif

#ifdef HAVE_PRIVATE_REPORTER
    RegisterReporter(new PrivateReporter);
#endif

    RegisterReporter(new AtomTablesReporter);

#ifdef MOZ_DMD
    RegisterMultiReporter(new mozilla::dmd::DMDMultiReporter);
#endif

#if defined(XP_LINUX)
    nsMemoryInfoDumper::Initialize();
#endif

    return NS_OK;
}

namespace {









class HashtableEnumerator MOZ_FINAL : public nsISimpleEnumerator
{
public:
    HashtableEnumerator(nsTHashtable<nsISupportsHashKey>& aHashtable)
        : mIndex(0)
    {
        aHashtable.EnumerateEntries(EnumeratorFunc, this);
    }

    NS_DECL_ISUPPORTS
    NS_DECL_NSISIMPLEENUMERATOR

private:
    static PLDHashOperator
    EnumeratorFunc(nsISupportsHashKey* aEntry, void* aData);

    uint32_t mIndex;
    nsCOMArray<nsISupports> mArray;
};

NS_IMPL_ISUPPORTS1(HashtableEnumerator, nsISimpleEnumerator)

 PLDHashOperator
HashtableEnumerator::EnumeratorFunc(nsISupportsHashKey* aElem, void* aData)
{
    HashtableEnumerator* enumerator = static_cast<HashtableEnumerator*>(aData);
    enumerator->mArray.AppendObject(aElem->GetKey());
    return PL_DHASH_NEXT;
}

NS_IMETHODIMP
HashtableEnumerator::HasMoreElements(bool* aResult)
{
    *aResult = mIndex < mArray.Length();
    return NS_OK;
}

NS_IMETHODIMP
HashtableEnumerator::GetNext(nsISupports** aNext)
{
    if (mIndex < mArray.Length()) {
        nsCOMPtr<nsISupports> next = mArray.ObjectAt(mIndex);
        next.forget(aNext);
        mIndex++;
        return NS_OK;
    }

    *aNext = nullptr;
    return NS_ERROR_FAILURE;
}

} 

nsMemoryReporterManager::nsMemoryReporterManager()
  : mMutex("nsMemoryReporterManager::mMutex"),
    mIsRegistrationBlocked(false)
{
    mReporters.Init();
    mMultiReporters.Init();
}

nsMemoryReporterManager::~nsMemoryReporterManager()
{
}

NS_IMETHODIMP
nsMemoryReporterManager::EnumerateReporters(nsISimpleEnumerator** aResult)
{
    
    
    if (!NS_IsMainThread()) {
        MOZ_CRASH();
    }

    mozilla::MutexAutoLock autoLock(mMutex);

    nsRefPtr<HashtableEnumerator> enumerator =
        new HashtableEnumerator(mReporters);
    enumerator.forget(aResult);
    return NS_OK;
}

NS_IMETHODIMP
nsMemoryReporterManager::EnumerateMultiReporters(nsISimpleEnumerator** aResult)
{
    
    
    if (!NS_IsMainThread()) {
        MOZ_CRASH();
    }

    mozilla::MutexAutoLock autoLock(mMutex);

    nsRefPtr<HashtableEnumerator> enumerator =
        new HashtableEnumerator(mMultiReporters);
    enumerator.forget(aResult);
    return NS_OK;
}

static void
DebugAssertRefcountIsNonZero(nsISupports* aObj)
{
#ifdef DEBUG
    
    uint32_t refcnt = NS_ADDREF(aObj);
    MOZ_ASSERT(refcnt >= 2);
    NS_RELEASE(aObj);
#endif
}

nsresult
nsMemoryReporterManager::RegisterReporterHelper(
    nsIMemoryReporter* aReporter, bool aForce)
{
    
    mozilla::MutexAutoLock autoLock(mMutex);

    if ((mIsRegistrationBlocked && !aForce) || mReporters.Contains(aReporter)) {
        return NS_ERROR_FAILURE;
    }

    
    
    
    
    
    
    
    
    

    {
        nsCOMPtr<nsIMemoryReporter> kungFuDeathGrip = aReporter;
        mReporters.PutEntry(aReporter);
    }

    DebugAssertRefcountIsNonZero(aReporter);

    return NS_OK;
}

NS_IMETHODIMP
nsMemoryReporterManager::RegisterReporter(nsIMemoryReporter* aReporter)
{
    return RegisterReporterHelper(aReporter,  false);
}

NS_IMETHODIMP
nsMemoryReporterManager::RegisterReporterEvenIfBlocked(
    nsIMemoryReporter* aReporter)
{
    return RegisterReporterHelper(aReporter,  true);
}

nsresult
nsMemoryReporterManager::RegisterMultiReporterHelper(
    nsIMemoryMultiReporter* aReporter, bool aForce)
{
    
    mozilla::MutexAutoLock autoLock(mMutex);

    if ((mIsRegistrationBlocked && !aForce) ||
         mMultiReporters.Contains(aReporter)) {
        return NS_ERROR_FAILURE;
    }

    {
        nsCOMPtr<nsIMemoryMultiReporter> kungFuDeathGrip = aReporter;
        mMultiReporters.PutEntry(aReporter);
    }

    DebugAssertRefcountIsNonZero(aReporter);

    return NS_OK;
}

NS_IMETHODIMP
nsMemoryReporterManager::RegisterMultiReporter(nsIMemoryMultiReporter* aReporter)
{
    return RegisterMultiReporterHelper(aReporter,  false);
}

NS_IMETHODIMP
nsMemoryReporterManager::RegisterMultiReporterEvenIfBlocked(
    nsIMemoryMultiReporter* aReporter)
{
    return RegisterMultiReporterHelper(aReporter,  true);
}

NS_IMETHODIMP
nsMemoryReporterManager::UnregisterReporter(nsIMemoryReporter* aReporter)
{
    
    mozilla::MutexAutoLock autoLock(mMutex);

    if (!mReporters.Contains(aReporter)) {
        return NS_ERROR_FAILURE;
    }

    mReporters.RemoveEntry(aReporter);
    return NS_OK;
}

NS_IMETHODIMP
nsMemoryReporterManager::UnregisterMultiReporter(nsIMemoryMultiReporter* aReporter)
{
    
    mozilla::MutexAutoLock autoLock(mMutex);

    if (!mMultiReporters.Contains(aReporter)) {
        return NS_ERROR_FAILURE;
    }

    mMultiReporters.RemoveEntry(aReporter);
    return NS_OK;
}

NS_IMETHODIMP
nsMemoryReporterManager::BlockRegistration()
{
    
    mozilla::MutexAutoLock autoLock(mMutex);
    if (mIsRegistrationBlocked) {
        return NS_ERROR_FAILURE;
    }
    mIsRegistrationBlocked = true;
    return NS_OK;
}

NS_IMETHODIMP
nsMemoryReporterManager::UnblockRegistration()
{
    
    mozilla::MutexAutoLock autoLock(mMutex);
    if (!mIsRegistrationBlocked) {
        return NS_ERROR_FAILURE;
    }
    mIsRegistrationBlocked = false;
    return NS_OK;
}

NS_IMETHODIMP
nsMemoryReporterManager::GetResident(int64_t* aResident)
{
#ifdef HAVE_VSIZE_AND_RESIDENT_REPORTERS
    return ::GetResident(aResident);
#else
    *aResident = 0;
    return NS_ERROR_NOT_AVAILABLE;
#endif
}



class Int64Wrapper MOZ_FINAL : public nsISupports {
public:
    NS_DECL_ISUPPORTS
    Int64Wrapper() : mValue(0) { }
    int64_t mValue;
};
NS_IMPL_ISUPPORTS0(Int64Wrapper)

class ExplicitNonHeapCountingCallback MOZ_FINAL : public nsIMemoryMultiReporterCallback
{
public:
    NS_DECL_ISUPPORTS

    NS_IMETHOD Callback(const nsACString& aProcess, const nsACString& aPath,
                        int32_t aKind, int32_t aUnits, int64_t aAmount,
                        const nsACString& aDescription,
                        nsISupports* aWrappedExplicitNonHeap)
    {
        if (aKind == nsIMemoryReporter::KIND_NONHEAP &&
            PromiseFlatCString(aPath).Find("explicit") == 0 &&
            aAmount != int64_t(-1))
        {
            Int64Wrapper* wrappedPRInt64 =
                static_cast<Int64Wrapper*>(aWrappedExplicitNonHeap);
            wrappedPRInt64->mValue += aAmount;
        }
        return NS_OK;
    }
};
NS_IMPL_ISUPPORTS1(
  ExplicitNonHeapCountingCallback
, nsIMemoryMultiReporterCallback
)

NS_IMETHODIMP
nsMemoryReporterManager::GetExplicit(int64_t* aExplicit)
{
    NS_ENSURE_ARG_POINTER(aExplicit);
    *aExplicit = 0;
#ifndef HAVE_JEMALLOC_STATS
    return NS_ERROR_NOT_AVAILABLE;
#else
    nsresult rv;
    bool more;

    
    
    int64_t heapAllocated = int64_t(-1);
    int64_t explicitNonHeapNormalSize = 0;
    nsCOMPtr<nsISimpleEnumerator> e;
    EnumerateReporters(getter_AddRefs(e));
    while (NS_SUCCEEDED(e->HasMoreElements(&more)) && more) {
        nsCOMPtr<nsIMemoryReporter> r;
        e->GetNext(getter_AddRefs(r));

        int32_t kind;
        rv = r->GetKind(&kind);
        NS_ENSURE_SUCCESS(rv, rv);

        nsCString path;
        rv = r->GetPath(path);
        NS_ENSURE_SUCCESS(rv, rv);

        
        
        if (kind == nsIMemoryReporter::KIND_NONHEAP &&
            path.Find("explicit") == 0)
        {
            
            
            int64_t amount;
            rv = r->GetAmount(&amount);
            if (NS_SUCCEEDED(rv)) {
                explicitNonHeapNormalSize += amount;
            }
        } else if (path.Equals("heap-allocated")) {
            
            
            rv = r->GetAmount(&heapAllocated);
            NS_ENSURE_SUCCESS(rv, rv);
        }
    }

    
    
    
    

    nsRefPtr<ExplicitNonHeapCountingCallback> cb =
      new ExplicitNonHeapCountingCallback();
    nsRefPtr<Int64Wrapper> wrappedExplicitNonHeapMultiSize =
      new Int64Wrapper();
    nsCOMPtr<nsISimpleEnumerator> e2;
    EnumerateMultiReporters(getter_AddRefs(e2));
    while (NS_SUCCEEDED(e2->HasMoreElements(&more)) && more) {
      nsCOMPtr<nsIMemoryMultiReporter> r;
      e2->GetNext(getter_AddRefs(r));
      r->CollectReports(cb, wrappedExplicitNonHeapMultiSize);
    }
    int64_t explicitNonHeapMultiSize = wrappedExplicitNonHeapMultiSize->mValue;

    *aExplicit = heapAllocated + explicitNonHeapNormalSize + explicitNonHeapMultiSize;
    return NS_OK;
#endif 
}

NS_IMETHODIMP
nsMemoryReporterManager::GetHasMozMallocUsableSize(bool* aHas)
{
    void* p = malloc(16);
    if (!p) {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    size_t usable = moz_malloc_usable_size(p);
    free(p);
    *aHas = !!(usable > 0);
    return NS_OK;
}

namespace {









class MinimizeMemoryUsageRunnable : public nsCancelableRunnable
{
public:
  MinimizeMemoryUsageRunnable(nsIRunnable* aCallback)
    : mCallback(aCallback)
    , mRemainingIters(sNumIters)
    , mCanceled(false)
  {}

  NS_IMETHOD Run()
  {
    if (mCanceled) {
      return NS_OK;
    }

    nsCOMPtr<nsIObserverService> os = services::GetObserverService();
    if (!os) {
      return NS_ERROR_FAILURE;
    }

    if (mRemainingIters == 0) {
      os->NotifyObservers(nullptr, "after-minimize-memory-usage",
                          NS_LITERAL_STRING("MinimizeMemoryUsageRunnable").get());
      if (mCallback) {
        mCallback->Run();
      }
      return NS_OK;
    }

    os->NotifyObservers(nullptr, "memory-pressure",
                        NS_LITERAL_STRING("heap-minimize").get());
    mRemainingIters--;
    NS_DispatchToMainThread(this);

    return NS_OK;
  }

  NS_IMETHOD Cancel()
  {
    if (mCanceled) {
      return NS_ERROR_UNEXPECTED;
    }

    mCanceled = true;

    return NS_OK;
  }

private:
  
  
  
  static const uint32_t sNumIters = 3;

  nsCOMPtr<nsIRunnable> mCallback;
  uint32_t mRemainingIters;
  bool mCanceled;
};

} 

NS_IMETHODIMP
nsMemoryReporterManager::MinimizeMemoryUsage(nsIRunnable* aCallback,
                                             nsICancelableRunnable** aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);

  nsRefPtr<nsICancelableRunnable> runnable =
    new MinimizeMemoryUsageRunnable(aCallback);
  NS_ADDREF(*aResult = runnable);

  return NS_DispatchToMainThread(runnable);
}




NS_IMPL_ISUPPORTS1(MemoryReporterBase, nsIMemoryReporter)

nsresult
NS_RegisterMemoryReporter(nsIMemoryReporter* aReporter)
{
    nsCOMPtr<nsIMemoryReporterManager> mgr = do_GetService("@mozilla.org/memory-reporter-manager;1");
    if (!mgr) {
        return NS_ERROR_FAILURE;
    }
    return mgr->RegisterReporter(aReporter);
}

nsresult
NS_RegisterMemoryMultiReporter(nsIMemoryMultiReporter* aReporter)
{
    nsCOMPtr<nsIMemoryReporterManager> mgr = do_GetService("@mozilla.org/memory-reporter-manager;1");
    if (!mgr) {
        return NS_ERROR_FAILURE;
    }
    return mgr->RegisterMultiReporter(aReporter);
}

nsresult
NS_UnregisterMemoryReporter(nsIMemoryReporter* aReporter)
{
    nsCOMPtr<nsIMemoryReporterManager> mgr = do_GetService("@mozilla.org/memory-reporter-manager;1");
    if (!mgr) {
        return NS_ERROR_FAILURE;
    }
    return mgr->UnregisterReporter(aReporter);
}

nsresult
NS_UnregisterMemoryMultiReporter(nsIMemoryMultiReporter* aReporter)
{
    nsCOMPtr<nsIMemoryReporterManager> mgr = do_GetService("@mozilla.org/memory-reporter-manager;1");
    if (!mgr) {
        return NS_ERROR_FAILURE;
    }
    return mgr->UnregisterMultiReporter(aReporter);
}

#if defined(MOZ_DMD)

namespace mozilla {
namespace dmd {

class NullMultiReporterCallback : public nsIMemoryMultiReporterCallback
{
public:
    NS_DECL_ISUPPORTS

    NS_IMETHOD Callback(const nsACString& aProcess, const nsACString& aPath,
                        int32_t aKind, int32_t aUnits, int64_t aAmount,
                        const nsACString& aDescription,
                        nsISupports* aData)
    {
        
        return NS_OK;
    }
};
NS_IMPL_ISUPPORTS1(
  NullMultiReporterCallback
, nsIMemoryMultiReporterCallback
)

void
RunReporters()
{
    nsCOMPtr<nsIMemoryReporterManager> mgr =
        do_GetService("@mozilla.org/memory-reporter-manager;1");

    
    nsCOMPtr<nsISimpleEnumerator> e;
    mgr->EnumerateReporters(getter_AddRefs(e));
    bool more;
    while (NS_SUCCEEDED(e->HasMoreElements(&more)) && more) {
        nsCOMPtr<nsIMemoryReporter> r;
        e->GetNext(getter_AddRefs(r));

        int32_t kind;
        nsresult rv = r->GetKind(&kind);
        if (NS_FAILED(rv)) {
            continue;
        }
        nsCString path;
        rv = r->GetPath(path);
        if (NS_FAILED(rv)) {
            continue;
        }

        
        
        
        
        
        if (kind == nsIMemoryReporter::KIND_HEAP &&
            path.Find("explicit") == 0)
        {
            
            
            int64_t amount;
            (void)r->GetAmount(&amount);
        }
    }

    
    nsCOMPtr<nsISimpleEnumerator> e2;
    mgr->EnumerateMultiReporters(getter_AddRefs(e2));
    nsRefPtr<NullMultiReporterCallback> cb = new NullMultiReporterCallback();
    while (NS_SUCCEEDED(e2->HasMoreElements(&more)) && more) {
      nsCOMPtr<nsIMemoryMultiReporter> r;
      e2->GetNext(getter_AddRefs(r));
      r->CollectReports(cb, nullptr);
    }
}

} 
} 

#endif  


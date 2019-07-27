





#include "gc/Memory.h"

#include "mozilla/Atomics.h"
#include "mozilla/TaggedAnonymousMemory.h"

#include "js/HeapAPI.h"
#include "vm/Runtime.h"

#if defined(XP_WIN)

#include "jswin.h"
#include <psapi.h>

#elif defined(SOLARIS)

#include <sys/mman.h>
#include <unistd.h>

#elif defined(XP_UNIX)

#include <algorithm>
#include <errno.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#endif

namespace js {
namespace gc {



static size_t pageSize = 0;


static size_t allocGranularity = 0;

#if defined(XP_UNIX)

static mozilla::Atomic<int, mozilla::Relaxed> growthDirection(0);
#endif





static const int MaxLastDitchAttempts = 32;

static void GetNewChunk(void **aAddress, void **aRetainedAddr, size_t size, size_t alignment);
static void *MapAlignedPagesSlow(size_t size, size_t alignment);
static void *MapAlignedPagesLastDitch(size_t size, size_t alignment);

size_t
SystemPageSize()
{
    return pageSize;
}

static bool
DecommitEnabled()
{
    return pageSize == ArenaSize;
}







static inline size_t
OffsetFromAligned(void *p, size_t alignment)
{
    return uintptr_t(p) % alignment;
}

void *
TestMapAlignedPagesLastDitch(size_t size, size_t alignment)
{
    return MapAlignedPagesLastDitch(size, alignment);
}


#if defined(XP_WIN)

void
InitMemorySubsystem()
{
    if (pageSize == 0) {
        SYSTEM_INFO sysinfo;
        GetSystemInfo(&sysinfo);
        pageSize = sysinfo.dwPageSize;
        allocGranularity = sysinfo.dwAllocationGranularity;
    }
}

static inline void *
MapMemoryAt(void *desired, size_t length, int flags, int prot = PAGE_READWRITE)
{
    return VirtualAlloc(desired, length, flags, prot);
}

static inline void *
MapMemory(size_t length, int flags, int prot = PAGE_READWRITE)
{
    return VirtualAlloc(nullptr, length, flags, prot);
}

void *
MapAlignedPages(size_t size, size_t alignment)
{
    MOZ_ASSERT(size >= alignment);
    MOZ_ASSERT(size % alignment == 0);
    MOZ_ASSERT(size % pageSize == 0);
    MOZ_ASSERT(alignment % allocGranularity == 0);

    void *p = MapMemory(size, MEM_COMMIT | MEM_RESERVE);

    
    if (alignment == allocGranularity)
        return p;

    if (OffsetFromAligned(p, alignment) == 0)
        return p;

    void *retainedAddr;
    GetNewChunk(&p, &retainedAddr, size, alignment);
    if (retainedAddr)
        UnmapPages(retainedAddr, size);
    if (p) {
        if (OffsetFromAligned(p, alignment) == 0)
            return p;
        UnmapPages(p, size);
    }

    p = MapAlignedPagesSlow(size, alignment);
    if (!p)
        return MapAlignedPagesLastDitch(size, alignment);

    MOZ_ASSERT(OffsetFromAligned(p, alignment) == 0);
    return p;
}

static void *
MapAlignedPagesSlow(size_t size, size_t alignment)
{
    





    void *p;
    do {
        







        size_t reserveSize = size + alignment - pageSize;
        p = MapMemory(reserveSize, MEM_RESERVE);
        if (!p)
            return nullptr;
        void *chunkStart = (void *)AlignBytes(uintptr_t(p), alignment);
        UnmapPages(p, reserveSize);
        p = MapMemoryAt(chunkStart, size, MEM_COMMIT | MEM_RESERVE);

        
    } while (!p);

    return p;
}









static void *
MapAlignedPagesLastDitch(size_t size, size_t alignment)
{
    void *tempMaps[MaxLastDitchAttempts];
    int attempt = 0;
    void *p = MapMemory(size, MEM_COMMIT | MEM_RESERVE);
    if (OffsetFromAligned(p, alignment) == 0)
        return p;
    for (; attempt < MaxLastDitchAttempts; ++attempt) {
        GetNewChunk(&p, tempMaps + attempt, size, alignment);
        if (OffsetFromAligned(p, alignment) == 0) {
            if (tempMaps[attempt])
                UnmapPages(tempMaps[attempt], size);
            break;
        }
        if (!tempMaps[attempt])
            break; 
    }
    if (OffsetFromAligned(p, alignment)) {
        UnmapPages(p, size);
        p = nullptr;
    }
    while (--attempt >= 0)
        UnmapPages(tempMaps[attempt], size);
    return p;
}






static void
GetNewChunk(void **aAddress, void **aRetainedAddr, size_t size, size_t alignment)
{
    void *address = *aAddress;
    void *retainedAddr = nullptr;
    do {
        size_t retainedSize;
        size_t offset = OffsetFromAligned(address, alignment);
        if (!offset)
            break;
        UnmapPages(address, size);
        retainedSize = alignment - offset;
        retainedAddr = MapMemoryAt(address, retainedSize, MEM_RESERVE);
        address = MapMemory(size, MEM_COMMIT | MEM_RESERVE);
        
    } while (!retainedAddr);
    *aAddress = address;
    *aRetainedAddr = retainedAddr;
}

void
UnmapPages(void *p, size_t size)
{
    MOZ_ALWAYS_TRUE(VirtualFree(p, 0, MEM_RELEASE));
}

bool
MarkPagesUnused(void *p, size_t size)
{
    if (!DecommitEnabled())
        return true;

    MOZ_ASSERT(OffsetFromAligned(p, pageSize) == 0);
    LPVOID p2 = MapMemoryAt(p, size, MEM_RESET);
    return p2 == p;
}

bool
MarkPagesInUse(void *p, size_t size)
{
    if (!DecommitEnabled())
        return true;

    MOZ_ASSERT(OffsetFromAligned(p, pageSize) == 0);
    return true;
}

size_t
GetPageFaultCount()
{
    PROCESS_MEMORY_COUNTERS pmc;
    if (!GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
        return 0;
    return pmc.PageFaultCount;
}

void *
AllocateMappedContent(int fd, size_t offset, size_t length, size_t alignment)
{
    
    return nullptr;
}


void
DeallocateMappedContent(void *p, size_t length)
{
    
}

#elif defined(SOLARIS)

#ifndef MAP_NOSYNC
# define MAP_NOSYNC 0
#endif

void
InitMemorySubsystem()
{
    if (pageSize == 0)
        pageSize = allocGranularity = size_t(sysconf(_SC_PAGESIZE));
}

void *
MapAlignedPages(size_t size, size_t alignment)
{
    MOZ_ASSERT(size >= alignment);
    MOZ_ASSERT(size % alignment == 0);
    MOZ_ASSERT(size % pageSize == 0);
    MOZ_ASSERT(alignment % allocGranularity == 0);

    int prot = PROT_READ | PROT_WRITE;
    int flags = MAP_PRIVATE | MAP_ANON | MAP_ALIGN | MAP_NOSYNC;

    void *p = mmap((caddr_t)alignment, size, prot, flags, -1, 0);
    if (p == MAP_FAILED)
        return nullptr;
    return p;
}

void
UnmapPages(void *p, size_t size)
{
    MOZ_ALWAYS_TRUE(0 == munmap((caddr_t)p, size));
}

bool
MarkPagesUnused(void *p, size_t size)
{
    MOZ_ASSERT(OffsetFromAligned(p, pageSize) == 0);
    return true;
}

bool
MarkPagesInUse(void *p, size_t size)
{
    if (!DecommitEnabled())
        return true;

    MOZ_ASSERT(OffsetFromAligned(p, pageSize) == 0);
    return true;
}

size_t
GetPageFaultCount()
{
    return 0;
}

void *
AllocateMappedContent(int fd, size_t offset, size_t length, size_t alignment)
{
    
    return nullptr;
}


void
DeallocateMappedContent(void *p, size_t length)
{
    
}

#elif defined(XP_UNIX)

void
InitMemorySubsystem()
{
    if (pageSize == 0)
        pageSize = allocGranularity = size_t(sysconf(_SC_PAGESIZE));
}

static inline void *
MapMemoryAt(void *desired, size_t length, int prot = PROT_READ | PROT_WRITE,
            int flags = MAP_PRIVATE | MAP_ANON, int fd = -1, off_t offset = 0)
{
#if defined(__ia64__) || (defined(__sparc64__) && defined(__NetBSD__))
    MOZ_ASSERT(0xffff800000000000ULL & (uintptr_t(desired) + length - 1) == 0);
#endif
    void *region = mmap(desired, length, prot, flags, fd, offset);
    if (region == MAP_FAILED)
        return nullptr;
    




    if (region != desired) {
        if (munmap(region, length))
            MOZ_ASSERT(errno == ENOMEM);
        return nullptr;
    }
    return region;
}

static inline void *
MapMemory(size_t length, int prot = PROT_READ | PROT_WRITE,
          int flags = MAP_PRIVATE | MAP_ANON, int fd = -1, off_t offset = 0)
{
#if defined(__ia64__) || (defined(__sparc64__) && defined(__NetBSD__))
    












    void *region = mmap((void*)0x0000070000000000, length, prot, flags, fd, offset);
    if (region == MAP_FAILED)
        return nullptr;
    



    if ((uintptr_t(region) + (length - 1)) & 0xffff800000000000) {
        if (munmap(region, length))
            MOZ_ASSERT(errno == ENOMEM);
        return nullptr;
    }
    return region;
#else
    void *region = MozTaggedAnonymousMmap(nullptr, length, prot, flags, fd, offset, "js-gc-heap");
    if (region == MAP_FAILED)
        return nullptr;
    return region;
#endif
}

void *
MapAlignedPages(size_t size, size_t alignment)
{
    MOZ_ASSERT(size >= alignment);
    MOZ_ASSERT(size % alignment == 0);
    MOZ_ASSERT(size % pageSize == 0);
    MOZ_ASSERT(alignment % allocGranularity == 0);

    void *p = MapMemory(size);

    
    if (alignment == allocGranularity)
        return p;

    if (OffsetFromAligned(p, alignment) == 0)
        return p;

    void *retainedAddr;
    GetNewChunk(&p, &retainedAddr, size, alignment);
    if (retainedAddr)
        UnmapPages(retainedAddr, size);
    if (p) {
        if (OffsetFromAligned(p, alignment) == 0)
            return p;
        UnmapPages(p, size);
    }

    p = MapAlignedPagesSlow(size, alignment);
    if (!p)
        return MapAlignedPagesLastDitch(size, alignment);

    MOZ_ASSERT(OffsetFromAligned(p, alignment) == 0);
    return p;
}

static void *
MapAlignedPagesSlow(size_t size, size_t alignment)
{
    
    size_t reqSize = size + alignment - pageSize;
    void *region = MapMemory(reqSize);
    if (!region)
        return nullptr;

    void *regionEnd = (void *)(uintptr_t(region) + reqSize);
    void *front;
    void *end;
    if (growthDirection <= 0) {
        size_t offset = OffsetFromAligned(regionEnd, alignment);
        end = (void *)(uintptr_t(regionEnd) - offset);
        front = (void *)(uintptr_t(end) - size);
    } else {
        size_t offset = OffsetFromAligned(region, alignment);
        front = (void *)(uintptr_t(region) + (offset ? alignment - offset : 0));
        end = (void *)(uintptr_t(front) + size);
    }

    if (front != region)
        UnmapPages(region, uintptr_t(front) - uintptr_t(region));
    if (end != regionEnd)
        UnmapPages(end, uintptr_t(regionEnd) - uintptr_t(end));

    return front;
}









static void *
MapAlignedPagesLastDitch(size_t size, size_t alignment)
{
    void *tempMaps[MaxLastDitchAttempts];
    int attempt = 0;
    void *p = MapMemory(size);
    if (OffsetFromAligned(p, alignment) == 0)
        return p;
    for (; attempt < MaxLastDitchAttempts; ++attempt) {
        GetNewChunk(&p, tempMaps + attempt, size, alignment);
        if (OffsetFromAligned(p, alignment) == 0) {
            if (tempMaps[attempt])
                UnmapPages(tempMaps[attempt], size);
            break;
        }
        if (!tempMaps[attempt])
            break; 
    }
    if (OffsetFromAligned(p, alignment)) {
        UnmapPages(p, size);
        p = nullptr;
    }
    while (--attempt >= 0)
        UnmapPages(tempMaps[attempt], size);
    return p;
}







static void
GetNewChunk(void **aAddress, void **aRetainedAddr, size_t size, size_t alignment)
{
    void *address = *aAddress;
    void *retainedAddr = nullptr;
    bool addrsGrowDown = growthDirection <= 0;
    int i = 0;
    for (; i < 2; ++i) {
        
        if (addrsGrowDown) {
            size_t offset = OffsetFromAligned(address, alignment);
            void *head = (void *)((uintptr_t)address - offset);
            void *tail = (void *)((uintptr_t)head + size);
            if (MapMemoryAt(head, offset)) {
                UnmapPages(tail, offset);
                if (growthDirection >= -8)
                    --growthDirection;
                address = head;
                break;
            }
        } else {
            size_t offset = alignment - OffsetFromAligned(address, alignment);
            void *head = (void *)((uintptr_t)address + offset);
            void *tail = (void *)((uintptr_t)address + size);
            if (MapMemoryAt(tail, offset)) {
                UnmapPages(address, offset);
                if (growthDirection <= 8)
                    ++growthDirection;
                address = head;
                break;
            }
        }
        
        if (growthDirection < -8 || growthDirection > 8)
            break;
        
        addrsGrowDown = !addrsGrowDown;
    }
    
    if (OffsetFromAligned(address, alignment)) {
        retainedAddr = address;
        address = MapMemory(size);
    }
    *aAddress = address;
    *aRetainedAddr = retainedAddr;
}

void
UnmapPages(void *p, size_t size)
{
    if (munmap(p, size))
        MOZ_ASSERT(errno == ENOMEM);
}

bool
MarkPagesUnused(void *p, size_t size)
{
    if (!DecommitEnabled())
        return false;

    MOZ_ASSERT(OffsetFromAligned(p, pageSize) == 0);
    int result = madvise(p, size, MADV_DONTNEED);
    return result != -1;
}

bool
MarkPagesInUse(void *p, size_t size)
{
    if (!DecommitEnabled())
        return true;

    MOZ_ASSERT(OffsetFromAligned(p, pageSize) == 0);
    return true;
}

size_t
GetPageFaultCount()
{
    struct rusage usage;
    int err = getrusage(RUSAGE_SELF, &usage);
    if (err)
        return 0;
    return usage.ru_majflt;
}

void *
AllocateMappedContent(int fd, size_t offset, size_t length, size_t alignment)
{
#define NEED_PAGE_ALIGNED 0
    size_t pa_start; 
    size_t pa_end; 
    size_t pa_size; 
    struct stat st;
    uint8_t *buf;

    
    if (fstat(fd, &st) < 0 || offset >= (size_t) st.st_size ||
        length == 0 || length > (size_t) st.st_size - offset)
        return nullptr;

    
#if NEED_PAGE_ALIGNED
    alignment = std::max(alignment, pageSize);
#endif
    if (offset & (alignment - 1))
        return nullptr;

    
    pa_start = offset & ~(pageSize - 1);
    
    
    pa_end = ((offset + length - 1) & ~(pageSize - 1)) + pageSize;
    pa_size = pa_end - pa_start;

    
    buf = (uint8_t *) MapMemory(pa_size);
    if (!buf)
        return nullptr;

    buf = (uint8_t *) MapMemoryAt(buf, pa_size, PROT_READ | PROT_WRITE,
                                  MAP_PRIVATE | MAP_FIXED, fd, pa_start);
    if (!buf)
        return nullptr;

    
    memset(buf, 0, offset - pa_start);

    
    memset(buf + (offset - pa_start) + length, 0, pa_end - (offset + length));

    return buf + (offset - pa_start);
}

void
DeallocateMappedContent(void *p, size_t length)
{
    void *pa_start; 
    size_t total_size; 

    pa_start = (void *)(uintptr_t(p) & ~(pageSize - 1));
    total_size = ((uintptr_t(p) + length) & ~(pageSize - 1)) + pageSize - uintptr_t(pa_start);
    if (munmap(pa_start, total_size))
        MOZ_ASSERT(errno == ENOMEM);
}

#else
#error "Memory mapping functions are not defined for your OS."
#endif

} 
} 







#include "gc/Memory.h"

#include "js/HeapAPI.h"
#include "vm/Runtime.h"

using namespace js;
using namespace js::gc;

bool
SystemPageAllocator::decommitEnabled()
{
    return pageSize == ArenaSize;
}







static inline size_t
OffsetFromAligned(void *p, size_t alignment)
{
    return uintptr_t(p) % alignment;
}

#if defined(XP_WIN)
#include "jswin.h"
#include <psapi.h>

SystemPageAllocator::SystemPageAllocator()
{
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    pageSize = sysinfo.dwPageSize;
    allocGranularity = sysinfo.dwAllocationGranularity;
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
SystemPageAllocator::mapAlignedPages(size_t size, size_t alignment)
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
    size_t retainedSize;
    getNewChunk(&p, &retainedAddr, &retainedSize, size, alignment);
    if (retainedAddr)
        unmapPages(retainedAddr, retainedSize);
    if (p) {
        if (OffsetFromAligned(p, alignment) == 0)
            return p;
        unmapPages(p, size);
    }

    p = mapAlignedPagesSlow(size, alignment);
    if (!p)
        return mapAlignedPagesLastDitch(size, alignment);

    MOZ_ASSERT(OffsetFromAligned(p, alignment) == 0);
    return p;
}

void *
SystemPageAllocator::mapAlignedPagesSlow(size_t size, size_t alignment)
{
    





    void *p;
    do {
        







        size_t reserveSize = size + alignment - pageSize;
        p = MapMemory(reserveSize, MEM_RESERVE);
        if (!p)
            return nullptr;
        void *chunkStart = (void *)AlignBytes(uintptr_t(p), alignment);
        unmapPages(p, reserveSize);
        p = MapMemoryAt(chunkStart, size, MEM_COMMIT | MEM_RESERVE);

        
    } while (!p);

    return p;
}









void *
SystemPageAllocator::mapAlignedPagesLastDitch(size_t size, size_t alignment)
{
    void *p = nullptr;
    void *tempMaps[MaxLastDitchAttempts];
    int attempt = 0;
    for (; attempt < MaxLastDitchAttempts; ++attempt) {
        size_t retainedSize;
        getNewChunk(&p, tempMaps + attempt, &retainedSize, size, alignment);
        if (OffsetFromAligned(p, alignment) == 0) {
            if (tempMaps[attempt])
                unmapPages(tempMaps[attempt], retainedSize);
            break;
        }
        if (!tempMaps[attempt]) {
            
            tempMaps[attempt] = p;
            p = nullptr;
        }
    }
    if (OffsetFromAligned(p, alignment)) {
        unmapPages(p, size);
        p = nullptr;
    }
    while (--attempt >= 0)
        unmapPages(tempMaps[attempt], 0);
    return p;
}






void
SystemPageAllocator::getNewChunk(void **aAddress, void **aRetainedAddr, size_t *aRetainedSize,
                                 size_t size, size_t alignment)
{
    void *address = *aAddress;
    void *retainedAddr = nullptr;
    size_t retainedSize = 0;
    do {
        if (!address)
            address = MapMemory(size, MEM_COMMIT | MEM_RESERVE);
        size_t offset = OffsetFromAligned(address, alignment);
        if (!offset)
            break;
        unmapPages(address, size);
        retainedSize = alignment - offset;
        retainedAddr = MapMemoryAt(address, retainedSize, MEM_RESERVE);
        address = MapMemory(size, MEM_COMMIT | MEM_RESERVE);
        
    } while (!retainedAddr);
    *aAddress = address;
    *aRetainedAddr = retainedAddr;
    *aRetainedSize = retainedSize;
}

void
SystemPageAllocator::unmapPages(void *p, size_t size)
{
    MOZ_ALWAYS_TRUE(VirtualFree(p, 0, MEM_RELEASE));
}

bool
SystemPageAllocator::markPagesUnused(void *p, size_t size)
{
    if (!decommitEnabled())
        return true;

    MOZ_ASSERT(OffsetFromAligned(p, pageSize) == 0);
    LPVOID p2 = MapMemoryAt(p, size, MEM_RESET);
    return p2 == p;
}

bool
SystemPageAllocator::markPagesInUse(void *p, size_t size)
{
    MOZ_ASSERT(OffsetFromAligned(p, pageSize) == 0);
    return true;
}

size_t
SystemPageAllocator::GetPageFaultCount()
{
    PROCESS_MEMORY_COUNTERS pmc;
    if (!GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
        return 0;
    return pmc.PageFaultCount;
}

void *
SystemPageAllocator::AllocateMappedContent(int fd, size_t offset, size_t length, size_t alignment)
{
    
    return nullptr;
}


void
SystemPageAllocator::DeallocateMappedContent(void *p, size_t length)
{
    
}

#elif defined(SOLARIS)

#include <sys/mman.h>
#include <unistd.h>

#ifndef MAP_NOSYNC
# define MAP_NOSYNC 0
#endif

SystemPageAllocator::SystemPageAllocator()
{
    pageSize = allocGranularity = size_t(sysconf(_SC_PAGESIZE));
}

void *
SystemPageAllocator::mapAlignedPages(size_t size, size_t alignment)
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
SystemPageAllocator::unmapPages(void *p, size_t size)
{
    MOZ_ALWAYS_TRUE(0 == munmap((caddr_t)p, size));
}

bool
SystemPageAllocator::markPagesUnused(void *p, size_t size)
{
    MOZ_ASSERT(OffsetFromAligned(p, pageSize) == 0);
    return true;
}

bool
SystemPageAllocator::markPagesInUse(void *p, size_t size)
{
    MOZ_ASSERT(OffsetFromAligned(p, pageSize) == 0);
    return true;
}

size_t
SystemPageAllocator::GetPageFaultCount()
{
    return 0;
}

void *
SystemPageAllocator::AllocateMappedContent(int fd, size_t offset, size_t length, size_t alignment)
{
    
    return nullptr;
}


void
SystemPageAllocator::DeallocateMappedContent(void *p, size_t length)
{
    
}

#elif defined(XP_UNIX)

#include <algorithm>
#include <errno.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

SystemPageAllocator::SystemPageAllocator()
{
    pageSize = allocGranularity = size_t(sysconf(_SC_PAGESIZE));
    growthDirection = 0;
}

static inline void *
MapMemoryAt(void *desired, size_t length, int prot = PROT_READ | PROT_WRITE,
            int flags = MAP_PRIVATE | MAP_ANON, int fd = -1, off_t offset = 0)
{
#if defined(__ia64__)
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
#if defined(__ia64__)
    












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
    void *region = mmap(nullptr, length, prot, flags, fd, offset);
    if (region == MAP_FAILED)
        return nullptr;
    return region;
#endif
}

void *
SystemPageAllocator::mapAlignedPages(size_t size, size_t alignment)
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
    size_t retainedSize;
    getNewChunk(&p, &retainedAddr, &retainedSize, size, alignment);
    if (retainedAddr)
        unmapPages(retainedAddr, retainedSize);
    if (p) {
        if (OffsetFromAligned(p, alignment) == 0)
            return p;
        unmapPages(p, size);
    }

    p = mapAlignedPagesSlow(size, alignment);
    if (!p)
        return mapAlignedPagesLastDitch(size, alignment);

    MOZ_ASSERT(OffsetFromAligned(p, alignment) == 0);
    return p;
}

void *
SystemPageAllocator::mapAlignedPagesSlow(size_t size, size_t alignment)
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
        unmapPages(region, uintptr_t(front) - uintptr_t(region));
    if (end != regionEnd)
        unmapPages(end, uintptr_t(regionEnd) - uintptr_t(end));

    return front;
}









void *
SystemPageAllocator::mapAlignedPagesLastDitch(size_t size, size_t alignment)
{
    void *p = nullptr;
    void *tempMaps[MaxLastDitchAttempts];
    size_t tempSizes[MaxLastDitchAttempts];
    int attempt = 0;
    for (; attempt < MaxLastDitchAttempts; ++attempt) {
        getNewChunk(&p, tempMaps + attempt, tempSizes + attempt, size, alignment);
        if (OffsetFromAligned(p, alignment) == 0) {
            if (tempMaps[attempt])
                unmapPages(tempMaps[attempt], tempSizes[attempt]);
            break;
        }
        if (!tempMaps[attempt]) {
            
            tempMaps[attempt] = p;
            tempSizes[attempt] = size;
            p = nullptr;
        }
    }
    if (OffsetFromAligned(p, alignment)) {
        unmapPages(p, size);
        p = nullptr;
    }
    while (--attempt >= 0)
        unmapPages(tempMaps[attempt], tempSizes[attempt]);
    return p;
}







void
SystemPageAllocator::getNewChunk(void **aAddress, void **aRetainedAddr, size_t *aRetainedSize,
                                 size_t size, size_t alignment)
{
    void *address = *aAddress;
    void *retainedAddr = nullptr;
    size_t retainedSize = 0;
    do {
        bool addrsGrowDown = growthDirection <= 0;
        
        if (getNewChunkInner(&address, &retainedAddr, &retainedSize, size,
                             alignment, addrsGrowDown)) {
            break;
        }
        
        if (getNewChunkInner(&address, &retainedAddr, &retainedSize, size,
                             alignment, !addrsGrowDown)) {
            break;
        }
        
    } while (retainedAddr);
    *aAddress = address;
    *aRetainedAddr = retainedAddr;
    *aRetainedSize = retainedSize;
}

#define SET_OUT_PARAMS_AND_RETURN(address_, retainedAddr_, retainedSize_, toReturn_)\
    do {                                                                            \
        *aAddress = address_; *aRetainedAddr = retainedAddr_;                       \
        *aRetainedSize = retainedSize_; return toReturn_;                           \
    } while(false)

bool
SystemPageAllocator::getNewChunkInner(void **aAddress, void **aRetainedAddr, size_t *aRetainedSize,
                                      size_t size, size_t alignment, bool addrsGrowDown)
{
    void *initial = *aAddress;
    if (!initial)
        initial = MapMemory(size);
    if (OffsetFromAligned(initial, alignment) == 0)
        SET_OUT_PARAMS_AND_RETURN(initial, nullptr, 0, true);
    
    size_t offset;
    void *discardedAddr;
    void *retainedAddr;
    int delta;
    if (addrsGrowDown) {
        offset = OffsetFromAligned(initial, alignment);
        discardedAddr = initial;
        retainedAddr = (void *)(uintptr_t(initial) + size - offset);
        delta = -1;
    } else {
        offset = alignment - OffsetFromAligned(initial, alignment);
        discardedAddr = (void*)(uintptr_t(initial) + offset);
        retainedAddr = initial;
        delta = 1;
    }
    
    unmapPages(discardedAddr, size - offset);
    void *address = MapMemory(size);
    if (!address) {
        
        address = MapMemoryAt(initial, size - offset);
        if (!address)
            unmapPages(retainedAddr, offset);
        SET_OUT_PARAMS_AND_RETURN(address, nullptr, 0, false);
    }
    if ((addrsGrowDown && address < retainedAddr) || (!addrsGrowDown && address > retainedAddr)) {
        growthDirection += delta;
        SET_OUT_PARAMS_AND_RETURN(address, retainedAddr, offset, true);
    }
    
    growthDirection -= delta;
    
    if (OffsetFromAligned(address, alignment) == 0 && growthDirection + delta != 0)
        SET_OUT_PARAMS_AND_RETURN(address, retainedAddr, offset, true);
    unmapPages(address, size);
    
    address = MapMemoryAt(initial, size - offset);
    if (!address) {
        
        unmapPages(retainedAddr, offset);
        SET_OUT_PARAMS_AND_RETURN(nullptr, retainedAddr, 0, false);
    }
    SET_OUT_PARAMS_AND_RETURN(address, nullptr, 0, false);
}

#undef SET_OUT_PARAMS_AND_RETURN

void
SystemPageAllocator::unmapPages(void *p, size_t size)
{
    if (munmap(p, size))
        MOZ_ASSERT(errno == ENOMEM);
}

bool
SystemPageAllocator::markPagesUnused(void *p, size_t size)
{
    if (!decommitEnabled())
        return false;

    MOZ_ASSERT(OffsetFromAligned(p, pageSize) == 0);
    int result = madvise(p, size, MADV_DONTNEED);
    return result != -1;
}

bool
SystemPageAllocator::markPagesInUse(void *p, size_t size)
{
    MOZ_ASSERT(OffsetFromAligned(p, pageSize) == 0);
    return true;
}

size_t
SystemPageAllocator::GetPageFaultCount()
{
    struct rusage usage;
    int err = getrusage(RUSAGE_SELF, &usage);
    if (err)
        return 0;
    return usage.ru_majflt;
}

void *
SystemPageAllocator::AllocateMappedContent(int fd, size_t offset, size_t length, size_t alignment)
{
#define NEED_PAGE_ALIGNED 0
    size_t pa_start; 
    size_t pa_end; 
    size_t pa_size; 
    size_t page_size = sysconf(_SC_PAGESIZE); 
    struct stat st;
    uint8_t *buf;

    
    if (fstat(fd, &st) < 0 || offset >= (size_t) st.st_size ||
        length == 0 || length > (size_t) st.st_size - offset)
        return nullptr;

    
#if NEED_PAGE_ALIGNED
    alignment = std::max(alignment, page_size);
#endif
    if (offset & (alignment - 1))
        return nullptr;

    
    pa_start = offset & ~(page_size - 1);
    
    
    pa_end = ((offset + length - 1) & ~(page_size - 1)) + page_size;
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
SystemPageAllocator::DeallocateMappedContent(void *p, size_t length)
{
    void *pa_start; 
    size_t page_size = sysconf(_SC_PAGESIZE); 
    size_t total_size; 

    pa_start = (void *)(uintptr_t(p) & ~(page_size - 1));
    total_size = ((uintptr_t(p) + length) & ~(page_size - 1)) + page_size - uintptr_t(pa_start);
    if (munmap(pa_start, total_size))
        MOZ_ASSERT(errno == ENOMEM);
}

#else
#error "Memory mapping functions are not defined for your OS."
#endif

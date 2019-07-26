





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

    if (uintptr_t(p) % alignment != 0) {
        unmapPages(p, size);
        p = mapAlignedPagesSlow(size, alignment);
    }

    MOZ_ASSERT(uintptr_t(p) % alignment == 0);
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

    MOZ_ASSERT(uintptr_t(p) % pageSize == 0);
    LPVOID p2 = MapMemoryAt(p, size, MEM_RESET);
    return p2 == p;
}

bool
SystemPageAllocator::markPagesInUse(void *p, size_t size)
{
    MOZ_ASSERT(uintptr_t(p) % pageSize == 0);
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
    MOZ_ASSERT(uintptr_t(p) % pageSize == 0);
    return true;
}

bool
SystemPageAllocator::markPagesInUse(void *p, size_t size)
{
    MOZ_ASSERT(uintptr_t(p) % pageSize == 0);
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

    if (uintptr_t(p) % alignment != 0) {
        unmapPages(p, size);
        p = mapAlignedPagesSlow(size, alignment);
    }

    MOZ_ASSERT(uintptr_t(p) % alignment == 0);
    return p;
}

void *
SystemPageAllocator::mapAlignedPagesSlow(size_t size, size_t alignment)
{
    
    size_t reqSize = Min(size + 2 * alignment, 2 * size);
    void *region = MapMemory(reqSize);
    if (!region)
        return nullptr;

    uintptr_t regionEnd = uintptr_t(region) + reqSize;
    uintptr_t offset = uintptr_t(region) % alignment;
    MOZ_ASSERT(offset < reqSize - size);

    void *front = (void *)AlignBytes(uintptr_t(region), alignment);
    void *end = (void *)(uintptr_t(front) + size);
    if (front != region)
        unmapPages(region, alignment - offset);
    if (uintptr_t(end) != regionEnd)
        unmapPages(end, regionEnd - uintptr_t(end));

    return front;
}

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

    MOZ_ASSERT(uintptr_t(p) % pageSize == 0);
    int result = madvise(p, size, MADV_DONTNEED);
    return result != -1;
}

bool
SystemPageAllocator::markPagesInUse(void *p, size_t size)
{
    MOZ_ASSERT(uintptr_t(p) % pageSize == 0);
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

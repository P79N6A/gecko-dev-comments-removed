





#include "gc/Memory.h"

#include "js/HeapAPI.h"
#include "vm/Runtime.h"

using namespace js;
using namespace js::gc;

static bool
DecommitEnabled(JSRuntime *rt)
{
    return rt->gcSystemPageSize == ArenaSize;
}

#if defined(XP_WIN)
#include "jswin.h"
#include <psapi.h>

void
gc::InitMemorySubsystem(JSRuntime *rt)
{
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    rt->gcSystemPageSize = sysinfo.dwPageSize;
    rt->gcSystemAllocGranularity = sysinfo.dwAllocationGranularity;
}

static void *
VirtualAllocAligned(JSRuntime *rt, size_t size, size_t alignment, int type)
{
    JS_ASSERT(size >= alignment);
    JS_ASSERT(size % alignment == 0);
    JS_ASSERT(size % rt->gcSystemPageSize == 0);
    JS_ASSERT(alignment % rt->gcSystemAllocGranularity == 0);

    
    if (alignment == rt->gcSystemAllocGranularity) {
        return VirtualAlloc(nullptr, size, type, PAGE_READWRITE);
    }

    





    void *p = nullptr;
    while (!p) {
        







        size_t reserveSize = size + alignment - rt->gcSystemPageSize;
        p = VirtualAlloc(nullptr, reserveSize, MEM_RESERVE, PAGE_READWRITE);
        if (!p)
            return nullptr;
        void *chunkStart = (void *)AlignBytes(uintptr_t(p), alignment);
        UnmapPages(rt, p, reserveSize);
        p = VirtualAlloc(chunkStart, size, type, PAGE_READWRITE);

        
    }

    JS_ASSERT(uintptr_t(p) % alignment == 0);
    return p;
}

void *
gc::AllocateAlignedAddressRange(JSRuntime *rt, size_t size, size_t alignment)
{
    return VirtualAllocAligned(rt, size, alignment, MEM_RESERVE);
}

bool
gc::CommitAddressRange(void *addr, size_t size)
{
    void *start = VirtualAlloc(addr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    return addr == start;
}

void *
gc::MapAlignedPages(JSRuntime *rt, size_t size, size_t alignment)
{
    return VirtualAllocAligned(rt, size, alignment, MEM_COMMIT | MEM_RESERVE);
}

void
gc::UnmapPages(JSRuntime *rt, void *p, size_t size)
{
    JS_ALWAYS_TRUE(VirtualFree(p, 0, MEM_RELEASE));
}

bool
gc::MarkPagesUnused(JSRuntime *rt, void *p, size_t size)
{
    if (!DecommitEnabled(rt))
        return true;

    JS_ASSERT(uintptr_t(p) % rt->gcSystemPageSize == 0);
    LPVOID p2 = VirtualAlloc(p, size, MEM_RESET, PAGE_READWRITE);
    return p2 == p;
}

bool
gc::MarkPagesInUse(JSRuntime *rt, void *p, size_t size)
{
    JS_ASSERT(uintptr_t(p) % rt->gcSystemPageSize == 0);
    return true;
}

size_t
gc::GetPageFaultCount()
{
    PROCESS_MEMORY_COUNTERS pmc;
    if (!GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
        return 0;
    return pmc.PageFaultCount;
}

#elif defined(SOLARIS)

#include <sys/mman.h>
#include <unistd.h>

#ifndef MAP_NOSYNC
# define MAP_NOSYNC 0
#endif

void
gc::InitMemorySubsystem(JSRuntime *rt)
{
    rt->gcSystemPageSize = rt->gcSystemAllocGranularity = size_t(sysconf(_SC_PAGESIZE));
}

void *
gc::MapAlignedPages(JSRuntime *rt, size_t size, size_t alignment)
{
    JS_ASSERT(size >= alignment);
    JS_ASSERT(size % alignment == 0);
    JS_ASSERT(size % rt->gcSystemPageSize == 0);
    JS_ASSERT(alignment % rt->gcSystemAllocGranularity == 0);

    int prot = PROT_READ | PROT_WRITE;
    int flags = MAP_PRIVATE | MAP_ANON | MAP_ALIGN | MAP_NOSYNC;

    void *p = mmap((caddr_t)alignment, size, prot, flags, -1, 0);
    if (p == MAP_FAILED)
        return nullptr;
    return p;
}

void
gc::UnmapPages(JSRuntime *rt, void *p, size_t size)
{
    JS_ALWAYS_TRUE(0 == munmap((caddr_t)p, size));
}

bool
gc::MarkPagesUnused(JSRuntime *rt, void *p, size_t size)
{
    JS_ASSERT(uintptr_t(p) % rt->gcSystemPageSize == 0);
    return true;
}

bool
gc::MarkPagesInUse(JSRuntime *rt, void *p, size_t size)
{
    JS_ASSERT(uintptr_t(p) % rt->gcSystemPageSize == 0);
    return true;
}

size_t
gc::GetPageFaultCount()
{
    return 0;
}

#elif defined(XP_UNIX)

#include <sys/mman.h>
#include <sys/resource.h>
#include <unistd.h>

void
gc::InitMemorySubsystem(JSRuntime *rt)
{
    rt->gcSystemPageSize = rt->gcSystemAllocGranularity = size_t(sysconf(_SC_PAGESIZE));
}

static inline void *
MapMemory(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
#if defined(__ia64__)
    












    if (!addr)
        addr = (void*)0x0000070000000000;
    void *region = mmap(addr, length, prot, flags, fd, offset);
    if (region == MAP_FAILED)
        return MAP_FAILED;
    



    if ((uintptr_t(region) + (length - 1)) & 0xffff800000000000) {
        JS_ALWAYS_TRUE(0 == munmap(region, length));
        return MAP_FAILED;
    }
    return region;
#else
    return mmap(addr, length, prot, flags, fd, offset);
#endif
}

static void *
MmapAligned(JSRuntime *rt, size_t size, size_t alignment, int prot, int flags)
{
    JS_ASSERT(size >= alignment);
    JS_ASSERT(size % alignment == 0);
    JS_ASSERT(size % rt->gcSystemPageSize == 0);
    JS_ASSERT(alignment % rt->gcSystemAllocGranularity == 0);

    
    if (alignment == rt->gcSystemAllocGranularity) {
        void *region = MapMemory(nullptr, size, prot, flags, -1, 0);
        if (region == MAP_FAILED)
            return nullptr;
        return region;
    }

    
    size_t reqSize = Min(size + 2 * alignment, 2 * size);
    void *region = MapMemory(nullptr, reqSize, prot, flags, -1, 0);
    if (region == MAP_FAILED)
        return nullptr;

    uintptr_t regionEnd = uintptr_t(region) + reqSize;
    uintptr_t offset = uintptr_t(region) % alignment;
    JS_ASSERT(offset < reqSize - size);

    void *front = (void *)AlignBytes(uintptr_t(region), alignment);
    void *end = (void *)(uintptr_t(front) + size);
    if (front != region)
        JS_ALWAYS_TRUE(0 == munmap(region, alignment - offset));
    if (uintptr_t(end) != regionEnd)
        JS_ALWAYS_TRUE(0 == munmap(end, regionEnd - uintptr_t(end)));

    JS_ASSERT(uintptr_t(front) % alignment == 0);
    return front;
}

void *
gc::AllocateAlignedAddressRange(JSRuntime *rt, size_t size, size_t alignment)
{
    int prot = PROT_NONE;
    int flags = MAP_PRIVATE | MAP_ANON | MAP_NORESERVE;
    return MmapAligned(rt, size, alignment, prot, flags);
}

bool
gc::CommitAddressRange(void *addr, size_t size)
{
    int prot = PROT_READ | PROT_WRITE;
    int flags = MAP_PRIVATE | MAP_ANON | MAP_FIXED;
    void *start = MapMemory(addr, size, prot, flags, -1, 0);
    return addr == start;
}

void *
gc::MapAlignedPages(JSRuntime *rt, size_t size, size_t alignment)
{
    int prot = PROT_READ | PROT_WRITE;
    int flags = MAP_PRIVATE | MAP_ANON;
    return MmapAligned(rt, size, alignment, prot, flags);
}

void
gc::UnmapPages(JSRuntime *rt, void *p, size_t size)
{
    JS_ALWAYS_TRUE(0 == munmap(p, size));
}

bool
gc::MarkPagesUnused(JSRuntime *rt, void *p, size_t size)
{
    if (!DecommitEnabled(rt))
        return false;

    JS_ASSERT(uintptr_t(p) % rt->gcSystemPageSize == 0);
    int result = madvise(p, size, MADV_DONTNEED);
    return result != -1;
}

bool
gc::MarkPagesInUse(JSRuntime *rt, void *p, size_t size)
{
    JS_ASSERT(uintptr_t(p) % rt->gcSystemPageSize == 0);
    return true;
}

size_t
gc::GetPageFaultCount()
{
    struct rusage usage;
    int err = getrusage(RUSAGE_SELF, &usage);
    if (err)
        return 0;
    return usage.ru_majflt;
}

#else
#error "Memory mapping functions are not defined for your OS."
#endif

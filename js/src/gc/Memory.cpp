





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

void *
gc::MapAlignedPages(JSRuntime *rt, size_t size, size_t alignment)
{
    JS_ASSERT(size >= alignment);
    JS_ASSERT(size % alignment == 0);
    JS_ASSERT(size % rt->gcSystemPageSize == 0);
    JS_ASSERT(alignment % rt->gcSystemAllocGranularity == 0);

    
    if (alignment == rt->gcSystemAllocGranularity) {
        return VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    }

    





    void *p = nullptr;
    while (!p) {
        







        p = VirtualAlloc(nullptr, size * 2, MEM_RESERVE, PAGE_READWRITE);
        if (!p)
            return nullptr;
        void *chunkStart = (void *)(uintptr_t(p) + (alignment - (uintptr_t(p) % alignment)));
        UnmapPages(rt, p, size * 2);
        p = VirtualAlloc(chunkStart, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

        
    }

    JS_ASSERT(uintptr_t(p) % alignment == 0);
    return p;
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

#elif defined(XP_OS2)

#define INCL_DOSMEMMGR
#include <os2.h>

#define JS_GC_HAS_MAP_ALIGN 1
#define OS2_MAX_RECURSIONS  16

void
gc::InitMemorySubsystem(JSRuntime *rt)
{
    rt->gcSystemPageSize = rt->gcSystemAllocGranularity = ArenaSize;
}

void
gc::UnmapPages(JSRuntime *rt, void *addr, size_t size)
{
    if (!DosFreeMem(addr))
        return;

    



    unsigned long cb = 2 * size;
    unsigned long flags;
    if (DosQueryMem(addr, &cb, &flags) || cb < size)
        return;

    uintptr_t base = reinterpret_cast<uintptr_t>(addr) - ((2 * size) - cb);
    DosFreeMem(reinterpret_cast<void*>(base));

    return;
}

static void *
MapAlignedPagesRecursively(JSRuntime *rt, size_t size, size_t alignment, int& recursions)
{
    if (++recursions >= OS2_MAX_RECURSIONS)
        return nullptr;

    void *tmp;
    if (DosAllocMem(&tmp, size,
                    OBJ_ANY | PAG_COMMIT | PAG_READ | PAG_WRITE)) {
        JS_ALWAYS_TRUE(DosAllocMem(&tmp, size,
                                   PAG_COMMIT | PAG_READ | PAG_WRITE) == 0);
    }
    size_t offset = reinterpret_cast<uintptr_t>(tmp) & (alignment - 1);
    if (!offset)
        return tmp;

    




    size_t filler = size + alignment - offset;
    unsigned long cb = filler;
    unsigned long flags = 0;
    unsigned long rc = DosQueryMem(&(static_cast<char*>(tmp))[size],
                                   &cb, &flags);
    if (!rc && (flags & PAG_FREE) && cb >= filler) {
        UnmapPages(rt, tmp, 0);
        if (DosAllocMem(&tmp, filler,
                        OBJ_ANY | PAG_COMMIT | PAG_READ | PAG_WRITE)) {
            JS_ALWAYS_TRUE(DosAllocMem(&tmp, filler,
                                       PAG_COMMIT | PAG_READ | PAG_WRITE) == 0);
        }
    }

    void *p = MapAlignedPagesRecursively(rt, size, alignment, recursions);
    UnmapPages(rt, tmp, 0);

    return p;
}

void *
gc::MapAlignedPages(JSRuntime *rt, size_t size, size_t alignment)
{
    JS_ASSERT(size >= alignment);
    JS_ASSERT(size % alignment == 0);
    JS_ASSERT(size % rt->gcSystemPageSize == 0);
    JS_ASSERT(alignment % rt->gcSystemAllocGranularity == 0);

    int recursions = -1;

    




    void *p = MapAlignedPagesRecursively(rt, size, alignment, recursions);
    if (p)
        return p;

    




    if (DosAllocMem(&p, 2 * size,
                    OBJ_ANY | PAG_COMMIT | PAG_READ | PAG_WRITE)) {
        JS_ALWAYS_TRUE(DosAllocMem(&p, 2 * size,
                                   PAG_COMMIT | PAG_READ | PAG_WRITE) == 0);
    }

    uintptr_t addr = reinterpret_cast<uintptr_t>(p);
    addr = (addr + (alignment - 1)) & ~(alignment - 1);

    return reinterpret_cast<void *>(addr);
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

#elif defined(XP_UNIX) || defined(XP_MACOSX) || defined(DARWIN)

#include <sys/mman.h>
#include <sys/resource.h>
#include <unistd.h>

void
gc::InitMemorySubsystem(JSRuntime *rt)
{
    rt->gcSystemPageSize = rt->gcSystemAllocGranularity = size_t(sysconf(_SC_PAGESIZE));
}

static inline void *
MapMemory(size_t length, int prot, int flags, int fd, off_t offset)
{
#if defined(__ia64__)
    












    void *region = mmap((void*)0x0000070000000000, length, prot, flags, fd, offset);
    if (region == MAP_FAILED)
        return MAP_FAILED;
    



    if ((uintptr_t(region) + (length - 1)) & 0xffff800000000000) {
        JS_ALWAYS_TRUE(0 == munmap(region, length));
        return MAP_FAILED;
    }
    return region;
#else
    return mmap(nullptr, length, prot, flags, fd, offset);
#endif
}

void *
gc::MapAlignedPages(JSRuntime *rt, size_t size, size_t alignment)
{
    JS_ASSERT(size >= alignment);
    JS_ASSERT(size % alignment == 0);
    JS_ASSERT(size % rt->gcSystemPageSize == 0);
    JS_ASSERT(alignment % rt->gcSystemAllocGranularity == 0);

    int prot = PROT_READ | PROT_WRITE;
    int flags = MAP_PRIVATE | MAP_ANON;

    
    if (alignment == rt->gcSystemAllocGranularity) {
        void *region = MapMemory(size, prot, flags, -1, 0);
        if (region == MAP_FAILED)
            return nullptr;
        return region;
    }

    
    size_t reqSize = Min(size + 2 * alignment, 2 * size);
    void *region = MapMemory(reqSize, prot, flags, -1, 0);
    if (region == MAP_FAILED)
        return nullptr;

    uintptr_t regionEnd = uintptr_t(region) + reqSize;
    uintptr_t offset = uintptr_t(region) % alignment;
    JS_ASSERT(offset < reqSize - size);

    void *front = (void *)(uintptr_t(region) + (alignment - offset));
    void *end = (void *)(uintptr_t(front) + size);
    if (front != region)
        JS_ALWAYS_TRUE(0 == munmap(region, alignment - offset));
    if (uintptr_t(end) != regionEnd)
        JS_ALWAYS_TRUE(0 == munmap(end, regionEnd - uintptr_t(end)));

    JS_ASSERT(uintptr_t(front) % alignment == 0);
    return front;
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

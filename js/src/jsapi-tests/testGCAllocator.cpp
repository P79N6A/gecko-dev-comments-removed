






#include "gc/Memory.h"
#include "jsapi-tests/tests.h"

#if defined(XP_WIN)
#include "jswin.h"
#include <psapi.h>
#elif defined(SOLARIS)

#elif defined(XP_UNIX)
#include <algorithm>
#include <errno.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#else
#error "Memory mapping functions are not defined for your OS."
#endif

BEGIN_TEST(testGCAllocator)
{
#if defined(XP_WIN)
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    const size_t PageSize = sysinfo.dwPageSize;
#elif defined(SOLARIS)
    return true;
#elif defined(XP_UNIX)
    const size_t PageSize = size_t(sysconf(_SC_PAGESIZE));
#else
    return true;
#endif
    if (addressesGrowUp())
        return testGCAllocatorUp(PageSize);
    return testGCAllocatorDown(PageSize);
}

static const size_t Chunk = 512 * 1024;
static const size_t Alignment = 2 * Chunk;
static const int MaxTempChunks = 4096;
static const size_t StagingSize = 16 * Chunk;

bool
addressesGrowUp()
{
    void *p1 = mapMemory(2 * Chunk);
    void *p2 = mapMemory(2 * Chunk);
    unmapPages(p1, 2 * Chunk);
    unmapPages(p2, 2 * Chunk);
    return p1 < p2;
}

size_t
offsetFromAligned(void *p)
{
    return uintptr_t(p) % Alignment;
}

enum AllocType {
   UseNormalAllocator,
   UseLastDitchAllocator
};

bool
testGCAllocatorUp(const size_t PageSize)
{
    const size_t UnalignedSize = StagingSize + Alignment - PageSize;
    void *chunkPool[MaxTempChunks];
    
    void *stagingArea = mapMemory(UnalignedSize);
    if (!stagingArea)
        return false;
    
    unmapPages(stagingArea, UnalignedSize);
    if (offsetFromAligned(stagingArea)) {
        const size_t Offset = offsetFromAligned(stagingArea);
        
        stagingArea = (void *)(uintptr_t(stagingArea) + (Alignment - Offset));
    }
    mapMemoryAt(stagingArea, StagingSize);
    
    int tempChunks;
    if (!fillSpaceBeforeStagingArea(tempChunks, stagingArea, chunkPool, false))
        return false;
    
    unmapPages(stagingArea, StagingSize);
    
    js::gc::SystemPageAllocator GCAlloc;
    
    CHECK(positionIsCorrect("xxooxxx---------", stagingArea, chunkPool, tempChunks, GCAlloc));
    
    CHECK(positionIsCorrect("x-ooxxx---------", stagingArea, chunkPool, tempChunks, GCAlloc));
    
    CHECK(positionIsCorrect("x--xooxxx-------", stagingArea, chunkPool, tempChunks, GCAlloc));
    
    CHECK(positionIsCorrect("x--xx--xoo--xxx-", stagingArea, chunkPool, tempChunks, GCAlloc));
    
    CHECK(positionIsCorrect("x--xx---x-oo--x-", stagingArea, chunkPool, tempChunks, GCAlloc));
    
    CHECK(positionIsCorrect("x--xx--xx-oox---", stagingArea, chunkPool, tempChunks, GCAlloc,
                            UseLastDitchAllocator));

    
    while (--tempChunks >= 0)
        unmapPages(chunkPool[tempChunks], 2 * Chunk);
    return true;
}

bool
testGCAllocatorDown(const size_t PageSize)
{
    const size_t UnalignedSize = StagingSize + Alignment - PageSize;
    void *chunkPool[MaxTempChunks];
    
    void *stagingArea = mapMemory(UnalignedSize);
    if (!stagingArea)
        return false;
    
    unmapPages(stagingArea, UnalignedSize);
    if (offsetFromAligned(stagingArea)) {
        void *stagingEnd = (void *)(uintptr_t(stagingArea) + UnalignedSize);
        const size_t Offset = offsetFromAligned(stagingEnd);
        
        stagingArea = (void *)(uintptr_t(stagingEnd) - Offset - StagingSize);
    }
    mapMemoryAt(stagingArea, StagingSize);
    
    int tempChunks;
    if (!fillSpaceBeforeStagingArea(tempChunks, stagingArea, chunkPool, true))
        return false;
    
    unmapPages(stagingArea, StagingSize);
    
    js::gc::SystemPageAllocator GCAlloc;
    
    CHECK(positionIsCorrect("---------xxxooxx", stagingArea, chunkPool, tempChunks, GCAlloc));
    
    CHECK(positionIsCorrect("---------xxxoo-x", stagingArea, chunkPool, tempChunks, GCAlloc));
    
    CHECK(positionIsCorrect("-------xxxoox--x", stagingArea, chunkPool, tempChunks, GCAlloc));
    
    CHECK(positionIsCorrect("-xxx--oox--xx--x", stagingArea, chunkPool, tempChunks, GCAlloc));
    
    CHECK(positionIsCorrect("-x--oo-x---xx--x", stagingArea, chunkPool, tempChunks, GCAlloc));
    
    CHECK(positionIsCorrect("---xoo-xx--xx--x", stagingArea, chunkPool, tempChunks, GCAlloc,
                            UseLastDitchAllocator));

    
    while (--tempChunks >= 0)
        unmapPages(chunkPool[tempChunks], 2 * Chunk);
    return true;
}

bool
fillSpaceBeforeStagingArea(int &tempChunks, void *stagingArea,
                           void **chunkPool, bool addressesGrowDown)
{
    
    tempChunks = 0;
    chunkPool[tempChunks++] = mapMemory(2 * Chunk);
    while (tempChunks < MaxTempChunks && chunkPool[tempChunks - 1] &&
           (chunkPool[tempChunks - 1] < stagingArea) ^ addressesGrowDown) {
        chunkPool[tempChunks++] = mapMemory(2 * Chunk);
        if (!chunkPool[tempChunks - 1])
            break; 
        if ((chunkPool[tempChunks - 1] < chunkPool[tempChunks - 2]) ^ addressesGrowDown)
            break; 
    }
    
    if (!chunkPool[tempChunks - 1]) {
        --tempChunks;
        return true;
    }
    
    if ((chunkPool[tempChunks - 1] < stagingArea) ^ addressesGrowDown || (tempChunks > 1 &&
            (chunkPool[tempChunks - 1] < chunkPool[tempChunks - 2]) ^ addressesGrowDown))
    {
        while (--tempChunks >= 0)
            unmapPages(chunkPool[tempChunks], 2 * Chunk);
        unmapPages(stagingArea, StagingSize);
        return false;
    }
    return true;
}

bool
positionIsCorrect(const char *str, void *base, void **chunkPool, int tempChunks,
                  js::gc::SystemPageAllocator& GCAlloc, AllocType allocator = UseNormalAllocator)
{
    
    
    
    
    
    
    
    int len = strlen(str);
    int i;
    
    for (i = 0; i < len && str[i] != 'o'; ++i);
    void *desired = (void *)(uintptr_t(base) + i * Chunk);
    
    for (i = 0; i < len; ++i) {
        if (str[i] == 'x')
            mapMemoryAt((void *)(uintptr_t(base) +  i * Chunk), Chunk);
    }
    
    void *result;
    if (allocator == UseNormalAllocator)
        result = GCAlloc.mapAlignedPages(2 * Chunk, Alignment);
    else
        result = GCAlloc.testMapAlignedPagesLastDitch(2 * Chunk, Alignment);
    
    if (result)
        GCAlloc.unmapPages(result, 2 * Chunk);
    for (--i; i >= 0; --i) {
        if (str[i] == 'x')
            unmapPages((void *)(uintptr_t(base) +  i * Chunk), Chunk);
    }
    
    if (result != desired) {
        while (--tempChunks >= 0)
            unmapPages(chunkPool[tempChunks], 2 * Chunk);
    }
    return result == desired;
}

#if defined(XP_WIN)

void *
mapMemoryAt(void *desired, size_t length)
{
    return VirtualAlloc(desired, length, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

void *
mapMemory(size_t length)
{
    return VirtualAlloc(nullptr, length, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

void
unmapPages(void *p, size_t size)
{
    MOZ_ALWAYS_TRUE(VirtualFree(p, 0, MEM_RELEASE));
}

#elif defined(SOLARIS)

#elif defined(XP_UNIX)

void *
mapMemoryAt(void *desired, size_t length)
{
#if defined(__ia64__)
    MOZ_ASSERT(0xffff800000000000ULL & (uintptr_t(desired) + length - 1) == 0);
#endif
    void *region = mmap(desired, length, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (region == MAP_FAILED)
        return nullptr;
    if (region != desired) {
        if (munmap(region, length))
            MOZ_ASSERT(errno == ENOMEM);
        return nullptr;
    }
    return region;
}

void *
mapMemory(size_t length)
{
    void *hint = nullptr;
#if defined(__ia64__)
    hint = (void*)0x0000070000000000ULL;
#endif
    void *region = mmap(hint, length, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (region == MAP_FAILED)
        return nullptr;
#if defined(__ia64__)
    if ((uintptr_t(region) + (length - 1)) & 0xffff800000000000ULL) {
        if (munmap(region, length))
            MOZ_ASSERT(errno == ENOMEM);
        return nullptr;
    }
#endif
    return region;
}

void
unmapPages(void *p, size_t size)
{
    if (munmap(p, size))
        MOZ_ASSERT(errno == ENOMEM);
}

#else 
#error "Memory mapping functions are not defined for your OS."
#endif
END_TEST(testGCAllocator)


























#include "ExecutableAllocator.h"

#if ENABLE_ASSEMBLER && WTF_PLATFORM_UNIX && !WTF_PLATFORM_SYMBIAN

#include <sys/mman.h>
#include <unistd.h>

namespace JSC {

void ExecutableAllocator::intializePageSize()
{
    ExecutableAllocator::pageSize = getpagesize();
}

ExecutablePool::Allocation ExecutablePool::systemAlloc(size_t n)
{
    void* allocation = mmap(NULL, n, INITIAL_PROTECTION_FLAGS, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (allocation == MAP_FAILED)
        CRASH();
    ExecutablePool::Allocation alloc = { reinterpret_cast<char*>(allocation), n };
    return alloc;
}

void ExecutablePool::systemRelease(const ExecutablePool::Allocation& alloc)
{ 
    int result = munmap(alloc.pages, alloc.size);
    ASSERT_UNUSED(result, !result);
}

#if WTF_ENABLE_ASSEMBLER_WX_EXCLUSIVE
void ExecutableAllocator::reprotectRegion(void* start, size_t size, ProtectionSeting setting)
{
    if (!pageSize)
        intializePageSize();

    
    
    intptr_t startPtr = reinterpret_cast<intptr_t>(start);
    intptr_t pageStartPtr = startPtr & ~(pageSize - 1);
    void* pageStart = reinterpret_cast<void*>(pageStartPtr);
    size += (startPtr - pageStartPtr);

    
    size += (pageSize - 1);
    size &= ~(pageSize - 1);

    mprotect(pageStart, size, (setting == Writable) ? PROTECTION_FLAGS_RW : PROTECTION_FLAGS_RX);
}
#endif

}

#endif 

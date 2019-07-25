
























#include "ExecutableAllocator.h"

#if ENABLE_ASSEMBLER && WTF_PLATFORM_UNIX && !WTF_PLATFORM_SYMBIAN

#include <sys/mman.h>
#include <unistd.h>
#include <wtf/VMTags.h>

namespace JSC {

size_t ExecutableAllocator::determinePageSize()
{
    return getpagesize();
}

ExecutablePool::Allocation ExecutableAllocator::systemAlloc(size_t n)
{
    void* allocation = mmap(NULL, n, INITIAL_PROTECTION_FLAGS, MAP_PRIVATE | MAP_ANON, VM_TAG_FOR_EXECUTABLEALLOCATOR_MEMORY, 0);
    if (allocation == MAP_FAILED)
        allocation = NULL;
    ExecutablePool::Allocation alloc = { reinterpret_cast<char*>(allocation), n };
    return alloc;
}

void ExecutableAllocator::systemRelease(const ExecutablePool::Allocation& alloc)
{ 
    int result = munmap(alloc.pages, alloc.size);
    ASSERT_UNUSED(result, !result);
}

#if WTF_ENABLE_ASSEMBLER_WX_EXCLUSIVE
void ExecutableAllocator::reprotectRegion(void* start, size_t size, ProtectionSetting setting)
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

#if WTF_CPU_ARM_TRADITIONAL && WTF_PLATFORM_LINUX && WTF_COMPILER_RVCT
__asm void ExecutableAllocator::cacheFlush(void* code, size_t size)
{
    ARM
    push {r7}
    add r1, r1, r0
    mov r7, #0xf0000
    add r7, r7, #0x2
    mov r2, #0x0
    svc #0x0
    pop {r7}
    bx lr
}
#endif

}

#endif 

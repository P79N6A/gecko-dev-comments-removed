

























#include "ExecutableAllocator.h"

#if ENABLE_ASSEMBLER && WTF_OS_WINDOWS

#include "jswin.h"

namespace JSC {

size_t ExecutableAllocator::determinePageSize()
{
    SYSTEM_INFO system_info;
    GetSystemInfo(&system_info);
    return system_info.dwPageSize;
}

ExecutablePool::Allocation ExecutableAllocator::systemAlloc(size_t n)
{
    void *allocation = VirtualAlloc(0, n, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    ExecutablePool::Allocation alloc = {reinterpret_cast<char*>(allocation), n};
    return alloc;
}

void ExecutableAllocator::systemRelease(const ExecutablePool::Allocation& alloc)
{ 
    VirtualFree(alloc.pages, 0, MEM_RELEASE); 
}

#if ENABLE_ASSEMBLER_WX_EXCLUSIVE
#error "ASSEMBLER_WX_EXCLUSIVE not yet suported on this platform."
#endif

}

#endif 

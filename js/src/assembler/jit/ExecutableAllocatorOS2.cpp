

























#include "ExecutableAllocator.h"

#if ENABLE_ASSEMBLER && WTF_PLATFORM_OS2

#define INCL_DOS
#include <os2.h>

namespace JSC {

void ExecutableAllocator::intializePageSize()
{
    ExecutableAllocator::pageSize = 4096u;
}

ExecutablePool::Allocation ExecutablePool::systemAlloc(size_t n)
{
    void* allocation = NULL;
    if (DosAllocMem(&allocation, n, OBJ_ANY|PAG_COMMIT|PAG_READ|PAG_WRITE) &&
        DosAllocMem(&allocation, n, PAG_COMMIT|PAG_READ|PAG_WRITE))
        CRASH();
    ExecutablePool::Allocation alloc = {reinterpret_cast<char*>(allocation), n};
    return alloc;
}

void ExecutablePool::systemRelease(const ExecutablePool::Allocation& alloc)
{
    DosFreeMem(alloc.pages);
}

#if ENABLE_ASSEMBLER_WX_EXCLUSIVE
#error "ASSEMBLER_WX_EXCLUSIVE not yet suported on this platform."
#endif

}

#endif 

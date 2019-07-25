

























#include "ExecutableAllocator.h"

#if ENABLE_ASSEMBLER && WTF_OS_OS2

#define INCL_DOS
#include <os2.h>

namespace JSC {

size_t ExecutableAllocator::determinePageSize()
{
    return 4096u;
}

ExecutablePool::Allocation ExecutableAllocator::systemAlloc(size_t n)
{
    void* allocation = NULL;
    if (DosAllocMem(&allocation, n, OBJ_ANY|PAG_COMMIT|PAG_READ|PAG_WRITE) &&
        DosAllocMem(&allocation, n, PAG_COMMIT|PAG_READ|PAG_WRITE))
        CRASH();
    ExecutablePool::Allocation alloc = {reinterpret_cast<char*>(allocation), n};
    return alloc;
}

void ExecutableAllocator::systemRelease(const ExecutablePool::Allocation& alloc)
{
    DosFreeMem(alloc.pages);
}

#if ENABLE_ASSEMBLER_WX_EXCLUSIVE
#error "ASSEMBLER_WX_EXCLUSIVE not yet suported on this platform."
#endif

}

#endif 

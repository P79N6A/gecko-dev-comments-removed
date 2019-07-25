



















#include "config.h"

#include "ExecutableAllocator.h"

#if ENABLE_ASSEMBLER && WTF_PLATFORM_SYMBIAN

#include <e32hal.h>
#include <e32std.h>


const size_t MOVING_MEM_PAGE_SIZE = 256 * 1024; 

namespace JSC {

void ExecutableAllocator::intializePageSize()
{
#if WTF_CPU_ARMV5_OR_LOWER
    
    
    
    
    ExecutableAllocator::pageSize = MOVING_MEM_PAGE_SIZE;
#else
    TInt page_size;
    UserHal::PageSizeInBytes(page_size);
    ExecutableAllocator::pageSize = page_size;
#endif
}

ExecutablePool::Allocation ExecutablePool::systemAlloc(size_t n)
{
    RChunk* codeChunk = new RChunk();

    TInt errorCode = codeChunk->CreateLocalCode(n, n);

    char* allocation = reinterpret_cast<char*>(codeChunk->Base());
    if (!allocation)
        CRASH();
    ExecutablePool::Allocation alloc = { allocation, n, codeChunk };
    return alloc;
}

void ExecutablePool::systemRelease(const ExecutablePool::Allocation& alloc)
{ 
    alloc.chunk->Close();
    delete alloc.chunk;
}

#if ENABLE_ASSEMBLER_WX_EXCLUSIVE
#error "ASSEMBLER_WX_EXCLUSIVE not yet suported on this platform."
#endif

}

#endif 

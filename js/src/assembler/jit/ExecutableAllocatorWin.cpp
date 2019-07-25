
























#include "ExecutableAllocator.h"

#if ENABLE_ASSEMBLER && WTF_OS_WINDOWS

#include "jswin.h"
#include "prmjtime.h"

extern void random_setSeed(int64 *, int64);
extern uint64 random_next(int64 *, int);

namespace JSC {

int64 ExecutableAllocator::rngSeed;

void ExecutableAllocator::initSeed()
{
    random_setSeed(&rngSeed, (PRMJ_Now() / 1000) ^ int64(this));
}

size_t ExecutableAllocator::determinePageSize()
{
    SYSTEM_INFO system_info;
    GetSystemInfo(&system_info);
    return system_info.dwPageSize;
}

void *ExecutableAllocator::computeRandomAllocationAddress()
{
    










    static const uintN chunkBits = 16;
#if WTF_CPU_X86_64
    static const uintptr_t base = 0x0000000080000000;
    static const uintptr_t mask = 0x000003ffffff0000;
#elif WTF_CPU_X86
    static const uintptr_t base = 0x04000000;
    static const uintptr_t mask = 0x3fff0000;
#else
# error "Unsupported architecture"
#endif
    uint64 rand = random_next(&rngSeed, 32) << chunkBits;
    return (void *) (base | rand & mask);
}

ExecutablePool::Allocation ExecutableAllocator::systemAlloc(size_t n)
{
    void *allocation = NULL;
    if (allocBehavior == AllocationCanRandomize) {
        void *randomAddress = computeRandomAllocationAddress();
        allocation = VirtualAlloc(randomAddress, n, MEM_COMMIT | MEM_RESERVE,
                                  PAGE_EXECUTE_READWRITE);
    }
    if (!allocation)
        allocation = VirtualAlloc(0, n, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    ExecutablePool::Allocation alloc = { reinterpret_cast<char*>(allocation), n };
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

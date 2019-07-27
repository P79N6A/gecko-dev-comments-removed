
























#include "mozilla/WindowsVersion.h"

#include "jsmath.h"

#include "jit/ExecutableAllocator.h"

#include "jswin.h"

using namespace js::jit;

uint64_t ExecutableAllocator::rngSeed;

size_t ExecutableAllocator::determinePageSize()
{
    SYSTEM_INFO system_info;
    GetSystemInfo(&system_info);
    return system_info.dwPageSize;
}

void *ExecutableAllocator::computeRandomAllocationAddress()
{
    










    static const unsigned chunkBits = 16;
#ifdef JS_CPU_X64
    static const uintptr_t base = 0x0000000080000000;
    static const uintptr_t mask = 0x000003ffffff0000;
#elif defined(JS_CPU_X86)
    static const uintptr_t base = 0x04000000;
    static const uintptr_t mask = 0x3fff0000;
#else
# error "Unsupported architecture"
#endif
    uint64_t rand = random_next(&rngSeed, 32) << chunkBits;
    return (void *) (base | rand & mask);
}

static bool
RandomizeIsBrokenImpl()
{
    
    return !mozilla::IsVistaOrLater();
}

static bool
RandomizeIsBroken()
{
    
    
    static int result = RandomizeIsBrokenImpl();
    return !!result;
}

ExecutablePool::Allocation ExecutableAllocator::systemAlloc(size_t n)
{
    void *allocation = NULL;
    
    
#ifndef JS_CPU_X64
    if (!RandomizeIsBroken()) {
        void *randomAddress = computeRandomAllocationAddress();
        allocation = VirtualAlloc(randomAddress, n, MEM_COMMIT | MEM_RESERVE,
                                  PAGE_EXECUTE_READWRITE);
    }
#endif
    if (!allocation)
        allocation = VirtualAlloc(0, n, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    ExecutablePool::Allocation alloc = { reinterpret_cast<char*>(allocation), n };
    return alloc;
}

void ExecutableAllocator::systemRelease(const ExecutablePool::Allocation& alloc)
{
    VirtualFree(alloc.pages, 0, MEM_RELEASE);
}

void
ExecutablePool::toggleAllCodeAsAccessible(bool accessible)
{
    char* begin = m_allocation.pages;
    size_t size = m_freePtr - begin;

    if (size) {
        
        
        DWORD oldProtect;
        int flags = accessible ? PAGE_EXECUTE_READWRITE : PAGE_NOACCESS;
        if (!VirtualProtect(begin, size, flags, &oldProtect))
            MOZ_CRASH();
    }
}

#if ENABLE_ASSEMBLER_WX_EXCLUSIVE
#error "ASSEMBLER_WX_EXCLUSIVE not yet suported on this platform."
#endif

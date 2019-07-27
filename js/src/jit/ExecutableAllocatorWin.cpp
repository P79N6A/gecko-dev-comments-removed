
























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

void *
js::jit::AllocateExecutableMemory(void *addr, size_t bytes, unsigned permissions, const char *tag,
                                  size_t pageSize)
{
    MOZ_ASSERT(bytes % pageSize == 0);
    return VirtualAlloc(addr, bytes, MEM_COMMIT | MEM_RESERVE, permissions);
}

void
js::jit::DeallocateExecutableMemory(void *addr, size_t bytes, size_t pageSize)
{
    MOZ_ASSERT(bytes % pageSize == 0);
    VirtualFree(addr, 0, MEM_RELEASE);
}

ExecutablePool::Allocation ExecutableAllocator::systemAlloc(size_t n)
{
    void *allocation = NULL;
    
    
#ifndef JS_CPU_X64
    if (!RandomizeIsBroken()) {
        void *randomAddress = computeRandomAllocationAddress();
        allocation = AllocateExecutableMemory(randomAddress, n, PAGE_EXECUTE_READWRITE,
                                              "js-jit-code", pageSize);
    }
#endif
    if (!allocation) {
        allocation = AllocateExecutableMemory(nullptr, n, PAGE_EXECUTE_READWRITE,
                                              "js-jit-code", pageSize);
    }
    ExecutablePool::Allocation alloc = { reinterpret_cast<char*>(allocation), n };
    return alloc;
}

void ExecutableAllocator::systemRelease(const ExecutablePool::Allocation& alloc)
{
    DeallocateExecutableMemory(alloc.pages, alloc.size, pageSize);
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




























#include "mozilla/DebugOnly.h"
#include "mozilla/TaggedAnonymousMemory.h"

#include <errno.h>
#include <sys/mman.h>
#include <unistd.h>

#include "jit/ExecutableAllocator.h"
#include "js/Utility.h"

using namespace js::jit;

size_t
ExecutableAllocator::determinePageSize()
{
    return getpagesize();
}

void*
js::jit::AllocateExecutableMemory(void* addr, size_t bytes, unsigned permissions, const char* tag,
                                  size_t pageSize)
{
    MOZ_ASSERT(bytes % pageSize == 0);
    void* p = MozTaggedAnonymousMmap(addr, bytes, permissions, MAP_PRIVATE | MAP_ANON, -1, 0, tag);
    return p == MAP_FAILED ? nullptr : p;
}

void
js::jit::DeallocateExecutableMemory(void* addr, size_t bytes, size_t pageSize)
{
    MOZ_ASSERT(bytes % pageSize == 0);
    mozilla::DebugOnly<int> result = munmap(addr, bytes);
    MOZ_ASSERT(!result || errno == ENOMEM);
}

ExecutablePool::Allocation
ExecutableAllocator::systemAlloc(size_t n)
{
    void* allocation = AllocateExecutableMemory(nullptr, n, initialProtectionFlags(Executable),
                                                "js-jit-code", pageSize);
    ExecutablePool::Allocation alloc = { reinterpret_cast<char*>(allocation), n };
    return alloc;
}

void
ExecutableAllocator::systemRelease(const ExecutablePool::Allocation& alloc)
{
    DeallocateExecutableMemory(alloc.pages, alloc.size, pageSize);
}

static const unsigned FLAGS_RW = PROT_READ | PROT_WRITE;
static const unsigned FLAGS_RX = PROT_READ | PROT_EXEC;

void
ExecutableAllocator::reprotectRegion(void* start, size_t size, ProtectionSetting setting)
{
    MOZ_ASSERT(nonWritableJitCode);
    MOZ_ASSERT(pageSize);

    
    
    intptr_t startPtr = reinterpret_cast<intptr_t>(start);
    intptr_t pageStartPtr = startPtr & ~(pageSize - 1);
    void* pageStart = reinterpret_cast<void*>(pageStartPtr);
    size += (startPtr - pageStartPtr);

    
    size += (pageSize - 1);
    size &= ~(pageSize - 1);

    mprotect(pageStart, size, (setting == Writable) ? FLAGS_RW : FLAGS_RX);
}

 unsigned
ExecutableAllocator::initialProtectionFlags(ProtectionSetting protection)
{
    if (!nonWritableJitCode)
        return FLAGS_RW | FLAGS_RX;

    return (protection == Writable) ? FLAGS_RW : FLAGS_RX;
}

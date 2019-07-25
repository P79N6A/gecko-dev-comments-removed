




























#include "assembler/wtf/Platform.h"

#if ENABLE_ASSEMBLER && WTF_OS_UNIX && !WTF_OS_SYMBIAN

#include "OSAllocator.h"

#include <errno.h>
#include <sys/mman.h>
#include "wtf/Assertions.h"

namespace WTF {

void* OSAllocator::reserveUncommitted(size_t bytes, Usage usage, bool writable, bool executable)
{
    void* result = reserveAndCommit(bytes, usage, writable, executable);
#if HAVE_MADV_FREE_REUSE
    
    while (madvise(result, bytes, MADV_FREE_REUSABLE) == -1 && errno == EAGAIN) { }
#endif
    return result;
}

void* OSAllocator::reserveAndCommit(size_t bytes, Usage usage, bool writable, bool executable)
{
    
    int protection = PROT_READ;
    if (writable)
        protection |= PROT_WRITE;
    if (executable)
        protection |= PROT_EXEC;

    int flags = MAP_PRIVATE | MAP_ANON;

#if WTF_OS_DARWIN && !defined(BUILDING_ON_TIGER)
    int fd = usage;
#else
    int fd = -1;
#endif

    void* result = 0;
#if (WTF_OS_DARWIN && WTF_CPU_X86_64)
    if (executable) {
        
        
        
        
        
        
        
        
        
        intptr_t randomLocation = 0;
        randomLocation = arc4random() & ((1 << 25) - 1);
        randomLocation += (1 << 24);
        randomLocation <<= 21;
        result = reinterpret_cast<void*>(randomLocation);
    }
#endif

    result = mmap(result, bytes, protection, flags, fd, 0);
    if (result == MAP_FAILED)
        CRASH();
    return result;
}

void OSAllocator::commit(void* address, size_t bytes, bool, bool)
{
#if HAVE_MADV_FREE_REUSE
    while (madvise(address, bytes, MADV_FREE_REUSE) == -1 && errno == EAGAIN) { }
#else
    
    UNUSED_PARAM(address);
    UNUSED_PARAM(bytes);
#endif
}

void OSAllocator::decommit(void* address, size_t bytes)
{
#if HAVE_MADV_FREE_REUSE
    while (madvise(address, bytes, MADV_FREE_REUSABLE) == -1 && errno == EAGAIN) { }
#elif HAVE_MADV_FREE
    while (madvise(address, bytes, MADV_FREE) == -1 && errno == EAGAIN) { }
#elif HAVE_MADV_DONTNEED
    while (madvise(address, bytes, MADV_DONTNEED) == -1 && errno == EAGAIN) { }
#else
    UNUSED_PARAM(address);
    UNUSED_PARAM(bytes);
#endif
}

void OSAllocator::releaseDecommitted(void* address, size_t bytes)
{
    int result = munmap(address, bytes);
    if (result == -1)
        CRASH();
}

} 

#endif

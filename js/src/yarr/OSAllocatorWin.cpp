




























#include "assembler/wtf/Platform.h"

#if ENABLE_ASSEMBLER && WTF_OS_WINDOWS

#include "windows.h"
#include "wtf/Assertions.h"

#include "OSAllocator.h"

namespace WTF {

static inline DWORD protection(bool writable, bool executable)
{
    return executable ?
        (writable ? PAGE_EXECUTE_READWRITE : PAGE_EXECUTE_READ) :
        (writable ? PAGE_READWRITE : PAGE_READONLY);
}

void* OSAllocator::reserveUncommitted(size_t bytes, Usage, bool writable, bool executable)
{
    void* result = VirtualAlloc(0, bytes, MEM_RESERVE, protection(writable, executable));
    if (!result)
        CRASH();
    return result;
}

void* OSAllocator::reserveAndCommit(size_t bytes, Usage, bool writable, bool executable)
{
    void* result = VirtualAlloc(0, bytes, MEM_RESERVE | MEM_COMMIT, protection(writable, executable));
    if (!result)
        CRASH();
    return result;
}

void OSAllocator::commit(void* address, size_t bytes, bool writable, bool executable)
{
    void* result = VirtualAlloc(address, bytes, MEM_COMMIT, protection(writable, executable));
    if (!result)
        CRASH();
}

void OSAllocator::decommit(void* address, size_t bytes)
{
    bool result = VirtualFree(address, bytes, MEM_DECOMMIT);
    if (!result)
        CRASH();
}

void OSAllocator::releaseDecommitted(void* address, size_t bytes)
{
    
    
    bool result = VirtualFree(address, 0, MEM_RELEASE);
    if (!result)
        CRASH();
}

} 

#endif

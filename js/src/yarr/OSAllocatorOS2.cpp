




























#include "assembler/wtf/Platform.h"

#if ENABLE_ASSEMBLER && WTF_OS_OS2

#define INCL_DOS
#include <os2.h>

#include "assembler/wtf/Assertions.h"

#include "yarr/OSAllocator.h"

namespace WTF {

static inline ULONG protection(bool writable, bool executable)
{
    return (PAG_READ | (writable ? PAG_WRITE : 0) | (executable ? PAG_EXECUTE : 0));
}

void* OSAllocator::reserveUncommitted(size_t bytes, Usage, bool writable, bool executable)
{
    void* result = NULL;
    if (DosAllocMem(&result, bytes, OBJ_ANY | protection(writable, executable)) &&
        DosAllocMem(&result, bytes, protection(writable, executable)))
    {   CRASH();
    }
    return result;
}

void* OSAllocator::reserveAndCommit(size_t bytes, Usage, bool writable, bool executable)
{
    void* result = NULL;
    if (DosAllocMem(&result, bytes, OBJ_ANY | PAG_COMMIT | protection(writable, executable)) &&
        DosAllocMem(&result, bytes, PAG_COMMIT | protection(writable, executable)))
    {   CRASH();
    }
    return result;

}

void OSAllocator::commit(void* address, size_t bytes, bool writable, bool executable)
{
if (DosSetMem(address, bytes, PAG_COMMIT | protection(writable, executable)))
    CRASH();
}

void OSAllocator::decommit(void* address, size_t bytes)
{
if (DosSetMem(address, bytes, PAG_DECOMMIT))
    CRASH();
}

void OSAllocator::releaseDecommitted(void* address, size_t bytes)
{
if (DosFreeMem(address))
    CRASH();
}

} 

#endif

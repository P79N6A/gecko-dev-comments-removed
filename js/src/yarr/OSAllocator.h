




























#ifndef OSAllocator_h
#define OSAllocator_h

#include <stdlib.h>
#include "wtfbridge.h"
#include "assembler/wtf/VMTags.h"
#include "assembler/wtf/Assertions.h"

namespace WTF {

class OSAllocator {
public:
    enum Usage {
        UnknownUsage = -1,
        FastMallocPages = VM_TAG_FOR_TCMALLOC_MEMORY,
        JSGCHeapPages = VM_TAG_FOR_COLLECTOR_MEMORY,
        JSVMStackPages = VM_TAG_FOR_REGISTERFILE_MEMORY,
        JSJITCodePages = VM_TAG_FOR_EXECUTABLEALLOCATOR_MEMORY
    };

    
    
    
    static void* reserveUncommitted(size_t, Usage = UnknownUsage, bool writable = true, bool executable = false);
    static void releaseDecommitted(void*, size_t);

    
    
    
    static void commit(void*, size_t, bool writable, bool executable);
    static void decommit(void*, size_t);

    
    
    
    static void* reserveAndCommit(size_t, Usage = UnknownUsage, bool writable = true, bool executable = false);
    static void decommitAndRelease(void* base, size_t size);

    
    
    
    static void* reserveAndCommit(size_t reserveSize, size_t commitSize, Usage = UnknownUsage, bool writable = true, bool executable = false);
    static void decommitAndRelease(void* releaseBase, size_t releaseSize, void* decommitBase, size_t decommitSize);
};

inline void* OSAllocator::reserveAndCommit(size_t reserveSize, size_t commitSize, Usage usage, bool writable, bool executable)
{
    void* base = reserveUncommitted(reserveSize, usage, writable, executable);
    commit(base, commitSize, writable, executable);
    return base;
}

inline void OSAllocator::decommitAndRelease(void* releaseBase, size_t releaseSize, void* decommitBase, size_t decommitSize)
{
    ASSERT(decommitBase >= releaseBase && (static_cast<char*>(decommitBase) + decommitSize) <= (static_cast<char*>(releaseBase) + releaseSize));
#if WTF_OS_WINCE || WTF_OS_SYMBIAN
    
    
    
    decommit(decommitBase, decommitSize);
#endif
    releaseDecommitted(releaseBase, releaseSize);
}

inline void OSAllocator::decommitAndRelease(void* base, size_t size)
{
    decommitAndRelease(base, size, base, size);
}

} 

using WTF::OSAllocator;

#endif






























#ifndef PageAllocation_h
#define PageAllocation_h

#include "wtfbridge.h"
#include "OSAllocator.h"
#include "PageBlock.h"
#include "assembler/wtf/VMTags.h"

#if WTF_OS_DARWIN
#include <mach/mach_init.h>
#include <mach/vm_map.h>
#endif

#if WTF_OS_HAIKU
#include <OS.h>
#endif

#if WTF_OS_WINDOWS
#include <malloc.h>
#include <windows.h>
#endif

#if WTF_OS_SYMBIAN
#include <e32hal.h>
#include <e32std.h>
#endif

#if WTF_HAVE_ERRNO_H
#include <errno.h>
#endif

#if WTF_HAVE_MMAP
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace WTF {

















class PageAllocation : private PageBlock {
public:
    PageAllocation()
    {
    }

    using PageBlock::size;
    using PageBlock::base;

#ifndef __clang__
    using PageBlock::operator bool;
#else
    
    
    operator bool() const { return PageBlock::operator bool(); }
#endif

    static PageAllocation allocate(size_t size, OSAllocator::Usage usage = OSAllocator::UnknownUsage, bool writable = true, bool executable = false)
    {
        ASSERT(isPageAligned(size));
        return PageAllocation(OSAllocator::reserveAndCommit(size, usage, writable, executable), size);
    }

    void deallocate()
    {
        
        
        PageAllocation tmp;
        JSC::std::swap(tmp, *this);

        ASSERT(tmp);
        ASSERT(!*this);

        OSAllocator::decommitAndRelease(tmp.base(), tmp.size());
    }

private:
    PageAllocation(void* base, size_t size)
        : PageBlock(base, size)
    {
    }
};

} 

using WTF::PageAllocation;

#endif 

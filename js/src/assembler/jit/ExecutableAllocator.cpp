
























#include "ExecutableAllocator.h"

#if ENABLE_ASSEMBLER

#include "prmjtime.h"

namespace JSC {

size_t ExecutableAllocator::pageSize = 0;
size_t ExecutableAllocator::largeAllocSize = 0;

ExecutablePool::~ExecutablePool()
{
    m_allocator->releasePoolPages(this);
}

void
ExecutableAllocator::sizeOfCode(size_t *jaeger, size_t *ion, size_t *asmJS, size_t *regexp, size_t *unused) const
{
    *jaeger = 0;
    *ion    = 0;
    *asmJS  = 0;
    *regexp = 0;
    *unused = 0;

    if (m_pools.initialized()) {
        for (ExecPoolHashSet::Range r = m_pools.all(); !r.empty(); r.popFront()) {
            ExecutablePool* pool = r.front();
            *jaeger += pool->m_jaegerCodeBytes;
            *ion    += pool->m_ionCodeBytes;
            *asmJS  += pool->m_asmJSCodeBytes;
            *regexp += pool->m_regexpCodeBytes;
            *unused += pool->m_allocation.size - pool->m_jaegerCodeBytes - pool->m_ionCodeBytes
                                               - pool->m_asmJSCodeBytes - pool->m_regexpCodeBytes;
        }
    }
}

}

#endif 

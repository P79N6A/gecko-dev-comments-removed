


























#include "ExecutableAllocator.h"

#include "js/MemoryMetrics.h"

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
ExecutableAllocator::sizeOfCode(JS::CodeSizes *sizes) const
{
    *sizes = JS::CodeSizes();

    if (m_pools.initialized()) {
        for (ExecPoolHashSet::Range r = m_pools.all(); !r.empty(); r.popFront()) {
            ExecutablePool* pool = r.front();
            sizes->jaeger   += pool->m_jaegerCodeBytes;
            sizes->ion      += pool->m_ionCodeBytes;
            sizes->baseline += pool->m_baselineCodeBytes;
            sizes->asmJS    += pool->m_asmJSCodeBytes;
            sizes->regexp   += pool->m_regexpCodeBytes;
            sizes->other    += pool->m_otherCodeBytes;
            sizes->unused   += pool->m_allocation.size - pool->m_jaegerCodeBytes
                                                       - pool->m_ionCodeBytes
                                                       - pool->m_baselineCodeBytes
                                                       - pool->m_asmJSCodeBytes
                                                       - pool->m_regexpCodeBytes
                                                       - pool->m_otherCodeBytes;
        }
    }
}

}

#endif 

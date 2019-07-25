
























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
ExecutableAllocator::getCodeStats(size_t& method, size_t& regexp, size_t& unused) const
{
    method = 0;
    regexp = 0;
    unused = 0;

    for (ExecPoolHashSet::Range r = m_pools.all(); !r.empty(); r.popFront()) {
        ExecutablePool* pool = r.front();
        method += pool->m_mjitCodeMethod;
        regexp += pool->m_mjitCodeRegexp;
        unused += pool->m_allocation.size - pool->m_mjitCodeMethod - pool->m_mjitCodeRegexp;
    }
}

}

#endif 

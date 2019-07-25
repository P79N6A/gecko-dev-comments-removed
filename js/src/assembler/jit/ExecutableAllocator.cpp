
























#include "ExecutableAllocator.h"

#if ENABLE_ASSEMBLER

namespace JSC {

size_t ExecutableAllocator::pageSize = 0;
size_t ExecutableAllocator::largeAllocSize = 0;

ExecutablePool::~ExecutablePool()
{
    m_allocator->releasePoolPages(this);
}

size_t
ExecutableAllocator::getCodeSize() const
{
    size_t n = 0;
    for (ExecPoolHashSet::Range r = m_pools.all(); !r.empty(); r.popFront()) {
        ExecutablePool* pool = r.front();
        n += pool->m_allocation.size;
    }
    return n;
}

}

#endif 

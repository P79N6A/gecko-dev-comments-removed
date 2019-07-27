


























#include "jit/ExecutableAllocator.h"

#include "js/MemoryMetrics.h"

using namespace js::jit;

size_t ExecutableAllocator::pageSize = 0;
size_t ExecutableAllocator::largeAllocSize = 0;

ExecutablePool::~ExecutablePool()
{
    MOZ_ASSERT(m_ionCodeBytes == 0);
    MOZ_ASSERT(m_baselineCodeBytes == 0);
    MOZ_ASSERT(m_regexpCodeBytes == 0);
    MOZ_ASSERT(m_otherCodeBytes == 0);

    m_allocator->releasePoolPages(this);
}

void
ExecutableAllocator::addSizeOfCode(JS::CodeSizes *sizes) const
{
    if (m_pools.initialized()) {
        for (ExecPoolHashSet::Range r = m_pools.all(); !r.empty(); r.popFront()) {
            ExecutablePool* pool = r.front();
            sizes->ion      += pool->m_ionCodeBytes;
            sizes->baseline += pool->m_baselineCodeBytes;
            sizes->regexp   += pool->m_regexpCodeBytes;
            sizes->other    += pool->m_otherCodeBytes;
            sizes->unused   += pool->m_allocation.size - pool->m_ionCodeBytes
                                                       - pool->m_baselineCodeBytes
                                                       - pool->m_regexpCodeBytes
                                                       - pool->m_otherCodeBytes;
        }
    }
}

void
ExecutableAllocator::toggleAllCodeAsAccessible(bool accessible)
{
    if (!m_pools.initialized())
        return;

    for (ExecPoolHashSet::Range r = m_pools.all(); !r.empty(); r.popFront()) {
        ExecutablePool* pool = r.front();
        pool->toggleAllCodeAsAccessible(accessible);
    }
}

bool
ExecutableAllocator::codeContains(char* address)
{
    if (!m_pools.initialized())
        return false;

    for (ExecPoolHashSet::Range r = m_pools.all(); !r.empty(); r.popFront()) {
        ExecutablePool* pool = r.front();
        if (pool->codeContains(address))
            return true;
    }

    return false;
}

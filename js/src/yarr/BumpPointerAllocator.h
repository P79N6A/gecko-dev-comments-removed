




























#ifndef BumpPointerAllocator_h
#define BumpPointerAllocator_h

#include "PageAllocation.h"

namespace WTF {

#define MINIMUM_BUMP_POOL_SIZE 0x1000

class BumpPointerPool {
public:
    
    
    
    
    
    
    
    
    BumpPointerPool* ensureCapacity(size_t size)
    {
        void* allocationEnd = static_cast<char*>(m_current) + size;
        ASSERT(allocationEnd > m_current); 
        if (allocationEnd <= static_cast<void*>(this))
            return this;
        return ensureCapacityCrossPool(this, size);
    }

    
    
    void* alloc(size_t size)
    {
        void* current = m_current;
        void* allocationEnd = static_cast<char*>(current) + size;
        ASSERT(allocationEnd > current); 
        ASSERT(allocationEnd <= static_cast<void*>(this));
        m_current = allocationEnd;
        return current;
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    BumpPointerPool* dealloc(void* position)
    {
        if ((position >= m_start) && (position <= static_cast<void*>(this))) {
            ASSERT(position <= m_current);
            m_current = position;
            return this;
        }
        return deallocCrossPool(this, position);
    }

private:
    
    void* operator new(size_t size, const PageAllocation& allocation)
    {
        ASSERT(size < allocation.size());
        return reinterpret_cast<char*>(reinterpret_cast<intptr_t>(allocation.base()) + allocation.size()) - size;
    }

    BumpPointerPool(const PageAllocation& allocation)
        : m_current(allocation.base())
        , m_start(allocation.base())
        , m_next(0)
        , m_previous(0)
        , m_allocation(allocation)
    {
    }

    static BumpPointerPool* create(size_t minimumCapacity = 0)
    {
        
        minimumCapacity += sizeof(BumpPointerPool);
        if (minimumCapacity < sizeof(BumpPointerPool))
            return 0;

        size_t poolSize = MINIMUM_BUMP_POOL_SIZE;
        while (poolSize < minimumCapacity) {
            poolSize <<= 1;
            
            ASSERT(!(MINIMUM_BUMP_POOL_SIZE & (MINIMUM_BUMP_POOL_SIZE - 1)));
            if (!poolSize)
                return 0;
        }

        PageAllocation allocation = PageAllocation::allocate(poolSize);
        if (!!allocation)
            return new(allocation) BumpPointerPool(allocation);
        return 0;
    }

    void shrink()
    {
        ASSERT(!m_previous);
        m_current = m_start;
        while (m_next) {
            BumpPointerPool* nextNext = m_next->m_next;
            m_next->destroy();
            m_next = nextNext;
        }
    }

    void destroy()
    {
        m_allocation.deallocate();
    }

    static BumpPointerPool* ensureCapacityCrossPool(BumpPointerPool* previousPool, size_t size)
    {
        
        ASSERT(previousPool);
        ASSERT((static_cast<char*>(previousPool->m_current) + size) > previousPool->m_current); 
        ASSERT((static_cast<char*>(previousPool->m_current) + size) > static_cast<void*>(previousPool));
        BumpPointerPool* pool = previousPool->m_next;

        while (true) {
            if (!pool) {
                
                pool = BumpPointerPool::create(size);
                previousPool->m_next = pool;
                pool->m_previous = previousPool;
                return pool;
            }

            
            void* current = pool->m_current;
            void* allocationEnd = static_cast<char*>(current) + size;
            ASSERT(allocationEnd > current); 
            if (allocationEnd <= static_cast<void*>(pool))
                return pool;
        }
    }

    static BumpPointerPool* deallocCrossPool(BumpPointerPool* pool, void* position)
    {
        
        ASSERT((position < pool->m_start) || (position > static_cast<void*>(pool)));

        while (true) {
            
            pool->m_current = pool->m_start;
            pool = pool->m_previous;

            
            if (!pool)
                CRASH();

            if ((position >= pool->m_start) && (position <= static_cast<void*>(pool))) {
                ASSERT(position <= pool->m_current);
                pool->m_current = position;
                return pool;
            }
        }
    }

    void* m_current;
    void* m_start;
    BumpPointerPool* m_next;
    BumpPointerPool* m_previous;
    PageAllocation m_allocation;

    friend class BumpPointerAllocator;
};
















class BumpPointerAllocator {
public:
    BumpPointerAllocator()
        : m_head(0)
    {
    }

    ~BumpPointerAllocator()
    {
        if (m_head)
            m_head->destroy();
    }

    BumpPointerPool* startAllocator()
    {
        if (!m_head)
            m_head = BumpPointerPool::create();
        return m_head;
    }

    void stopAllocator()
    {
        if (m_head)
            m_head->shrink();
    }

private:
    BumpPointerPool* m_head;
};

}

using WTF::BumpPointerAllocator;

#endif 

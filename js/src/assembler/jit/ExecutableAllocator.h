
























#ifndef ExecutableAllocator_h
#define ExecutableAllocator_h

#include <stddef.h> 
#include <limits>
#include "assembler/wtf/Assertions.h"

#include "jsapi.h"
#include "jshashtable.h"
#include "jsprvtd.h"
#include "jsvector.h"
#include "jslock.h"

#if WTF_CPU_SPARC
#ifdef linux  
static void sync_instruction_memory(caddr_t v, u_int len)
{
    caddr_t end = v + len;
    caddr_t p = v;
    while (p < end) {
        asm("flush %0" : : "r" (p));
        p += 32;
    }
}
#else
extern  "C" void sync_instruction_memory(caddr_t v, u_int len);
#endif
#endif

#if WTF_OS_IOS
#include <libkern/OSCacheControl.h>
#include <sys/mman.h>
#endif

#if WTF_OS_SYMBIAN
#include <e32std.h>
#endif

#if WTF_CPU_MIPS && WTF_OS_LINUX
#include <sys/cachectl.h>
#endif

#if ENABLE_ASSEMBLER_WX_EXCLUSIVE
#define PROTECTION_FLAGS_RW (PROT_READ | PROT_WRITE)
#define PROTECTION_FLAGS_RX (PROT_READ | PROT_EXEC)
#define INITIAL_PROTECTION_FLAGS PROTECTION_FLAGS_RX
#else
#define INITIAL_PROTECTION_FLAGS (PROT_READ | PROT_WRITE | PROT_EXEC)
#endif

#if ENABLE_ASSEMBLER



namespace JSC {

  class ExecutableAllocator;

  
  class ExecutablePool {

    JS_DECLARE_ALLOCATION_FRIENDS_FOR_PRIVATE_CONSTRUCTOR;
    friend class ExecutableAllocator;
private:
    struct Allocation {
        char* pages;
        size_t size;
#if WTF_OS_SYMBIAN
        RChunk* chunk;
#endif
    };

    ExecutableAllocator* m_allocator;
    char* m_freePtr;
    char* m_end;
    Allocation m_allocation;

    
    unsigned m_refCount;

public:
    
    bool m_destroy;

    
    
    size_t m_gcNumber;

    void release(bool willDestroy = false)
    { 
        JS_ASSERT(m_refCount != 0);
        
        
        if (--m_refCount == 0) {
            js::UnwantedForeground::delete_(this);
        }
    }

private:
    
    
    
    void addRef()
    {
        JS_ASSERT(m_refCount);
        ++m_refCount;
    }

    ExecutablePool(ExecutableAllocator* allocator, Allocation a)
      : m_allocator(allocator), m_freePtr(a.pages), m_end(m_freePtr + a.size), m_allocation(a),
        m_refCount(1), m_destroy(false), m_gcNumber(0)
    { }

    ~ExecutablePool();

    void* alloc(size_t n)
    {
        JS_ASSERT(n <= available());
        void *result = m_freePtr;
        m_freePtr += n;
        return result;
    }
    
    size_t available() const { 
        JS_ASSERT(m_end >= m_freePtr);
        return m_end - m_freePtr;
    }
};

class ExecutableAllocator {
    enum ProtectionSetting { Writable, Executable };

public:
    ExecutableAllocator()
    {
        if (!pageSize) {
            pageSize = determinePageSize();
            







            largeAllocSize = pageSize * 16;
        }

        JS_ASSERT(m_smallPools.empty());
    }

    ~ExecutableAllocator()
    {
        for (size_t i = 0; i < m_smallPools.length(); i++)
            m_smallPools[i]->release(true);
        
        
    }

    
    
    
    void* alloc(size_t n, ExecutablePool** poolp)
    {
        
        
        
        n = roundUpAllocationSize(n, sizeof(void*));
        if (n == OVERSIZE_ALLOCATION) {
            *poolp = NULL;
            return NULL;
        }

        *poolp = poolForSize(n);
        if (!*poolp)
            return NULL;

        
        
        void *result = (*poolp)->alloc(n);
        JS_ASSERT(result);
        return result;
    }

    void releasePoolPages(ExecutablePool *pool) {
        JS_ASSERT(pool->m_allocation.pages);
        systemRelease(pool->m_allocation);
        m_pools.remove(m_pools.lookup(pool));   
    }

    size_t getCodeSize() const;

private:
    static size_t pageSize;
    static size_t largeAllocSize;

    static const size_t OVERSIZE_ALLOCATION = size_t(-1);

    static size_t roundUpAllocationSize(size_t request, size_t granularity)
    {
        
        
        #ifdef _MSC_VER
        # undef max
        #endif

        if ((std::numeric_limits<size_t>::max() - granularity) <= request)
            return OVERSIZE_ALLOCATION;
        
        
        size_t size = request + (granularity - 1);
        size = size & ~(granularity - 1);
        JS_ASSERT(size >= request);
        return size;
    }

    
    static ExecutablePool::Allocation systemAlloc(size_t n);
    static void systemRelease(const ExecutablePool::Allocation& alloc);

    ExecutablePool* createPool(size_t n)
    {
        size_t allocSize = roundUpAllocationSize(n, pageSize);
        if (allocSize == OVERSIZE_ALLOCATION)
            return NULL;

        if (!m_pools.initialized() && !m_pools.init())
            return NULL;

#ifdef DEBUG_STRESS_JSC_ALLOCATOR
        ExecutablePool::Allocation a = systemAlloc(size_t(4294967291));
#else
        ExecutablePool::Allocation a = systemAlloc(allocSize);
#endif
        if (!a.pages)
            return NULL;

        ExecutablePool *pool = js::OffTheBooks::new_<ExecutablePool>(this, a);
        if (!pool) {
            systemRelease(a);
            return NULL;
        }
        m_pools.put(pool);
        return pool;
    }

public:
    ExecutablePool* poolForSize(size_t n)
    {
#ifndef DEBUG_STRESS_JSC_ALLOCATOR
        
        
        
        
        
        ExecutablePool *minPool = NULL;
        for (size_t i = 0; i < m_smallPools.length(); i++) {
            ExecutablePool *pool = m_smallPools[i];
            if (n <= pool->available() && (!minPool || pool->available() < minPool->available()))
                minPool = pool;
        }
        if (minPool) {
            minPool->addRef();
            return minPool;
        }
#endif

        
        if (n > largeAllocSize)
            return createPool(n);

        
        ExecutablePool* pool = createPool(largeAllocSize);
        if (!pool)
            return NULL;
  	    

        if (m_smallPools.length() < maxSmallPools) {
            
            m_smallPools.append(pool);
            pool->addRef();
        } else {
            
            int iMin = 0;
            for (size_t i = 1; i < m_smallPools.length(); i++)
                if (m_smallPools[i]->available() <
                    m_smallPools[iMin]->available())
                {
                    iMin = i;
                }

            
            
            ExecutablePool *minPool = m_smallPools[iMin];
            if ((pool->available() - n) > minPool->available()) {
                minPool->release();
                m_smallPools[iMin] = pool;
                pool->addRef();
            }
        }

   	    
        return pool;
    }

#if ENABLE_ASSEMBLER_WX_EXCLUSIVE
    static void makeWritable(void* start, size_t size)
    {
        reprotectRegion(start, size, Writable);
    }

    static void makeExecutable(void* start, size_t size)
    {
        reprotectRegion(start, size, Executable);
    }
#else
    static void makeWritable(void*, size_t) {}
    static void makeExecutable(void*, size_t) {}
#endif


#if WTF_CPU_X86 || WTF_CPU_X86_64
    static void cacheFlush(void*, size_t)
    {
    }
#elif WTF_CPU_MIPS
    static void cacheFlush(void* code, size_t size)
    {
#if WTF_COMPILER_GCC && (GCC_VERSION >= 40300)
#if WTF_MIPS_ISA_REV(2) && (GCC_VERSION < 40403)
        int lineSize;
        asm("rdhwr %0, $1" : "=r" (lineSize));
        
        
        
        
        
        
        
        
        intptr_t start = reinterpret_cast<intptr_t>(code) & (-lineSize);
        intptr_t end = ((reinterpret_cast<intptr_t>(code) + size - 1) & (-lineSize)) - 1;
        __builtin___clear_cache(reinterpret_cast<char*>(start), reinterpret_cast<char*>(end));
#else
        intptr_t end = reinterpret_cast<intptr_t>(code) + size;
        __builtin___clear_cache(reinterpret_cast<char*>(code), reinterpret_cast<char*>(end));
#endif
#else
        _flush_cache(reinterpret_cast<char*>(code), size, BCACHE);
#endif
    }
#elif WTF_CPU_ARM_THUMB2 && WTF_OS_IOS
    static void cacheFlush(void* code, size_t size)
    {
        sys_dcache_flush(code, size);
        sys_icache_invalidate(code, size);
    }
#elif WTF_CPU_ARM_THUMB2 && WTF_IOS
    static void cacheFlush(void* code, size_t size)
    {
        asm volatile (
            "push    {r7}\n"
            "mov     r0, %0\n"
            "mov     r1, %1\n"
            "movw    r7, #0x2\n"
            "movt    r7, #0xf\n"
            "movs    r2, #0x0\n"
            "svc     0x0\n"
            "pop     {r7}\n"
            :
            : "r" (code), "r" (reinterpret_cast<char*>(code) + size)
            : "r0", "r1", "r2");
    }
#elif WTF_OS_SYMBIAN
    static void cacheFlush(void* code, size_t size)
    {
        User::IMB_Range(code, static_cast<char*>(code) + size);
    }
#elif WTF_CPU_ARM_TRADITIONAL && WTF_OS_LINUX && WTF_COMPILER_RVCT
    static __asm void cacheFlush(void* code, size_t size);
#elif WTF_CPU_ARM_TRADITIONAL && (WTF_OS_LINUX || WTF_OS_ANDROID) && WTF_COMPILER_GCC
    static void cacheFlush(void* code, size_t size)
    {
        asm volatile (
            "push    {r7}\n"
            "mov     r0, %0\n"
            "mov     r1, %1\n"
            "mov     r7, #0xf0000\n"
            "add     r7, r7, #0x2\n"
            "mov     r2, #0x0\n"
            "svc     0x0\n"
            "pop     {r7}\n"
            :
            : "r" (code), "r" (reinterpret_cast<char*>(code) + size)
            : "r0", "r1", "r2");
    }
#elif WTF_CPU_SPARC
    static void cacheFlush(void* code, size_t size)
    {
        sync_instruction_memory((caddr_t)code, size);
    }
#else
    #error "The cacheFlush support is missing on this platform."
#endif

private:

#if ENABLE_ASSEMBLER_WX_EXCLUSIVE
    static void reprotectRegion(void*, size_t, ProtectionSetting);
#endif

    
    static const size_t maxSmallPools = 4;
    typedef js::Vector<ExecutablePool *, maxSmallPools, js::SystemAllocPolicy> SmallExecPoolVector;
    SmallExecPoolVector m_smallPools;

    
    
    
    typedef js::HashSet<ExecutablePool *, js::DefaultHasher<ExecutablePool *>, js::SystemAllocPolicy>
            ExecPoolHashSet;
    ExecPoolHashSet m_pools;    

    static size_t determinePageSize();
};

}

#endif 

#endif 







#ifndef _POOLALLOC_INCLUDED_
#define _POOLALLOC_INCLUDED_

#ifdef _DEBUG
#define GUARD_BLOCKS
#endif





















#include <stddef.h>
#include <string.h>
#include <vector>






class TAllocation {
public:
    TAllocation(size_t size, unsigned char* mem, TAllocation* prev = 0) :
        size(size), mem(mem), prevAlloc(prev) {
        
        
        
        
        
#ifdef GUARD_BLOCKS
        memset(preGuard(), guardBlockBeginVal, guardBlockSize);
        memset(data(),      userDataFill,       size);
        memset(postGuard(), guardBlockEndVal,   guardBlockSize);
#endif
    }

    void check() const {
        checkGuardBlock(preGuard(),  guardBlockBeginVal, "before");
        checkGuardBlock(postGuard(), guardBlockEndVal,   "after");
    }

    void checkAllocList() const;

    
    
    inline static size_t allocationSize(size_t size) {
        return size + 2 * guardBlockSize + headerSize();
    }

    
    inline static unsigned char* offsetAllocation(unsigned char* m) {
        return m + guardBlockSize + headerSize();
    }

private:
    void checkGuardBlock(unsigned char* blockMem, unsigned char val, const char* locText) const;

    
    unsigned char* preGuard()  const { return mem + headerSize(); }
    unsigned char* data()      const { return preGuard() + guardBlockSize; }
    unsigned char* postGuard() const { return data() + size; }

    size_t size;                  
    unsigned char* mem;           
    TAllocation* prevAlloc;       

    
    const static unsigned char guardBlockBeginVal;
    const static unsigned char guardBlockEndVal;
    const static unsigned char userDataFill;

    const static size_t guardBlockSize;
#ifdef GUARD_BLOCKS
    inline static size_t headerSize() { return sizeof(TAllocation); }
#else
    inline static size_t headerSize() { return 0; }
#endif
};















class TPoolAllocator {
public:
    TPoolAllocator(int growthIncrement = 8*1024, int allocationAlignment = 16);

    
    
    
    ~TPoolAllocator();

    
    
    
    
    void push();

    
    
    
    
    void pop();

    
    
    
    void popAll();

    
    
    
    
    void* allocate(size_t numBytes);

    
    
    
    
    
    

protected:
    friend struct tHeader;
    
    struct tHeader {
        tHeader(tHeader* nextPage, size_t pageCount) :
            nextPage(nextPage),
            pageCount(pageCount)
#ifdef GUARD_BLOCKS
          , lastAllocation(0)
#endif
            { }

        ~tHeader() {
#ifdef GUARD_BLOCKS
            if (lastAllocation)
                lastAllocation->checkAllocList();
#endif
        }

        tHeader* nextPage;
        size_t pageCount;
#ifdef GUARD_BLOCKS
        TAllocation* lastAllocation;
#endif
    };

    struct tAllocState {
        size_t offset;
        tHeader* page;
    };
    typedef std::vector<tAllocState> tAllocStack;

    
    void* initializeAllocation(tHeader* block, unsigned char* memory, size_t numBytes) {
#ifdef GUARD_BLOCKS
        new(memory) TAllocation(numBytes, memory, block->lastAllocation);
        block->lastAllocation = reinterpret_cast<TAllocation*>(memory);
#endif
        
        return TAllocation::offsetAllocation(memory);
    }

    size_t pageSize;        
    size_t alignment;       
                            
    size_t alignmentMask;
    size_t headerSkip;      
                            
                            
    size_t currentPageOffset;  
    tHeader* freeList;      
    tHeader* inUseList;     
    tAllocStack stack;      

    int numCalls;           
    size_t totalBytes;      
private:
    TPoolAllocator& operator=(const TPoolAllocator&);  
    TPoolAllocator(const TPoolAllocator&);  
};







extern TPoolAllocator& GetGlobalPoolAllocator();
extern void SetGlobalPoolAllocator(TPoolAllocator* poolAllocator);
#define GlobalPoolAllocator GetGlobalPoolAllocator()

struct TThreadGlobalPools
{
    TPoolAllocator* globalPoolAllocator;
};








template<class T>
class pool_allocator {
public:
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T& reference;
    typedef const T& const_reference;
    typedef T value_type;

    template<class Other> 
    struct rebind {
        typedef pool_allocator<Other> other;
    };
    pointer address(reference x) const { return &x; }
    const_pointer address(const_reference x) const { return &x; }

    pool_allocator() : allocator(GlobalPoolAllocator) { }
    pool_allocator(TPoolAllocator& a) : allocator(a) { }
    pool_allocator(const pool_allocator<T>& p) : allocator(p.allocator) { }

    template<class Other>
    pool_allocator(const pool_allocator<Other>& p) : allocator(p.getAllocator()) { }

#if defined(__SUNPRO_CC) && !defined(_RWSTD_ALLOCATOR)
    
    
    
    void* allocate(size_type n) { 
        return getAllocator().allocate(n);
    }
    void* allocate(size_type n, const void*) {
        return getAllocator().allocate(n);
    }
    void deallocate(void*, size_type) {}
#else
    pointer allocate(size_type n) { 
        return reinterpret_cast<pointer>(getAllocator().allocate(n * sizeof(T)));
    }
    pointer allocate(size_type n, const void*) { 
        return reinterpret_cast<pointer>(getAllocator().allocate(n * sizeof(T)));
    }
    void deallocate(pointer, size_type) {}
#endif  

    void construct(pointer p, const T& val) { new ((void *)p) T(val); }
    void destroy(pointer p) { p->T::~T(); }

    bool operator==(const pool_allocator& rhs) const { return &getAllocator() == &rhs.getAllocator(); }
    bool operator!=(const pool_allocator& rhs) const { return &getAllocator() != &rhs.getAllocator(); }

    size_type max_size() const { return static_cast<size_type>(-1) / sizeof(T); }
    size_type max_size(int size) const { return static_cast<size_type>(-1) / size; }

    void setAllocator(TPoolAllocator* a) { allocator = *a; }
    TPoolAllocator& getAllocator() const { return allocator; }

protected:
    TPoolAllocator& allocator;
};

#endif 











#ifndef GrAllocPool_DEFINED
#define GrAllocPool_DEFINED

#include "GrNoncopyable.h"

class GrAllocPool : GrNoncopyable {
public:
    GrAllocPool(size_t blockSize = 0);
    ~GrAllocPool();

    


    void reset();

    




    void* alloc(size_t bytes);
    
    


    void release(size_t bytes);

private:
    struct Block;

    Block*  fBlock;
    size_t  fMinBlockSize;

#if GR_DEBUG
    int fBlocksAllocated;
    void validate() const;
#else
    void validate() const {}
#endif
};

template <typename T> class GrTAllocPool {
public:
    GrTAllocPool(int count) : fPool(count * sizeof(T)) {}

    void reset() { fPool.reset(); }
    T* alloc() { return (T*)fPool.alloc(sizeof(T)); }

private:
    GrAllocPool fPool;
};

#endif


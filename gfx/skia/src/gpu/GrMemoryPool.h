






#ifndef GrMemoryPool_DEFINED
#define GrMemoryPool_DEFINED

#include "GrTypes.h"








class GrMemoryPool {
public:
    




    GrMemoryPool(size_t preallocSize, size_t minAllocSize);

    ~GrMemoryPool();

    


    void* allocate(size_t size);

    


    void release(void* p);

    


    bool isEmpty() const { return fTail == fHead && !fHead->fLiveCount; }

private:
    struct BlockHeader;

    BlockHeader* CreateBlock(size_t size);

    void DeleteBlock(BlockHeader* block);

    void validate();

    struct BlockHeader {
        BlockHeader* fNext;      
        BlockHeader* fPrev;
        int          fLiveCount; 
                                 
        intptr_t     fCurrPtr;   
        intptr_t     fPrevPtr;   
        size_t       fFreeSize;  
    };

    enum {
        
        kAlignment    = 8,
        kHeaderSize   = GR_CT_ALIGN_UP(sizeof(BlockHeader), kAlignment),
        kPerAllocPad  = GR_CT_ALIGN_UP(sizeof(BlockHeader*), kAlignment),
    };
    size_t                            fPreallocSize;
    size_t                            fMinAllocSize;
    BlockHeader*                      fHead;
    BlockHeader*                      fTail;
#if GR_DEBUG
    int                               fAllocationCnt;
#endif
};

#endif

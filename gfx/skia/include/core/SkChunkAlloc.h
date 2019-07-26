








#ifndef SkChunkAlloc_DEFINED
#define SkChunkAlloc_DEFINED

#include "SkTypes.h"

class SkChunkAlloc : SkNoncopyable {
public:
    SkChunkAlloc(size_t minSize);
    ~SkChunkAlloc();

    



    void reset();

    enum AllocFailType {
        kReturnNil_AllocFailType,
        kThrow_AllocFailType
    };

    void* alloc(size_t bytes, AllocFailType);
    void* allocThrow(size_t bytes) {
        return this->alloc(bytes, kThrow_AllocFailType);
    }

    





    size_t unalloc(void* ptr);

    size_t totalCapacity() const { return fTotalCapacity; }
    int blockCount() const { return fBlockCount; }

    




    bool contains(const void* addr) const;

private:
    struct Block;

    Block*  fBlock;
    size_t  fMinSize;
    size_t  fChunkSize;
    size_t  fTotalCapacity;
    int     fBlockCount;

    Block* newBlock(size_t bytes, AllocFailType ftype);
};

#endif

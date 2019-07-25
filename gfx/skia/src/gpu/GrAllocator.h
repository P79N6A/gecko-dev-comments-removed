









#ifndef GrAllocator_DEFINED
#define GrAllocator_DEFINED

#include "GrConfig.h"
#include "SkTArray.h"

class GrAllocator : GrNoncopyable {
public:
    ~GrAllocator() {
        reset();
    }

    








    GrAllocator(size_t itemSize, int itemsPerBlock, void* initialBlock) :
            fItemSize(itemSize),
            fItemsPerBlock(itemsPerBlock),
            fOwnFirstBlock(NULL == initialBlock),
            fCount(0) {
        GrAssert(itemsPerBlock > 0);
        fBlockSize = fItemSize * fItemsPerBlock;
        fBlocks.push_back() = initialBlock;
        GR_DEBUGCODE(if (!fOwnFirstBlock) {*((char*)initialBlock+fBlockSize-1)='a';} );
    }

    




    void* push_back() {
        int indexInBlock = fCount % fItemsPerBlock;
        
        if (0 == indexInBlock) {
            if (0 != fCount) {
                fBlocks.push_back() = GrMalloc(fBlockSize);
            } else if (fOwnFirstBlock) {
                fBlocks[0] = GrMalloc(fBlockSize);
            }
        }        
        void* ret = (char*)fBlocks[fCount/fItemsPerBlock] + 
                    fItemSize * indexInBlock;
        ++fCount;
        return ret;
    }

    


    void reset() {        
        int blockCount = GrMax((unsigned)1, 
                               GrUIDivRoundUp(fCount, fItemsPerBlock));
        for (int i = 1; i < blockCount; ++i) {
            GrFree(fBlocks[i]);
        }
        if (fOwnFirstBlock) {
            GrFree(fBlocks[0]);
            fBlocks[0] = NULL;
        }
        fBlocks.pop_back_n(blockCount-1);
        fCount = 0;
    }

    


    int count() const {
        return fCount;
    }
    
    


    bool empty() const { return fCount == 0; }
    
    


    void* back() {
        GrAssert(fCount);
        return (*this)[fCount-1];
    }
    
    


    const void* back() const {
        GrAssert(fCount);
        return (*this)[fCount-1];
    }
    
    

    
    void* operator[] (int i) {
        GrAssert(i >= 0 && i < fCount);
        return (char*)fBlocks[i / fItemsPerBlock] + 
               fItemSize * (i % fItemsPerBlock);
    }

    

  
    const void* operator[] (int i) const {
        GrAssert(i >= 0 && i < fCount);
        return (const char*)fBlocks[i / fItemsPerBlock] + 
               fItemSize * (i % fItemsPerBlock);
    }

private:
    static const int NUM_INIT_BLOCK_PTRS = 8;
    
    SkSTArray<NUM_INIT_BLOCK_PTRS, void*>   fBlocks;
    size_t                                  fBlockSize;
    size_t                                  fItemSize;
    int                                     fItemsPerBlock;
    bool                                    fOwnFirstBlock;
    int                                     fCount;

    typedef GrNoncopyable INHERITED;
};

template <typename T>
class GrTAllocator : GrNoncopyable {

public:
    virtual ~GrTAllocator() { this->reset(); };

    







    explicit GrTAllocator(int itemsPerBlock)
        : fAllocator(sizeof(T), itemsPerBlock, NULL) {}

    




    T& push_back() {
        void* item = fAllocator.push_back();
        GrAssert(NULL != item);
        new (item) T;
        return *(T*)item;
    }

    T& push_back(const T& t) {
        void* item = fAllocator.push_back();
        GrAssert(NULL != item);
        new (item) T(t);
        return *(T*)item;
    }

    


    void reset() {
        int c = fAllocator.count();
        for (int i = 0; i < c; ++i) {
            ((T*)fAllocator[i])->~T();
        }
        fAllocator.reset();
    }
    
    


    int count() const {
        return fAllocator.count();
    }
    
    


    bool empty() const { return fAllocator.empty(); }
    
    


    T& back() {
        return *(T*)fAllocator.back();
    }

    


    const T& back() const {
        return *(const T*)fAllocator.back();
    }

    

  
    T& operator[] (int i) {
        return *(T*)(fAllocator[i]);
    }
    
    


    const T& operator[] (int i) const {
        return *(const T*)(fAllocator[i]);
    }

protected:
    GrTAllocator(int itemsPerBlock, void* initialBlock)
        : fAllocator(sizeof(T), itemsPerBlock, initialBlock) {
    }

private:
    GrAllocator fAllocator;
    typedef GrNoncopyable INHERITED;
};

template <int N, typename T> class GrSTAllocator : public GrTAllocator<T> {
private:
    typedef GrTAllocator<T> INHERITED;

public:
    GrSTAllocator() : INHERITED(N, fStorage.get()) {
    }

private:
    SkAlignedSTStorage<N, T> fStorage;
};

#endif

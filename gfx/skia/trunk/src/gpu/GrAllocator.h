






#ifndef GrAllocator_DEFINED
#define GrAllocator_DEFINED

#include "GrConfig.h"
#include "GrTypes.h"
#include "SkTArray.h"
#include "SkTypes.h"

class GrAllocator : public SkNoncopyable {
public:
    ~GrAllocator() {
        reset();
    }

    








    GrAllocator(size_t itemSize, int itemsPerBlock, void* initialBlock) :
            fItemSize(itemSize),
            fItemsPerBlock(itemsPerBlock),
            fOwnFirstBlock(NULL == initialBlock),
            fCount(0) {
        SkASSERT(itemsPerBlock > 0);
        fBlockSize = fItemSize * fItemsPerBlock;
        fBlocks.push_back() = initialBlock;
        SkDEBUGCODE(if (!fOwnFirstBlock) {*((char*)initialBlock+fBlockSize-1)='a';} );
    }

    







    void setInitialBlock(void* initialBlock) {
        SkASSERT(0 == fCount);
        SkASSERT(1 == fBlocks.count());
        SkASSERT(NULL == fBlocks.back());
        fOwnFirstBlock = false;
        fBlocks.back() = initialBlock;
    }

    




    void* push_back() {
        int indexInBlock = fCount % fItemsPerBlock;
        
        if (0 == indexInBlock) {
            if (0 != fCount) {
                fBlocks.push_back() = sk_malloc_throw(fBlockSize);
            } else if (fOwnFirstBlock) {
                fBlocks[0] = sk_malloc_throw(fBlockSize);
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
            sk_free(fBlocks[i]);
        }
        if (fOwnFirstBlock) {
            sk_free(fBlocks[0]);
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
        SkASSERT(fCount);
        return (*this)[fCount-1];
    }

    


    const void* back() const {
        SkASSERT(fCount);
        return (*this)[fCount-1];
    }

    


    void* operator[] (int i) {
        SkASSERT(i >= 0 && i < fCount);
        return (char*)fBlocks[i / fItemsPerBlock] +
               fItemSize * (i % fItemsPerBlock);
    }

    


    const void* operator[] (int i) const {
        SkASSERT(i >= 0 && i < fCount);
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

    typedef SkNoncopyable INHERITED;
};

template <typename T>
class GrTAllocator : public SkNoncopyable {
public:
    virtual ~GrTAllocator() { this->reset(); };

    




    explicit GrTAllocator(int itemsPerBlock)
        : fAllocator(sizeof(T), itemsPerBlock, NULL) {}

    




    T& push_back() {
        void* item = fAllocator.push_back();
        SkASSERT(NULL != item);
        SkNEW_PLACEMENT(item, T);
        return *(T*)item;
    }

    T& push_back(const T& t) {
        void* item = fAllocator.push_back();
        SkASSERT(NULL != item);
        SkNEW_PLACEMENT_ARGS(item, T, (t));
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
    






    void setInitialBlock(void* initialBlock) {
        fAllocator.setInitialBlock(initialBlock);
    }

private:
    GrAllocator fAllocator;
    typedef SkNoncopyable INHERITED;
};

template <int N, typename T> class GrSTAllocator : public GrTAllocator<T> {
private:
    typedef GrTAllocator<T> INHERITED;

public:
    GrSTAllocator() : INHERITED(N) {
        this->setInitialBlock(fStorage.get());
    }

private:
    SkAlignedSTStorage<N, T> fStorage;
};

#endif

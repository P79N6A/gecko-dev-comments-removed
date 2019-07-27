






#ifndef GrAllocator_DEFINED
#define GrAllocator_DEFINED

#include "GrConfig.h"
#include "GrTypes.h"
#include "SkTArray.h"
#include "SkTypes.h"

class GrAllocator : SkNoncopyable {
public:
    ~GrAllocator() { this->reset(); }

    








    GrAllocator(size_t itemSize, int itemsPerBlock, void* initialBlock)
        : fItemSize(itemSize)
        , fItemsPerBlock(itemsPerBlock)
        , fOwnFirstBlock(NULL == initialBlock)
        , fCount(0)
        , fInsertionIndexInBlock(0) {
        SkASSERT(itemsPerBlock > 0);
        fBlockSize = fItemSize * fItemsPerBlock;
        if (fOwnFirstBlock) {
            
            fInsertionIndexInBlock = fItemsPerBlock;
        } else {
            fBlocks.push_back() = initialBlock;
            fInsertionIndexInBlock = 0;
        }
    }

    




    void* push_back() {
        
        if (fItemsPerBlock == fInsertionIndexInBlock) {
            fBlocks.push_back() = sk_malloc_throw(fBlockSize);
            fInsertionIndexInBlock = 0;
        }
        void* ret = (char*)fBlocks.back() + fItemSize * fInsertionIndexInBlock;
        ++fCount;
        ++fInsertionIndexInBlock;
        return ret;
    }

    


    void reset() {
        int firstBlockToFree = fOwnFirstBlock ? 0 : 1;
        for (int i = firstBlockToFree; i < fBlocks.count(); ++i) {
            sk_free(fBlocks[i]);
        }
        if (fOwnFirstBlock) {
            fBlocks.reset();
            
            fInsertionIndexInBlock = fItemsPerBlock;
        } else {
            fBlocks.pop_back_n(fBlocks.count() - 1);
            fInsertionIndexInBlock = 0;
        }
        fCount = 0;
    }

    


    int count() const {
        return fCount;
    }

    


    bool empty() const { return 0 == fCount; }

    


    void* back() {
        SkASSERT(fCount);
        SkASSERT(fInsertionIndexInBlock > 0);
        return (char*)(fBlocks.back()) + (fInsertionIndexInBlock - 1) * fItemSize;
    }

    


    const void* back() const {
        SkASSERT(fCount);
        SkASSERT(fInsertionIndexInBlock > 0);
        return (const char*)(fBlocks.back()) + (fInsertionIndexInBlock - 1) * fItemSize;
    }


    



    class Iter {
    public:
        


        Iter(const GrAllocator* allocator)
            : fAllocator(allocator)
            , fBlockIndex(-1)
            , fIndexInBlock(allocator->fItemsPerBlock - 1)
            , fItemIndex(-1) {}

        


        bool next() {
            ++fIndexInBlock;
            ++fItemIndex;
            if (fIndexInBlock == fAllocator->fItemsPerBlock) {
                ++fBlockIndex;
                fIndexInBlock = 0;
            }
            return fItemIndex < fAllocator->fCount;
        }

        



        void* get() const {
            SkASSERT(fItemIndex >= 0 && fItemIndex < fAllocator->fCount);
            return (char*) fAllocator->fBlocks[fBlockIndex] + fIndexInBlock * fAllocator->fItemSize;
        }

    private:
        const GrAllocator* fAllocator;
        int                fBlockIndex;
        int                fIndexInBlock;
        int                fItemIndex;
    };

    


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

protected:
    







    void setInitialBlock(void* initialBlock) {
        SkASSERT(0 == fCount);
        SkASSERT(0 == fBlocks.count());
        SkASSERT(fItemsPerBlock == fInsertionIndexInBlock);
        fOwnFirstBlock = false;
        fBlocks.push_back() = initialBlock;
        fInsertionIndexInBlock = 0;
    }

    
    template <typename T> friend class GrTAllocator;

private:
    static const int NUM_INIT_BLOCK_PTRS = 8;

    SkSTArray<NUM_INIT_BLOCK_PTRS, void*, true>   fBlocks;
    size_t                                        fBlockSize;
    size_t                                        fItemSize;
    int                                           fItemsPerBlock;
    bool                                          fOwnFirstBlock;
    int                                           fCount;
    int                                           fInsertionIndexInBlock;

    typedef SkNoncopyable INHERITED;
};

template <typename T>
class GrTAllocator : SkNoncopyable {
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

    



    class Iter {
    public:
        


        Iter(const GrTAllocator* allocator) : fImpl(&allocator->fAllocator) {}

        


        bool next() { return fImpl.next(); }

        



        T* get() const { return (T*) fImpl.get(); }

        


        T& operator*() const { return *this->get(); }
        T* operator->() const { return this->get(); }

    private:
        GrAllocator::Iter fImpl;
    };

    


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

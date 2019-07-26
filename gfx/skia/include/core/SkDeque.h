








#ifndef SkDeque_DEFINED
#define SkDeque_DEFINED

#include "SkTypes.h"












class SK_API SkDeque : SkNoncopyable {
public:
    



    explicit SkDeque(size_t elemSize, int allocCount = 1);
    SkDeque(size_t elemSize, void* storage, size_t storageSize, int allocCount = 1);
    ~SkDeque();

    bool    empty() const { return 0 == fCount; }
    int     count() const { return fCount; }
    size_t  elemSize() const { return fElemSize; }

    const void* front() const { return fFront; }
    const void* back() const  { return fBack; }

    void* front() {
        return (void*)((const SkDeque*)this)->front();
    }

    void* back() {
        return (void*)((const SkDeque*)this)->back();
    }

    



    void* push_front();
    void* push_back();

    void pop_front();
    void pop_back();

private:
    struct Block;

public:
    class Iter {
    public:
        enum IterStart {
            kFront_IterStart,
            kBack_IterStart
        };

        


        Iter();

        Iter(const SkDeque& d, IterStart startLoc);
        void* next();
        void* prev();

        void reset(const SkDeque& d, IterStart startLoc);

    private:
        SkDeque::Block*  fCurBlock;
        char*           fPos;
        size_t          fElemSize;
    };

    
    class F2BIter : private Iter {
    public:
        F2BIter() {}

        



        F2BIter(const SkDeque& d) : INHERITED(d, kFront_IterStart) {}

        using Iter::next;

        



        void reset(const SkDeque& d) {
            this->INHERITED::reset(d, kFront_IterStart);
        }

    private:
        typedef Iter INHERITED;
    };

private:
    
    friend class DequeUnitTestHelper;

    void*   fFront;
    void*   fBack;

    Block*  fFrontBlock;
    Block*  fBackBlock;
    size_t  fElemSize;
    void*   fInitialStorage;
    int     fCount;             
    int     fAllocCount;        

    Block*  allocateBlock(int allocCount);
    void    freeBlock(Block* block);

    



    int  numBlocksAllocated() const;
};

#endif

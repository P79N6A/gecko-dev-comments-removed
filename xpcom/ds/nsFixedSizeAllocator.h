






























































































































#ifndef nsFixedSizeAllocator_h__
#define nsFixedSizeAllocator_h__

#include "nscore.h"
#include "nsError.h"
#include "plarena.h"

#define NS_SIZE_IN_HEAP(_size) (_size)

class NS_COM nsFixedSizeAllocator
{
protected:
    PLArenaPool mPool;

    struct Bucket;
    struct FreeEntry;
  
    friend struct Bucket;
    friend struct FreeEntry;

    struct FreeEntry {
        FreeEntry* mNext;
    };

    struct Bucket {
        size_t     mSize;
        FreeEntry* mFirst;
        Bucket*    mNext;
    };

    Bucket* mBuckets;

    Bucket *
    AddBucket(size_t aSize);

    Bucket *
    FindBucket(size_t aSize);

public:
    nsFixedSizeAllocator() : mBuckets(nsnull) {}

    ~nsFixedSizeAllocator() {
        if (mBuckets)
            PL_FinishArenaPool(&mPool);
    }

    






    nsresult
    Init(const char* aName,
         const size_t* aBucketSizes,
         PRInt32 aNumBuckets,
         PRInt32 aInitialSize,
         PRInt32 aAlign = 0);

    


    void* Alloc(size_t aSize);

    


    void Free(void* aPtr, size_t aSize);
};



#endif 

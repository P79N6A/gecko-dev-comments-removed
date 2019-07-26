




#include "nsDebug.h"
#include "nsFixedSizeAllocator.h"

nsFixedSizeAllocator::Bucket *
nsFixedSizeAllocator::AddBucket(size_t aSize)
{
    void* p;
    PL_ARENA_ALLOCATE(p, &mPool, sizeof(Bucket));
    if (! p)
        return nullptr;

    Bucket* bucket = static_cast<Bucket*>(p);
    bucket->mSize  = aSize;
    bucket->mFirst = nullptr;
    bucket->mNext  = mBuckets;

    mBuckets = bucket;
    return bucket;
}

nsresult
nsFixedSizeAllocator::Init(const char* aName,
                           const size_t* aBucketSizes,
                           int32_t aNumBuckets,
                           int32_t aChunkSize,
                           int32_t aAlign)
{
    NS_PRECONDITION(aNumBuckets > 0, "no buckets");
    if (aNumBuckets <= 0)
        return NS_ERROR_INVALID_ARG;

    
    if (mBuckets)
        PL_FinishArenaPool(&mPool);

    PL_InitArenaPool(&mPool, aName, aChunkSize, aAlign);

    mBuckets = nullptr;
    for (int32_t i = 0; i < aNumBuckets; ++i)
        AddBucket(aBucketSizes[i]);

    return NS_OK;
}

nsFixedSizeAllocator::Bucket *
nsFixedSizeAllocator::FindBucket(size_t aSize)
{
    Bucket** link = &mBuckets;
    Bucket* bucket;

    while ((bucket = *link) != nullptr) {
        if (aSize == bucket->mSize) {
            
            
            *link = bucket->mNext;
            bucket->mNext = mBuckets;
            mBuckets = bucket;
            return bucket;
        }

        link = &bucket->mNext;
    }
    return nullptr;
}

void*
nsFixedSizeAllocator::Alloc(size_t aSize)
{
    Bucket* bucket = FindBucket(aSize);
    if (! bucket) {
        
        bucket = AddBucket(aSize);
        if (! bucket)
            return nullptr;
    }

    void* next;
    if (bucket->mFirst) {
        next = bucket->mFirst;
        bucket->mFirst = bucket->mFirst->mNext;
    }
    else {
        PL_ARENA_ALLOCATE(next, &mPool, aSize);
        if (!next)
            return nullptr;
    }

#ifdef DEBUG
    memset(next, 0xc8, aSize);
#endif

    return next;
}

void
nsFixedSizeAllocator::Free(void* aPtr, size_t aSize)
{
    FreeEntry* entry = reinterpret_cast<FreeEntry*>(aPtr);
    Bucket* bucket = FindBucket(aSize);

#ifdef DEBUG
    NS_ASSERTION(bucket && bucket->mSize == aSize, "ack! corruption! bucket->mSize != aSize!");
    memset(aPtr, 0xd8, bucket->mSize);
#endif

    entry->mNext = bucket->mFirst;
    bucket->mFirst = entry;
}

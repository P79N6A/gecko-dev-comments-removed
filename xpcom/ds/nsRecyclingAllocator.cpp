









































#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "nsRecyclingAllocator.h"
#include "nsIMemory.h"
#include "nsAutoLock.h"
#include "prprf.h"
#include "nsITimer.h"

#define NS_SEC_TO_MS(s) ((s) * 1000)

void
nsRecyclingAllocator::nsRecycleTimerCallback(nsITimer *aTimer, void *aClosure)
{
    nsRecyclingAllocator *obj = (nsRecyclingAllocator *) aClosure;

    nsAutoLock lock(obj->mLock);

    if (!obj->mTouched)
    {
        obj->ClearFreeList();
    }
    else
    {
        
        
        obj->mTouched = PR_FALSE;
    }
}


nsRecyclingAllocator::nsRecyclingAllocator(PRUint32 nbucket, PRUint32 recycleAfter, const char *id) :
    mMaxBlocks(nbucket), mFreeListCount(0), mFreeList(nsnull),
    mRecycleTimer(nsnull), mRecycleAfter(recycleAfter), mTouched(PR_FALSE)
#ifdef DEBUG
    , mId(id), mNAllocated(0)
#endif
{
    mLock = PR_NewLock();
    NS_ASSERTION(mLock, "Recycling allocator cannot get lock");
}

nsresult
nsRecyclingAllocator::Init(PRUint32 nbucket, PRUint32 recycleAfter, const char *id)
{
    nsAutoLock lock(mLock);

    ClearFreeList();

    
    mMaxBlocks = nbucket;
    mRecycleAfter = recycleAfter;
#ifdef DEBUG
    mId = id;
#endif

    return NS_OK;
}

nsRecyclingAllocator::~nsRecyclingAllocator()
{
    ClearFreeList();

    if (mLock)
    {
        PR_DestroyLock(mLock);
        mLock = nsnull;
    }
}


void*
nsRecyclingAllocator::Malloc(PRSize bytes, PRBool zeroit)
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (mFreeList) {
        nsAutoLock lock(mLock);

        
        
        mTouched = PR_TRUE;

        Block* freeNode = mFreeList;
        Block** prevp = &mFreeList;

        while (freeNode)
        {
            if (freeNode->bytes >= bytes)
            {
                
                
                *prevp = freeNode->next;
                mFreeListCount --;

                void *data = DATA(freeNode);
                if (zeroit)
                    memset(data, 0, bytes);
                return data;
            }

            prevp = &(freeNode->next);
            freeNode = freeNode->next;
        }
    }
    
    
    
    PRSize allocBytes = bytes + NS_ALLOCATOR_OVERHEAD_BYTES;

    
    if (allocBytes < sizeof(Block)) allocBytes = sizeof(Block);

    
    Block *ptr = (Block *) (zeroit ? calloc(1, allocBytes) : malloc(allocBytes));
  
    
    if (!ptr)
        return ptr;

#ifdef DEBUG
    mNAllocated++;
#endif
  
    
    ptr->bytes = bytes;
    return DATA(ptr);
}

void
nsRecyclingAllocator::Free(void *ptr)
{
    Block* block = DATA_TO_BLOCK(ptr);

    nsAutoLock lock(mLock);

    
    
    mTouched = PR_TRUE;

    
    if (mFreeListCount < mMaxBlocks) {
        
        Block* freeNode = mFreeList;
        Block** prevp = &mFreeList;
        while (freeNode)
        {
            if (freeNode->bytes >= block->bytes)
                break;
            prevp = &(freeNode->next);
            freeNode = freeNode->next;
        }

        
        *prevp = block;
        block->next = freeNode;
        mFreeListCount ++;
    } else {
        
#ifdef DEBUG_dp
        char buf[1024];
        
        
        
        PR_snprintf(buf, sizeof(buf), "nsRecyclingAllocator(%s) FAILOVER 0x%p (%d) - %d allocations, %d max\n",
                    mId, (char *)ptr, block->bytes, mNAllocated, mMaxBlocks);
        NS_WARNING(buf);
        mNAllocated--;
#endif
        free(block);
    }

    
    
    
    
    if (mRecycleAfter && !mRecycleTimer)
    {
        
        extern nsresult NS_NewTimer(nsITimer* *aResult, nsTimerCallbackFunc aCallback, void *aClosure,
                                    PRUint32 aDelay, PRUint32 aType);

        (void) NS_NewTimer(&mRecycleTimer, nsRecycleTimerCallback, this,
                           NS_SEC_TO_MS(mRecycleAfter),
                           nsITimer::TYPE_REPEATING_SLACK);
        
        if (!mRecycleTimer)
            NS_WARNING("nsRecyclingAllocator: Creating timer failed");
    }
}






void
nsRecyclingAllocator::ClearFreeList()
{
#ifdef DEBUG_dp
    printf("DEBUG: nsRecyclingAllocator(%s) ClearFreeList (%d): ", mId, mFreeListCount);
#endif
    
    
    
    
    if (mRecycleTimer)
    {
        mRecycleTimer->Cancel();
        NS_RELEASE(mRecycleTimer);
    }

    
    Block* node = mFreeList;
    while (node)
    {
#ifdef DEBUG_dp
        printf("%d ", node->bytes);
#endif
        Block *next = node->next;
        free(node);
        node = next;
    }
    mFreeList = nsnull;
    mFreeListCount = 0;

#ifdef DEBUG
    mNAllocated = 0;
#endif
#ifdef DEBUG_dp
    printf("\n");
#endif
}







NS_IMPL_THREADSAFE_ISUPPORTS2(nsRecyclingAllocatorImpl, nsIMemory, nsIRecyclingAllocator)

NS_IMETHODIMP_(void *)
nsRecyclingAllocatorImpl::Alloc(PRSize size)
{
    return nsRecyclingAllocatorImpl::Malloc(size, PR_FALSE);
}

NS_IMETHODIMP_(void *)
nsRecyclingAllocatorImpl::Realloc(void *ptr, PRSize size)
{
    
    return NULL;
}

NS_IMETHODIMP_(void)
nsRecyclingAllocatorImpl::Free(void *ptr)
{
    nsRecyclingAllocator::Free(ptr);
}

NS_IMETHODIMP
nsRecyclingAllocatorImpl::Init(size_t nbuckets, size_t recycleAfter, const char *id)
{
    return nsRecyclingAllocator::Init((PRUint32) nbuckets, (PRUint32) recycleAfter, id);
}

NS_IMETHODIMP
nsRecyclingAllocatorImpl::HeapMinimize(PRBool immediate)
{
    
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsRecyclingAllocatorImpl::IsLowMemory(PRBool *lowmemoryb_ptr)
{
    
    return NS_ERROR_NOT_IMPLEMENTED;
}






























































#ifndef nsRecyclingAllocator_h__
#define nsRecyclingAllocator_h__

#include "nscore.h"
#include "prlock.h"
#include "nsIRecyclingAllocator.h"
#include "nsIGenericFactory.h"

#define NS_DEFAULT_RECYCLE_TIMEOUT 10  // secs
#define NS_ALLOCATOR_OVERHEAD_BYTES (sizeof(PRSize)) // bytes

class nsITimer;
class nsIMemory;

class NS_COM nsRecyclingAllocator {
 protected:
    struct Block {
      PRSize bytes;
      Block *next;
    };

#define DATA(block) ((void *)(((char *)block) + NS_ALLOCATOR_OVERHEAD_BYTES))
#define DATA_TO_BLOCK(data) ((Block *)((char *)(data) - NS_ALLOCATOR_OVERHEAD_BYTES))

    
    PRUint32 mMaxBlocks;

    
    PRUint32 mFreeListCount;

    
    Block* mFreeList;

    
    
    PRLock *mLock;

    
    nsITimer *mRecycleTimer;

    
    
    
    PRUint32 mRecycleAfter;

    
    
    
    PRBool mTouched;

#ifdef DEBUG
    
    
    
    const char *mId;

    
    PRInt32 mNAllocated;
#endif

 public:

    
    
    
    nsRecyclingAllocator(PRUint32 nbucket = 0, PRUint32 recycleAfter = NS_DEFAULT_RECYCLE_TIMEOUT,
                         const char *id = NULL);
    ~nsRecyclingAllocator();

    nsresult Init(PRUint32 nbucket, PRUint32 recycleAfter, const char *id);

    
    void* Malloc(PRSize size, PRBool zeroit = PR_FALSE);
    void  Free(void *ptr);

    void* Calloc(PRUint32 items, PRSize size)
    {
        return Malloc(items * size, PR_TRUE);
    }

    
    void ClearFreeList();

 protected:

    
    static void nsRecycleTimerCallback(nsITimer *aTimer, void *aClosure);
    friend void nsRecycleTimerCallback(nsITimer *aTimer, void *aClosure);
};






class nsRecyclingAllocatorImpl : public nsRecyclingAllocator, public nsIRecyclingAllocator {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIMEMORY
    NS_DECL_NSIRECYCLINGALLOCATOR

    nsRecyclingAllocatorImpl()
    {
    }

private:
    ~nsRecyclingAllocatorImpl() {}
};
#endif 

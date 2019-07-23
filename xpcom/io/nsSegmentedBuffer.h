




































#ifndef nsSegmentedBuffer_h__
#define nsSegmentedBuffer_h__

#include "nsMemory.h"
#include "prclist.h"

class nsSegmentedBuffer
{
public:
    nsSegmentedBuffer()
        : mSegmentSize(0), mMaxSize(0), 
          mSegAllocator(nsnull), mSegmentArray(nsnull),
          mSegmentArrayCount(0),
          mFirstSegmentIndex(0), mLastSegmentIndex(0) {}

    ~nsSegmentedBuffer() {
        Empty();
        NS_IF_RELEASE(mSegAllocator);
    }


    NS_COM nsresult Init(PRUint32 segmentSize, PRUint32 maxSize,
                  nsIMemory* allocator = nsnull);

    NS_COM char* AppendNewSegment();   

    
    PRBool DeleteFirstSegment();  

    
    PRBool DeleteLastSegment();  

    
    
    PRBool ReallocLastSegment(size_t newSize);

    NS_COM void Empty();               

    inline PRUint32 GetSegmentCount() {
        if (mFirstSegmentIndex <= mLastSegmentIndex)
            return mLastSegmentIndex - mFirstSegmentIndex;
        else 
            return mSegmentArrayCount + mLastSegmentIndex - mFirstSegmentIndex;
    }

    inline PRUint32 GetSegmentSize() { return mSegmentSize; }
    inline PRUint32 GetMaxSize() { return mMaxSize; }
    inline PRUint32 GetSize() { return GetSegmentCount() * mSegmentSize; }

    inline char* GetSegment(PRUint32 indx) {
        NS_ASSERTION(indx < GetSegmentCount(), "index out of bounds");
        PRInt32 i = ModSegArraySize(mFirstSegmentIndex + (PRInt32)indx);
        return mSegmentArray[i];
    }

protected:
    inline PRInt32 ModSegArraySize(PRInt32 n) {
        PRUint32 result = n & (mSegmentArrayCount - 1);
        NS_ASSERTION(result == n % mSegmentArrayCount,
                     "non-power-of-2 mSegmentArrayCount");
        return result;
    }

   inline PRBool IsFull() {
        return ModSegArraySize(mLastSegmentIndex + 1) == mFirstSegmentIndex;
    }

protected:
    PRUint32            mSegmentSize;
    PRUint32            mMaxSize;
    nsIMemory*       mSegAllocator;
    char**              mSegmentArray;
    PRUint32            mSegmentArrayCount;
    PRInt32             mFirstSegmentIndex;
    PRInt32             mLastSegmentIndex;
};









#define NS_SEGMENTARRAY_INITIAL_COUNT 32

#endif 

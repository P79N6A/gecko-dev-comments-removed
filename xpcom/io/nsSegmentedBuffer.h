




#ifndef nsSegmentedBuffer_h__
#define nsSegmentedBuffer_h__

#include "nsMemory.h"
#include "prclist.h"

class nsSegmentedBuffer
{
public:
    nsSegmentedBuffer()
        : mSegmentSize(0), mMaxSize(0), 
          mSegAllocator(nullptr), mSegmentArray(nullptr),
          mSegmentArrayCount(0),
          mFirstSegmentIndex(0), mLastSegmentIndex(0) {}

    ~nsSegmentedBuffer() {
        Empty();
        NS_IF_RELEASE(mSegAllocator);
    }


    nsresult Init(uint32_t segmentSize, uint32_t maxSize,
                  nsIMemory* allocator = nullptr);

    char* AppendNewSegment();   

    
    bool DeleteFirstSegment();  

    
    bool DeleteLastSegment();  

    
    
    bool ReallocLastSegment(size_t newSize);

    void Empty();               

    inline uint32_t GetSegmentCount() {
        if (mFirstSegmentIndex <= mLastSegmentIndex)
            return mLastSegmentIndex - mFirstSegmentIndex;
        else 
            return mSegmentArrayCount + mLastSegmentIndex - mFirstSegmentIndex;
    }

    inline uint32_t GetSegmentSize() { return mSegmentSize; }
    inline uint32_t GetMaxSize() { return mMaxSize; }
    inline uint32_t GetSize() { return GetSegmentCount() * mSegmentSize; }

    inline char* GetSegment(uint32_t indx) {
        NS_ASSERTION(indx < GetSegmentCount(), "index out of bounds");
        int32_t i = ModSegArraySize(mFirstSegmentIndex + (int32_t)indx);
        return mSegmentArray[i];
    }

protected:
    inline int32_t ModSegArraySize(int32_t n) {
        uint32_t result = n & (mSegmentArrayCount - 1);
        NS_ASSERTION(result == n % mSegmentArrayCount,
                     "non-power-of-2 mSegmentArrayCount");
        return result;
    }

   inline bool IsFull() {
        return ModSegArraySize(mLastSegmentIndex + 1) == mFirstSegmentIndex;
    }

protected:
    uint32_t            mSegmentSize;
    uint32_t            mMaxSize;
    nsIMemory*       mSegAllocator;
    char**              mSegmentArray;
    uint32_t            mSegmentArrayCount;
    int32_t             mFirstSegmentIndex;
    int32_t             mLastSegmentIndex;
};









#define NS_SEGMENTARRAY_INITIAL_COUNT 32

#endif 

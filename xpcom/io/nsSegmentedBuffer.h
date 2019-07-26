





#ifndef nsSegmentedBuffer_h__
#define nsSegmentedBuffer_h__

#include "nsIMemory.h"

class nsSegmentedBuffer
{
public:
  nsSegmentedBuffer()
    : mSegmentSize(0)
    , mMaxSize(0)
    , mSegmentArray(nullptr)
    , mSegmentArrayCount(0)
    , mFirstSegmentIndex(0)
    , mLastSegmentIndex(0)
  {
  }

  ~nsSegmentedBuffer()
  {
    Empty();
  }


  nsresult Init(uint32_t aSegmentSize, uint32_t aMaxSize);

  char* AppendNewSegment();   

  
  bool DeleteFirstSegment();  

  
  bool DeleteLastSegment();  

  
  
  bool ReallocLastSegment(size_t aNewSize);

  void Empty();               

  inline uint32_t GetSegmentCount()
  {
    if (mFirstSegmentIndex <= mLastSegmentIndex) {
      return mLastSegmentIndex - mFirstSegmentIndex;
    } else {
      return mSegmentArrayCount + mLastSegmentIndex - mFirstSegmentIndex;
    }
  }

  inline uint32_t GetSegmentSize()
  {
    return mSegmentSize;
  }
  inline uint32_t GetMaxSize()
  {
    return mMaxSize;
  }
  inline uint32_t GetSize()
  {
    return GetSegmentCount() * mSegmentSize;
  }

  inline char* GetSegment(uint32_t aIndex)
  {
    NS_ASSERTION(aIndex < GetSegmentCount(), "index out of bounds");
    int32_t i = ModSegArraySize(mFirstSegmentIndex + (int32_t)aIndex);
    return mSegmentArray[i];
  }

protected:
  inline int32_t ModSegArraySize(int32_t aIndex)
  {
    uint32_t result = aIndex & (mSegmentArrayCount - 1);
    NS_ASSERTION(result == aIndex % mSegmentArrayCount,
                 "non-power-of-2 mSegmentArrayCount");
    return result;
  }

  inline bool IsFull()
  {
    return ModSegArraySize(mLastSegmentIndex + 1) == mFirstSegmentIndex;
  }

protected:
  uint32_t            mSegmentSize;
  uint32_t            mMaxSize;
  char**              mSegmentArray;
  uint32_t            mSegmentArrayCount;
  int32_t             mFirstSegmentIndex;
  int32_t             mLastSegmentIndex;
};









#define NS_SEGMENTARRAY_INITIAL_COUNT 32

#endif 

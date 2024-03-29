





#include "nsSegmentedBuffer.h"
#include "nsMemory.h"

nsresult
nsSegmentedBuffer::Init(uint32_t aSegmentSize, uint32_t aMaxSize)
{
  if (mSegmentArrayCount != 0) {
    return NS_ERROR_FAILURE;  
  }
  mSegmentSize = aSegmentSize;
  mMaxSize = aMaxSize;
#if 0 
  mSegmentArrayCount = 2;
#else
  mSegmentArrayCount = NS_SEGMENTARRAY_INITIAL_COUNT;
#endif
  return NS_OK;
}

char*
nsSegmentedBuffer::AppendNewSegment()
{
  if (GetSize() >= mMaxSize) {
    return nullptr;
  }

  if (!mSegmentArray) {
    uint32_t bytes = mSegmentArrayCount * sizeof(char*);
    mSegmentArray = (char**)moz_xmalloc(bytes);
    if (!mSegmentArray) {
      return nullptr;
    }
    memset(mSegmentArray, 0, bytes);
  }

  if (IsFull()) {
    uint32_t newArraySize = mSegmentArrayCount * 2;
    uint32_t bytes = newArraySize * sizeof(char*);
    char** newSegArray = (char**)moz_xrealloc(mSegmentArray, bytes);
    if (!newSegArray) {
      return nullptr;
    }
    mSegmentArray = newSegArray;
    
    if (mFirstSegmentIndex > mLastSegmentIndex) {
      
      memcpy(&mSegmentArray[mSegmentArrayCount],
             mSegmentArray,
             mLastSegmentIndex * sizeof(char*));
      memset(mSegmentArray, 0, mLastSegmentIndex * sizeof(char*));
      mLastSegmentIndex += mSegmentArrayCount;
      memset(&mSegmentArray[mLastSegmentIndex], 0,
             (newArraySize - mLastSegmentIndex) * sizeof(char*));
    } else {
      memset(&mSegmentArray[mLastSegmentIndex], 0,
             (newArraySize - mLastSegmentIndex) * sizeof(char*));
    }
    mSegmentArrayCount = newArraySize;
  }

  char* seg = (char*)malloc(mSegmentSize);
  if (!seg) {
    return nullptr;
  }
  mSegmentArray[mLastSegmentIndex] = seg;
  mLastSegmentIndex = ModSegArraySize(mLastSegmentIndex + 1);
  return seg;
}

bool
nsSegmentedBuffer::DeleteFirstSegment()
{
  NS_ASSERTION(mSegmentArray[mFirstSegmentIndex] != nullptr, "deleting bad segment");
  free(mSegmentArray[mFirstSegmentIndex]);
  mSegmentArray[mFirstSegmentIndex] = nullptr;
  int32_t last = ModSegArraySize(mLastSegmentIndex - 1);
  if (mFirstSegmentIndex == last) {
    mLastSegmentIndex = last;
    return true;
  } else {
    mFirstSegmentIndex = ModSegArraySize(mFirstSegmentIndex + 1);
    return false;
  }
}

bool
nsSegmentedBuffer::DeleteLastSegment()
{
  int32_t last = ModSegArraySize(mLastSegmentIndex - 1);
  NS_ASSERTION(mSegmentArray[last] != nullptr, "deleting bad segment");
  free(mSegmentArray[last]);
  mSegmentArray[last] = nullptr;
  mLastSegmentIndex = last;
  return (bool)(mLastSegmentIndex == mFirstSegmentIndex);
}

bool
nsSegmentedBuffer::ReallocLastSegment(size_t aNewSize)
{
  int32_t last = ModSegArraySize(mLastSegmentIndex - 1);
  NS_ASSERTION(mSegmentArray[last] != nullptr, "realloc'ing bad segment");
  char* newSegment = (char*)realloc(mSegmentArray[last], aNewSize);
  if (newSegment) {
    mSegmentArray[last] = newSegment;
    return true;
  }
  return false;
}

void
nsSegmentedBuffer::Empty()
{
  if (mSegmentArray) {
    for (uint32_t i = 0; i < mSegmentArrayCount; i++) {
      if (mSegmentArray[i]) {
        free(mSegmentArray[i]);
      }
    }
    free(mSegmentArray);
    mSegmentArray = nullptr;
  }
  mSegmentArrayCount = NS_SEGMENTARRAY_INITIAL_COUNT;
  mFirstSegmentIndex = mLastSegmentIndex = 0;
}

#if 0
void
TestSegmentedBuffer()
{
  nsSegmentedBuffer* buf = new nsSegmentedBuffer();
  NS_ASSERTION(buf, "out of memory");
  buf->Init(4, 16);
  char* seg;
  bool empty;
  seg = buf->AppendNewSegment();
  NS_ASSERTION(seg, "AppendNewSegment failed");
  seg = buf->AppendNewSegment();
  NS_ASSERTION(seg, "AppendNewSegment failed");
  seg = buf->AppendNewSegment();
  NS_ASSERTION(seg, "AppendNewSegment failed");
  empty = buf->DeleteFirstSegment();
  NS_ASSERTION(!empty, "DeleteFirstSegment failed");
  empty = buf->DeleteFirstSegment();
  NS_ASSERTION(!empty, "DeleteFirstSegment failed");
  seg = buf->AppendNewSegment();
  NS_ASSERTION(seg, "AppendNewSegment failed");
  seg = buf->AppendNewSegment();
  NS_ASSERTION(seg, "AppendNewSegment failed");
  seg = buf->AppendNewSegment();
  NS_ASSERTION(seg, "AppendNewSegment failed");
  empty = buf->DeleteFirstSegment();
  NS_ASSERTION(!empty, "DeleteFirstSegment failed");
  empty = buf->DeleteFirstSegment();
  NS_ASSERTION(!empty, "DeleteFirstSegment failed");
  empty = buf->DeleteFirstSegment();
  NS_ASSERTION(!empty, "DeleteFirstSegment failed");
  empty = buf->DeleteFirstSegment();
  NS_ASSERTION(empty, "DeleteFirstSegment failed");
  delete buf;
}
#endif








#if !defined(WebMBufferedParser_h_)
#define WebMBufferedParser_h_

#include "nsISupportsImpl.h"
#include "nsTArray.h"
#include "mozilla/ReentrantMonitor.h"

class TimeRanges;

namespace mozilla {




struct WebMTimeDataOffset
{
  WebMTimeDataOffset(int64_t aOffset, uint64_t aTimecode)
    : mOffset(aOffset), mTimecode(aTimecode)
  {}

  bool operator==(int64_t aOffset) const {
    return mOffset == aOffset;
  }

  bool operator<(int64_t aOffset) const {
    return mOffset < aOffset;
  }

  int64_t mOffset;
  uint64_t mTimecode;
};







struct WebMBufferedParser
{
  WebMBufferedParser(int64_t aOffset)
    : mStartOffset(aOffset), mCurrentOffset(aOffset), mState(CLUSTER_SYNC), mClusterIDPos(0)
  {}

  
  
  
  void Append(const unsigned char* aBuffer, uint32_t aLength,
              nsTArray<WebMTimeDataOffset>& aMapping,
              ReentrantMonitor& aReentrantMonitor);

  bool operator==(int64_t aOffset) const {
    return mCurrentOffset == aOffset;
  }

  bool operator<(int64_t aOffset) const {
    return mCurrentOffset < aOffset;
  }

  
  
  
  int64_t mStartOffset;

  
  
  int64_t mCurrentOffset;

private:
  enum State {
    
    
    
    
    
    CLUSTER_SYNC,

    





    
    
    
    
    
    READ_VINT,

    
    READ_VINT_REST,

    
    
    
    TIMECODE_SYNC,

    
    
    
    READ_CLUSTER_TIMECODE,

    
    
    
    
    
    
    
    ANY_BLOCK_SYNC,

    
    
    
    
    READ_BLOCK,

    
    
    
    
    READ_BLOCK_TIMECODE,

    
    SKIP_DATA,

    
    SKIP_ELEMENT
  };

  
  State mState;

  
  
  State mNextState;

  
  
  uint32_t mClusterIDPos;

  
  uint64_t mVInt;

  
  
  uint32_t mVIntLength;

  
  
  uint32_t mVIntLeft;

  
  
  uint64_t mBlockSize;

  
  uint64_t mClusterTimecode;

  
  
  
  int64_t mBlockOffset;

  
  
  int16_t mBlockTimecode;

  
  uint32_t mBlockTimecodeLength;

  
  
  uint32_t mSkipBytes;
};

class WebMBufferedState
{
  NS_INLINE_DECL_REFCOUNTING(WebMBufferedState)

public:
  WebMBufferedState() : mReentrantMonitor("WebMBufferedState") {
    MOZ_COUNT_CTOR(WebMBufferedState);
  }

  ~WebMBufferedState() {
    MOZ_COUNT_DTOR(WebMBufferedState);
  }

  void NotifyDataArrived(const char* aBuffer, uint32_t aLength, int64_t aOffset);
  bool CalculateBufferedForRange(int64_t aStartOffset, int64_t aEndOffset,
                                 uint64_t* aStartTime, uint64_t* aEndTime);

private:
  
  ReentrantMonitor mReentrantMonitor;

  
  
  nsTArray<WebMTimeDataOffset> mTimeMapping;

  
  nsTArray<WebMBufferedParser> mRangeParsers;
};

} 

#endif

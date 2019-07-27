




#if !defined(WebMBufferedParser_h_)
#define WebMBufferedParser_h_

#include "nsISupportsImpl.h"
#include "nsTArray.h"
#include "mozilla/ReentrantMonitor.h"

namespace mozilla {

namespace dom {
class TimeRanges;
}



struct WebMTimeDataOffset
{
  WebMTimeDataOffset(int64_t aOffset, uint64_t aTimecode, int64_t aSyncOffset)
    : mOffset(aOffset), mSyncOffset(aSyncOffset), mTimecode(aTimecode)
  {}

  bool operator==(int64_t aOffset) const {
    return mOffset == aOffset;
  }

  bool operator<(int64_t aOffset) const {
    return mOffset < aOffset;
  }

  int64_t mOffset;
  int64_t mSyncOffset;
  uint64_t mTimecode;
};







struct WebMBufferedParser
{
  explicit WebMBufferedParser(int64_t aOffset)
    : mStartOffset(aOffset), mCurrentOffset(aOffset), mState(READ_ELEMENT_ID),
      mVIntRaw(false), mTimecodeScale(1000000), mGotTimecodeScale(false)
  {}

  uint32_t GetTimecodeScale() {
    MOZ_ASSERT(mGotTimecodeScale);
    return mTimecodeScale;
  }

  
  
  void SetTimecodeScale(uint32_t aTimecodeScale) {
    mTimecodeScale = aTimecodeScale;
    mGotTimecodeScale = true;
  }

  
  
  
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
    
    
    READ_ELEMENT_ID,

    
    
    READ_ELEMENT_SIZE,

    
    
    
    PARSE_ELEMENT,

    
    
    
    
    
    READ_VINT,

    
    READ_VINT_REST,

    
    
    READ_TIMECODESCALE,

    
    
    READ_CLUSTER_TIMECODE,

    
    
    
    
    
    
    READ_BLOCK_TIMECODE,

    
    SKIP_DATA,
  };

  
  State mState;

  
  
  State mNextState;

  struct VInt {
    VInt() : mValue(0), mLength(0) {}
    uint64_t mValue;
    uint64_t mLength;
  };

  struct EBMLElement {
    uint64_t Length() { return mID.mLength + mSize.mLength; }
    VInt mID;
    VInt mSize;
  };

  EBMLElement mElement;

  VInt mVInt;

  bool mVIntRaw;

  
  
  uint32_t mVIntLeft;

  
  
  uint64_t mBlockSize;

  
  uint64_t mClusterTimecode;

  
  
  
  int64_t mClusterOffset;

  
  
  
  int64_t mBlockOffset;

  
  
  int16_t mBlockTimecode;

  
  uint32_t mBlockTimecodeLength;

  
  
  uint32_t mSkipBytes;

  
  
  uint32_t mTimecodeScale;

  
  
  bool mGotTimecodeScale;
};

class WebMBufferedState MOZ_FINAL
{
  NS_INLINE_DECL_REFCOUNTING(WebMBufferedState)

public:
  WebMBufferedState() : mReentrantMonitor("WebMBufferedState") {
    MOZ_COUNT_CTOR(WebMBufferedState);
  }

  void NotifyDataArrived(const char* aBuffer, uint32_t aLength, int64_t aOffset);
  bool CalculateBufferedForRange(int64_t aStartOffset, int64_t aEndOffset,
                                 uint64_t* aStartTime, uint64_t* aEndTime);
  enum OffsetType {
    CLUSTER_START,
    BLOCK_START
  };
  bool GetOffsetForTime(uint64_t aTime, int64_t* aOffset, enum OffsetType aType);

private:
  
  ~WebMBufferedState() {
    MOZ_COUNT_DTOR(WebMBufferedState);
  }

  
  ReentrantMonitor mReentrantMonitor;

  
  
  nsTArray<WebMTimeDataOffset> mTimeMapping;

  
  nsTArray<WebMBufferedParser> mRangeParsers;
};

} 

#endif

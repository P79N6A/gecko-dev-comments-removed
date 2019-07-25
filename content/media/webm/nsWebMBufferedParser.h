




































#if !defined(nsWebMBufferedParser_h_)
#define nsWebMBufferedParser_h_

#include "nsTArray.h"




struct nsWebMTimeDataOffset
{
  nsWebMTimeDataOffset(PRInt64 aOffset, PRUint64 aTimecode)
    : mOffset(aOffset), mTimecode(aTimecode)
  {}

  bool operator==(PRInt64 aOffset) const {
    return mOffset == aOffset;
  }

  bool operator<(PRInt64 aOffset) const {
    return mOffset < aOffset;
  }

  PRInt64 mOffset;
  PRUint64 mTimecode;
};







struct nsWebMBufferedParser
{
  nsWebMBufferedParser(PRInt64 aOffset)
    : mStartOffset(aOffset), mCurrentOffset(aOffset), mState(CLUSTER_SYNC), mClusterIDPos(0)
  {}

  
  
  void Append(const unsigned char* aBuffer, PRUint32 aLength,
              nsTArray<nsWebMTimeDataOffset>& aMapping);

  bool operator==(PRInt64 aOffset) const {
    return mCurrentOffset == aOffset;
  }

  bool operator<(PRInt64 aOffset) const {
    return mCurrentOffset < aOffset;
  }

  
  
  
  PRInt64 mStartOffset;

  
  
  PRInt64 mCurrentOffset;

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

  
  
  PRUint32 mClusterIDPos;

  
  PRUint64 mVInt;

  
  
  PRUint32 mVIntLength;

  
  
  PRUint32 mVIntLeft;

  
  
  PRUint64 mBlockSize;

  
  PRUint64 mClusterTimecode;

  
  
  
  PRInt64 mBlockOffset;

  
  
  PRInt16 mBlockTimecode;

  
  PRUint32 mBlockTimecodeLength;

  
  
  PRUint32 mSkipBytes;
};

#endif

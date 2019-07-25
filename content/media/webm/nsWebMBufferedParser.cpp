





































#include "nsAlgorithm.h"
#include "nsWebMBufferedParser.h"
#include "nsTimeRanges.h"
#include "nsThreadUtils.h"

using mozilla::ReentrantMonitorAutoEnter;

static const double NS_PER_S = 1e9;

static PRUint32
VIntLength(unsigned char aFirstByte, PRUint32* aMask)
{
  PRUint32 count = 1;
  PRUint32 mask = 1 << 7;
  while (count < 8) {
    if ((aFirstByte & mask) != 0) {
      break;
    }
    mask >>= 1;
    count += 1;
  }
  if (aMask) {
    *aMask = mask;
  }
  NS_ASSERTION(count >= 1 && count <= 8, "Insane VInt length.");
  return count;
}

void nsWebMBufferedParser::Append(const unsigned char* aBuffer, PRUint32 aLength,
                                  nsTArray<nsWebMTimeDataOffset>& aMapping,
                                  ReentrantMonitor& aReentrantMonitor)
{
  static const unsigned char CLUSTER_ID[] = { 0x1f, 0x43, 0xb6, 0x75 };
  static const unsigned char TIMECODE_ID = 0xe7;
  static const unsigned char BLOCKGROUP_ID = 0xa0;
  static const unsigned char BLOCK_ID = 0xa1;
  static const unsigned char SIMPLEBLOCK_ID = 0xa3;

  const unsigned char* p = aBuffer;

  
  
  
  
  while (p < aBuffer + aLength) {
    switch (mState) {
    case CLUSTER_SYNC:
      if (*p++ == CLUSTER_ID[mClusterIDPos]) {
        mClusterIDPos += 1;
      } else {
        mClusterIDPos = 0;
      }
      
      
      
      if (mClusterIDPos == sizeof(CLUSTER_ID)) {
        mClusterIDPos = 0;
        mState = READ_VINT;
        mNextState = TIMECODE_SYNC;
      }
      break;
    case READ_VINT: {
      unsigned char c = *p++;
      PRUint32 mask;
      mVIntLength = VIntLength(c, &mask);
      mVIntLeft = mVIntLength - 1;
      mVInt = c & ~mask;
      mState = READ_VINT_REST;
      break;
    }
    case READ_VINT_REST:
      if (mVIntLeft) {
        mVInt <<= 8;
        mVInt |= *p++;
        mVIntLeft -= 1;
      } else {
        mState = mNextState;
      }
      break;
    case TIMECODE_SYNC:
      if (*p++ != TIMECODE_ID) {
        p -= 1;
        mState = CLUSTER_SYNC;
        break;
      }
      mClusterTimecode = 0;
      mState = READ_VINT;
      mNextState = READ_CLUSTER_TIMECODE;
      break;
    case READ_CLUSTER_TIMECODE:
      if (mVInt) {
        mClusterTimecode <<= 8;
        mClusterTimecode |= *p++;
        mVInt -= 1;
      } else {
        mState = ANY_BLOCK_SYNC;
      }
      break;
    case ANY_BLOCK_SYNC: {
      unsigned char c = *p++;
      if (c == BLOCKGROUP_ID) {
        mState = READ_VINT;
        mNextState = ANY_BLOCK_SYNC;
      } else if (c == SIMPLEBLOCK_ID || c == BLOCK_ID) {
        mBlockOffset = mCurrentOffset + (p - aBuffer) - 1;
        mState = READ_VINT;
        mNextState = READ_BLOCK;
      } else {
        PRUint32 length = VIntLength(c, nsnull);
        if (length == 4) {
          p -= 1;
          mState = CLUSTER_SYNC;
        } else {
          mState = READ_VINT;
          mNextState = SKIP_ELEMENT;
        }
      }
      break;
    }
    case READ_BLOCK:
      mBlockSize = mVInt;
      mBlockTimecode = 0;
      mBlockTimecodeLength = 2;
      mState = READ_VINT;
      mNextState = READ_BLOCK_TIMECODE;
      break;
    case READ_BLOCK_TIMECODE:
      if (mBlockTimecodeLength) {
        mBlockTimecode <<= 8;
        mBlockTimecode |= *p++;
        mBlockTimecodeLength -= 1;
      } else {
        
        
        {
          ReentrantMonitorAutoEnter mon(aReentrantMonitor);
          PRUint32 idx;
          if (!aMapping.GreatestIndexLtEq(mBlockOffset, idx)) {
            nsWebMTimeDataOffset entry(mBlockOffset, mClusterTimecode + mBlockTimecode);
            aMapping.InsertElementAt(idx, entry);
          }
        }

        
        mBlockSize -= mVIntLength;
        mBlockSize -= 2;
        mSkipBytes = PRUint32(mBlockSize);
        mState = SKIP_DATA;
        mNextState = ANY_BLOCK_SYNC;
      }
      break;
    case SKIP_DATA:
      if (mSkipBytes) {
        PRUint32 left = aLength - (p - aBuffer);
        left = NS_MIN(left, mSkipBytes);
        p += left;
        mSkipBytes -= left;
      } else {
        mState = mNextState;
      }
      break;
    case SKIP_ELEMENT:
      mSkipBytes = PRUint32(mVInt);
      mState = SKIP_DATA;
      mNextState = ANY_BLOCK_SYNC;
      break;
    }
  }

  NS_ASSERTION(p == aBuffer + aLength, "Must have parsed to end of data.");
  mCurrentOffset += aLength;
}

void nsWebMBufferedState::CalculateBufferedForRange(nsTimeRanges* aBuffered,
                                                    PRInt64 aStartOffset, PRInt64 aEndOffset,
                                                    PRUint64 aTimecodeScale,
                                                    PRInt64 aStartTimeOffsetNS)
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

  
  PRUint32 start;
  mTimeMapping.GreatestIndexLtEq(aStartOffset, start);
  if (start == mTimeMapping.Length()) {
    return;
  }

  
  PRUint32 end;
  if (!mTimeMapping.GreatestIndexLtEq(aEndOffset, end) && end > 0) {
    
    
    end -= 1;
  }

  
  if (end <= start) {
    return;
  }

  NS_ASSERTION(mTimeMapping[start].mOffset >= aStartOffset &&
               mTimeMapping[end].mOffset <= aEndOffset,
               "Computed time range must lie within data range.");
  if (start > 0) {
    NS_ASSERTION(mTimeMapping[start - 1].mOffset <= aStartOffset,
                 "Must have found least nsWebMTimeDataOffset for start");
  }
  if (end < mTimeMapping.Length() - 1) {
    NS_ASSERTION(mTimeMapping[end + 1].mOffset >= aEndOffset,
                 "Must have found greatest nsWebMTimeDataOffset for end");
  }

  
  
  

  double startTime = (mTimeMapping[start].mTimecode * aTimecodeScale - aStartTimeOffsetNS) / NS_PER_S;
  double endTime = (mTimeMapping[end].mTimecode * aTimecodeScale - aStartTimeOffsetNS) / NS_PER_S;
  aBuffered->Add(startTime, endTime);
}

void nsWebMBufferedState::NotifyDataArrived(const char* aBuffer, PRUint32 aLength, PRUint32 aOffset)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  PRUint32 idx;
  if (!mRangeParsers.GreatestIndexLtEq(aOffset, idx)) {
    
    
    
    
    
    if (idx != mRangeParsers.Length() && mRangeParsers[idx].mStartOffset <= aOffset) {
      
      if (aOffset + aLength <= mRangeParsers[idx].mCurrentOffset) {
        return;
      }

      
      PRInt64 adjust = mRangeParsers[idx].mCurrentOffset - aOffset;
      NS_ASSERTION(adjust >= 0, "Overlap detection bug.");
      aBuffer += adjust;
      aLength -= PRUint32(adjust);
    } else {
      mRangeParsers.InsertElementAt(idx, nsWebMBufferedParser(aOffset));
    }
  }

  mRangeParsers[idx].Append(reinterpret_cast<const unsigned char*>(aBuffer),
                            aLength,
                            mTimeMapping,
                            mReentrantMonitor);

  
  PRUint32 i = 0;
  while (i + 1 < mRangeParsers.Length()) {
    if (mRangeParsers[i].mCurrentOffset >= mRangeParsers[i + 1].mStartOffset) {
      mRangeParsers[i + 1].mStartOffset = mRangeParsers[i].mStartOffset;
      mRangeParsers.RemoveElementAt(i);
    } else {
      i += 1;
    }
  }
}

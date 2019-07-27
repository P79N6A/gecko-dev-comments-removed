





#include "nsAlgorithm.h"
#include "WebMBufferedParser.h"
#include "mozilla/dom/TimeRanges.h"
#include "nsThreadUtils.h"
#include <algorithm>

namespace mozilla {

static uint32_t
VIntLength(unsigned char aFirstByte, uint32_t* aMask)
{
  uint32_t count = 1;
  uint32_t mask = 1 << 7;
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

void WebMBufferedParser::Append(const unsigned char* aBuffer, uint32_t aLength,
                                nsTArray<WebMTimeDataOffset>& aMapping,
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
        mClusterOffset = mCurrentOffset + (p - aBuffer) - sizeof(CLUSTER_ID);
        mState = READ_VINT;
        mNextState = TIMECODE_SYNC;
      }
      break;
    case READ_VINT: {
      unsigned char c = *p++;
      uint32_t mask;
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
        uint32_t length = VIntLength(c, nullptr);
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
          uint32_t idx = aMapping.IndexOfFirstElementGt(mBlockOffset);
          if (idx == 0 || !(aMapping[idx - 1] == mBlockOffset)) {
            
            if (mBlockOffset > 0 || mClusterTimecode > uint16_t(abs(mBlockOffset))) {
              WebMTimeDataOffset entry(mBlockOffset,
                                       mClusterTimecode + mBlockTimecode,
                                       mClusterOffset);
              aMapping.InsertElementAt(idx, entry);
            }
          }
        }

        
        mBlockSize -= mVIntLength;
        mBlockSize -= 2;
        mSkipBytes = uint32_t(mBlockSize);
        mState = SKIP_DATA;
        mNextState = ANY_BLOCK_SYNC;
      }
      break;
    case SKIP_DATA:
      if (mSkipBytes) {
        uint32_t left = aLength - (p - aBuffer);
        left = std::min(left, mSkipBytes);
        p += left;
        mSkipBytes -= left;
      } else {
        mState = mNextState;
      }
      break;
    case SKIP_ELEMENT:
      mSkipBytes = uint32_t(mVInt);
      mState = SKIP_DATA;
      mNextState = ANY_BLOCK_SYNC;
      break;
    }
  }

  NS_ASSERTION(p == aBuffer + aLength, "Must have parsed to end of data.");
  mCurrentOffset += aLength;
}

bool WebMBufferedState::CalculateBufferedForRange(int64_t aStartOffset, int64_t aEndOffset,
                                                  uint64_t* aStartTime, uint64_t* aEndTime)
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

  
  uint32_t start = mTimeMapping.IndexOfFirstElementGt(aStartOffset - 1);
  if (start == mTimeMapping.Length()) {
    return false;
  }

  
  uint32_t end = mTimeMapping.IndexOfFirstElementGt(aEndOffset - 1);
  if (end > 0) {
    end -= 1;
  }

  
  if (end <= start) {
    return false;
  }

  NS_ASSERTION(mTimeMapping[start].mOffset >= aStartOffset &&
               mTimeMapping[end].mOffset <= aEndOffset,
               "Computed time range must lie within data range.");
  if (start > 0) {
    NS_ASSERTION(mTimeMapping[start - 1].mOffset <= aStartOffset,
                 "Must have found least WebMTimeDataOffset for start");
  }
  if (end < mTimeMapping.Length() - 1) {
    NS_ASSERTION(mTimeMapping[end + 1].mOffset >= aEndOffset,
                 "Must have found greatest WebMTimeDataOffset for end");
  }

  
  
  

  *aStartTime = mTimeMapping[start].mTimecode;
  *aEndTime = mTimeMapping[end].mTimecode;
  *aEndTime += mTimeMapping[end].mTimecode - mTimeMapping[end - 1].mTimecode;
  return true;
}

bool WebMBufferedState::GetOffsetForTime(uint64_t aTime, int64_t* aOffset, enum OffsetType aType)
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);
  WebMTimeDataOffset result(0, 0, 0);

  for (uint32_t i = 0; i < mTimeMapping.Length(); ++i) {
    WebMTimeDataOffset o = mTimeMapping[i];
    if (o.mTimecode < aTime && o.mTimecode > result.mTimecode) {
      result = o;
    }
  }

  *aOffset = aType == CLUSTER_START ? result.mSyncOffset : result.mOffset;
  return true;
}

void WebMBufferedState::NotifyDataArrived(const char* aBuffer, uint32_t aLength, int64_t aOffset)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  uint32_t idx = mRangeParsers.IndexOfFirstElementGt(aOffset - 1);
  if (idx == 0 || !(mRangeParsers[idx-1] == aOffset)) {
    
    
    
    
    
    if (idx != mRangeParsers.Length() && mRangeParsers[idx].mStartOffset <= aOffset) {
      
      if (aOffset + aLength <= mRangeParsers[idx].mCurrentOffset) {
        return;
      }

      
      int64_t adjust = mRangeParsers[idx].mCurrentOffset - aOffset;
      NS_ASSERTION(adjust >= 0, "Overlap detection bug.");
      aBuffer += adjust;
      aLength -= uint32_t(adjust);
    } else {
      mRangeParsers.InsertElementAt(idx, WebMBufferedParser(aOffset));
    }
  }

  mRangeParsers[idx].Append(reinterpret_cast<const unsigned char*>(aBuffer),
                            aLength,
                            mTimeMapping,
                            mReentrantMonitor);

  
  uint32_t i = 0;
  while (i + 1 < mRangeParsers.Length()) {
    if (mRangeParsers[i].mCurrentOffset >= mRangeParsers[i + 1].mStartOffset) {
      mRangeParsers[i + 1].mStartOffset = mRangeParsers[i].mStartOffset;
      mRangeParsers.RemoveElementAt(i);
    } else {
      i += 1;
    }
  }
}

} 


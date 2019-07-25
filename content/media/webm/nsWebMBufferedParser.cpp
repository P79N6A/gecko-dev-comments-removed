





































#include "nsAlgorithm.h"
#include "nsWebMBufferedParser.h"

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
                                  nsTArray<nsWebMTimeDataOffset>& aMapping)
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
        
        
        PRUint32 idx;
        if (!aMapping.GreatestIndexLtEq(mBlockOffset, idx)) {
          nsWebMTimeDataOffset entry(mBlockOffset, mClusterTimecode + mBlockTimecode);
          aMapping.InsertElementAt(idx, entry);
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

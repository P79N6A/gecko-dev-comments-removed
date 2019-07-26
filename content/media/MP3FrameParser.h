





#include <stdint.h>
#include "mozilla/Mutex.h"

namespace mozilla {



























class MP3FrameParser
{
public:
  MP3FrameParser(int64_t aLength=-1);

  bool IsMP3() {
    MutexAutoLock mon(mLock);
    return mIsMP3 != NOT_MP3;
  }

  void Parse(const char* aBuffer, uint32_t aLength, int64_t aStreamOffset);

  
  
  
  int64_t GetDuration();

  
  
  int64_t GetMP3Offset();

  
  uint64_t GetLastStreamOffset() {
    return mOffset + mBufferLength;
  }

private:

  
  
  
  
  
  nsresult ParseBuffer(const uint8_t* aBuffer,
                       uint32_t aLength,
                       int64_t aStreamOffset,
                       uint32_t* aOutBytesRead);

  
  
  uint8_t  mBuffer[32];
  uint32_t mBufferLength;

  
  Mutex mLock;

  
  uint64_t mDurationUs;
  uint64_t mBitRateSum;
  uint64_t mTotalFrameSize;
  uint64_t mNumFrames;

  
  
  
  int64_t  mOffset;

  
  int64_t  mLength;

  
  
  int64_t mMP3Offset;

  
  
  
  uint32_t mSkippedBytes;

  
  uint16_t mSampleRate;

  enum eIsMP3 {
    MAYBE_MP3, 
    DEFINITELY_MP3, 
    NOT_MP3 
  };

  eIsMP3 mIsMP3;

};

}

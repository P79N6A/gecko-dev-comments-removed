





#include <stdint.h>
#include "mozilla/Mutex.h"

namespace mozilla {



























class MP3FrameParser
{
public:
  MP3FrameParser(int64_t aLength=-1);

  bool IsMP3() {
    MutexAutoLock mon(mLock);
    return mIsMP3;
  }

  void Parse(const uint8_t* aBuffer, uint32_t aLength, int64_t aOffset);

  void NotifyDataArrived(const char* aBuffer, uint32_t aLength, int64_t aOffset);

  int64_t GetDuration();

private:
  size_t ParseInternalBuffer(const uint8_t* aBuffer, uint32_t aLength, int64_t aOffset);

  uint8_t  mBuffer[10];
  uint32_t mBufferLength;

  
  Mutex mLock;

  
  uint64_t mDurationUs;
  uint64_t mBitRateSum;
  uint64_t mNumFrames;
  int64_t  mOffset;
  int64_t  mUnhandled;
  int64_t  mLength;
  uint32_t mTrailing;

  
  bool mIsMP3;
};

}

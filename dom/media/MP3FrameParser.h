





#ifndef MP3FrameParser_h
#define MP3FrameParser_h

#include <stdint.h>

#include "mozilla/Mutex.h"
#include "nsString.h"

namespace mozilla {




class ID3Parser
{
public:
  ID3Parser();

  void Reset();
  bool ParseChar(char ch);
  bool IsParsed() const;
  uint32_t GetHeaderLength() const;

private:
  uint32_t mCurrentChar;
  uint8_t mVersion;
  uint8_t mFlags;
  uint32_t mHeaderLength;
};

struct MP3Frame {
  uint16_t mSync1 : 8;      
  uint16_t mProtected : 1;  
  uint16_t mLayer : 2;
  uint16_t mVersion : 2;
  uint16_t mSync2 : 3;      
  uint16_t mPrivate : 1;    
  uint16_t mPad : 1;
  uint16_t mSampleRate : 2; 
  uint16_t mBitrate : 4;    

  uint16_t CalculateLength();
};


class MP3Parser
{
public:
  MP3Parser();

  
  void Reset();

  
  
  uint16_t ParseFrameLength(uint8_t ch);

  
  uint32_t GetSampleRate();

  
  uint32_t GetSamplesPerFrame();

private:
  uint32_t mCurrentChar;
  union {
    uint8_t mRaw[3];
    MP3Frame mFrame;
  } mData;
};




























class MP3FrameParser
{
public:
  explicit MP3FrameParser(int64_t aLength=-1);

  bool IsMP3() {
    MutexAutoLock mon(mLock);
    return mIsMP3 != NOT_MP3;
  }

  void Parse(const char* aBuffer, uint32_t aLength, uint64_t aStreamOffset);

  
  
  
  int64_t GetDuration();

  
  
  int64_t GetMP3Offset();

  
  
  
  bool ParsedHeaders();

  
  
  bool HasExactDuration();

  
  bool NeedsData();
  
  void SetLength(int64_t aLength) {
    MutexAutoLock mon(mLock);
    mLength = aLength;
  }
private:

  
  
  
  
  
  nsresult ParseBuffer(const uint8_t* aBuffer,
                       uint32_t aLength,
                       int64_t aStreamOffset,
                       uint32_t* aOutBytesRead);

  
  Mutex mLock;

  
  
  ID3Parser mID3Parser;

  
  MP3Parser mMP3Parser;

  
  
  
  
  uint32_t mTotalID3Size;

  

  
  
  uint64_t mTotalFrameSize;
  uint64_t mFrameCount;

  
  
  
  uint64_t mOffset;

  
  int64_t mLength;

  
  
  int64_t mMP3Offset;

  
  int64_t mNumFrames;

  
  
  
  uint16_t mSamplesPerSecond;
  uint16_t mSamplesPerFrame;

  
  
  nsAutoCString mFirstFrame;

  
  
  int64_t mFirstFrameEnd;

  enum eIsMP3 {
    MAYBE_MP3, 
    DEFINITELY_MP3, 
    NOT_MP3 
  };

  eIsMP3 mIsMP3;

};

}

#endif

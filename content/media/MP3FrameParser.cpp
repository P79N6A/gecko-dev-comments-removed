





#include <algorithm>
#include "nsMemory.h"
#include "MP3FrameParser.h"
#include "VideoUtils.h"


namespace mozilla {









const uint8_t mpeg_versions[4] = { 25, 0, 2, 1 };


const uint8_t mpeg_layers[4] = { 0, 3, 2, 1 };


const uint16_t mpeg_bitrates[4][4][16] = {
  { 
    { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 0 }, 
    { 0,   8,  16,  24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160, 0 }, 
    { 0,   8,  16,  24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160, 0 }, 
    { 0,  32,  48,  56,  64,  80,  96, 112, 128, 144, 160, 176, 192, 224, 256, 0 }  
  },
  { 
    { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 0 }, 
    { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 0 }, 
    { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 0 }, 
    { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 0 }  
  },
  { 
    { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 0 }, 
    { 0,   8,  16,  24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160, 0 }, 
    { 0,   8,  16,  24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160, 0 }, 
    { 0,  32,  48,  56,  64,  80,  96, 112, 128, 144, 160, 176, 192, 224, 256, 0 }  
  },
  { 
    { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 0 }, 
    { 0,  32,  40,  48,  56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 0 }, 
    { 0,  32,  48,  56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 384, 0 }, 
    { 0,  32,  64,  96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0 }, 
  }
};


const uint16_t mpeg_srates[4][4] = {
    { 11025, 12000,  8000, 0 }, 
    {     0,     0,     0, 0 }, 
    { 22050, 24000, 16000, 0 }, 
    { 44100, 48000, 32000, 0 }  
};


const uint16_t mpeg_frame_samples[4][4] = {

    {    0,  576, 1152,  384 }, 
    {    0,    0,    0,    0 }, 
    {    0,  576, 1152,  384 }, 
    {    0, 1152, 1152,  384 }  
};


const uint8_t mpeg_slot_size[4] = { 0, 1, 1, 4 }; 

uint16_t
MP3Frame::CalculateLength()
{
  
  uint32_t  bitrate   = mpeg_bitrates[mVersion][mLayer][mBitrate] * 1000;
  uint32_t  samprate  = mpeg_srates[mVersion][mSampleRate];
  uint16_t  samples   = mpeg_frame_samples[mVersion][mLayer];
  uint8_t   slot_size = mpeg_slot_size[mLayer];

  
  float     bps       = (float)samples / 8.0;
  float     fsize     = ( (bps * (float)bitrate) / (float)samprate )
    + ( (mPad) ? slot_size : 0 );

  
  return (uint16_t)fsize;
}






MP3Parser::MP3Parser()
  : mCurrentChar(0)
{ }

void
MP3Parser::Reset()
{
  mCurrentChar = 0;
}

uint16_t
MP3Parser::ParseFrameLength(uint8_t ch)
{
  mData.mRaw[mCurrentChar] = ch;

  MP3Frame &frame = mData.mFrame;

  
  
  

  
  
  if (ch == 0xff) {
    mCurrentChar = 0;
  }

  
  if (mCurrentChar == 2) {
    if (frame.mBitrate == 0x0f) {
      goto fail;
    }
  } else if (mCurrentChar == 1) {
    if (frame.mSync2 != 0x07
        || frame.mVersion == 0x01
        || frame.mLayer == 0x00) {
      goto fail;
    }
  }

  
  
  if (mCurrentChar == 0 && frame.mSync1 != 0xff) {
    
    return 0;
  }

  mCurrentChar++;
  MOZ_ASSERT(mCurrentChar <= sizeof(MP3Frame));

  
  if (mCurrentChar < sizeof(MP3Frame)) {
    return 0;
  }

  
  mCurrentChar = 0;
  return frame.CalculateLength();

fail:
  Reset();
  return 0;
}

uint32_t
MP3Parser::GetSampleRate()
{
  MP3Frame &frame = mData.mFrame;
  return mpeg_srates[frame.mVersion][frame.mSampleRate];
}




const char sID3Head[3] = { 'I', 'D', '3' };
const uint32_t ID3_HEADER_LENGTH = 10;

ID3Parser::ID3Parser()
  : mCurrentChar(0)
  , mHeaderLength(0)
{ }

void
ID3Parser::Reset()
{
  mCurrentChar = mHeaderLength = 0;
}

bool
ID3Parser::ParseChar(char ch)
{
  
  if (mCurrentChar < sizeof(sID3Head) / sizeof(*sID3Head)
      && ch != sID3Head[mCurrentChar]) {
    goto fail;
  }

  
  
  if (mCurrentChar >= 6 && mCurrentChar < ID3_HEADER_LENGTH) {
    if (ch & 0x80) {
      goto fail;
    } else {
      mHeaderLength <<= 7;
      mHeaderLength |= ch;
    }
  }

  mCurrentChar++;

  return IsParsed();

fail:
  Reset();
  return false;
}

bool
ID3Parser::IsParsed() const
{
  return mCurrentChar >= ID3_HEADER_LENGTH;
}

uint32_t
ID3Parser::GetHeaderLength() const
{
  MOZ_ASSERT(IsParsed(),
             "Queried length of ID3 header before parsing finished.");
  return mHeaderLength;
}







static const uint32_t MAX_SKIPPED_BYTES = 200 * 1024;





static const uint32_t SAMPLES_PER_FRAME = 1152;

enum {
  MP3_HEADER_LENGTH   = 4,
};

MP3FrameParser::MP3FrameParser(int64_t aLength)
: mLock("MP3FrameParser.mLock"),
  mTotalFrameSize(0),
  mNumFrames(0),
  mOffset(0),
  mLength(aLength),
  mMP3Offset(-1),
  mSampleRate(0),
  mIsMP3(MAYBE_MP3)
{ }

nsresult MP3FrameParser::ParseBuffer(const uint8_t* aBuffer,
                                     uint32_t aLength,
                                     int64_t aStreamOffset,
                                     uint32_t* aOutBytesRead)
{
  
  

  const uint8_t *buffer = aBuffer;
  const uint8_t *bufferEnd = aBuffer + aLength;

  
  
  if (mMP3Offset < 0) {
    for (const uint8_t *ch = buffer; ch < bufferEnd; ch++) {
      if (mID3Parser.ParseChar(*ch)) {
        
        
        buffer = ch + mID3Parser.GetHeaderLength() - (ID3_HEADER_LENGTH - 1);
        ch = buffer;

        
        mIsMP3 = DEFINITELY_MP3;

        mID3Parser.Reset();
      }
    }
  }

  while (buffer < bufferEnd) {
    uint16_t frameLen = mMP3Parser.ParseFrameLength(*buffer);

    if (frameLen) {

      if (mMP3Offset < 0) {
        
        
        mIsMP3 = DEFINITELY_MP3;
        
        
        mMP3Offset = aStreamOffset
          + (buffer - aBuffer)
          - (sizeof(MP3Frame) - 1);
      }

      mSampleRate = mMP3Parser.GetSampleRate();
      mTotalFrameSize += frameLen;
      mNumFrames++;

      buffer += frameLen - sizeof(MP3Frame);
    } else {
      buffer++;
    }
  }

  *aOutBytesRead = buffer - aBuffer;
  return NS_OK;
}

void MP3FrameParser::Parse(const char* aBuffer, uint32_t aLength, int64_t aOffset)
{
  MutexAutoLock mon(mLock);

  const uint8_t* buffer = reinterpret_cast<const uint8_t*>(aBuffer);
  int32_t length = aLength;
  int64_t offset = aOffset;

  
  if (aOffset < mOffset) {
    buffer += mOffset - aOffset;
    length -= mOffset - aOffset;
    offset = mOffset;

    if (length <= 0) {
      return;
    }
  }

  
  
  if (mOffset < aOffset) {
    if (!mID3Parser.IsParsed()) {
      
      mID3Parser.Reset();
    }
    mMP3Parser.Reset();
  }

  uint32_t bytesRead = 0;
  if (NS_FAILED(ParseBuffer(buffer,
                            length,
                            offset,
                            &bytesRead))) {
    return;
  }

  MOZ_ASSERT(length <= (int)bytesRead, "All bytes should have been consumed");

  
  mOffset = offset + bytesRead;

  
  if (!mID3Parser.IsParsed() && !mNumFrames && mOffset > MAX_SKIPPED_BYTES) {
    mIsMP3 = NOT_MP3;
  }
}

int64_t MP3FrameParser::GetDuration()
{
  MutexAutoLock mon(mLock);

  if (!mNumFrames) {
    return -1; 
  }

  
  
  double avgFrameSize = (double)mTotalFrameSize / mNumFrames;

  
  
  double estimatedFrames = (double)(mLength - mMP3Offset) / avgFrameSize;

  
  double usPerFrame = USECS_PER_S * SAMPLES_PER_FRAME / mSampleRate;

  return estimatedFrames * usPerFrame;
}

int64_t MP3FrameParser::GetMP3Offset()
{
  MutexAutoLock mon(mLock);
  return mMP3Offset;
}

}

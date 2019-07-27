





#include <algorithm>

#include "nsMemory.h"
#include "MP3FrameParser.h"
#include "VideoUtils.h"


#define FROM_BIG_ENDIAN(X) ((uint32_t)((uint8_t)(X)[0] << 24 | (uint8_t)(X)[1] << 16 | \
                                       (uint8_t)(X)[2] << 8 | (uint8_t)(X)[3]))


namespace mozilla {









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

uint32_t
MP3Parser::GetSamplesPerFrame()
{
  MP3Frame &frame = mData.mFrame;
  return mpeg_frame_samples[frame.mVersion][frame.mLayer];
}




const char sID3Head[3] = { 'I', 'D', '3' };
const uint32_t ID3_HEADER_LENGTH = 10;
const uint32_t ID3_FOOTER_LENGTH = 10;
const uint8_t ID3_FOOTER_PRESENT = 0x10;

ID3Parser::ID3Parser()
  : mCurrentChar(0)
  , mVersion(0)
  , mFlags(0)
  , mHeaderLength(0)
{ }

void
ID3Parser::Reset()
{
  mCurrentChar = mVersion = mFlags = mHeaderLength = 0;
}

bool
ID3Parser::ParseChar(char ch)
{
  switch (mCurrentChar) {
    
    case 0: case 1: case 2:
      if (ch != sID3Head[mCurrentChar]) {
        goto fail;
      }
      break;
    
    case 3:
      if (ch < '\2' || ch > '\4') {
        goto fail;
      }
      mVersion = uint8_t(ch);
      break;
    case 4:
      if (ch != '\0') {
        goto fail;
      }
      break;
    
    case 5:
      if ((ch & (0xff >> mVersion)) != '\0') {
        goto fail;
      }
      mFlags = uint8_t(ch);
      break;
    
    
    
    case 6: case 7: case 8: case 9:
      if (ch & 0x80) {
        goto fail;
      }
      mHeaderLength <<= 7;
      mHeaderLength |= ch;
      if (mCurrentChar == 9) {
        mHeaderLength += ID3_HEADER_LENGTH;
        mHeaderLength += (mFlags & ID3_FOOTER_PRESENT) ? ID3_FOOTER_LENGTH : 0;
      }
      break;
    default:
      MOZ_CRASH("Header already fully parsed!");
  }

  mCurrentChar++;

  return IsParsed();

fail:
  if (mCurrentChar) {
    Reset();
    return ParseChar(ch);
  }
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








const uint32_t VBRI_TAG = FROM_BIG_ENDIAN("VBRI");
const uint32_t VBRI_OFFSET = 32 - sizeof(MP3Frame);
const uint32_t VBRI_FRAME_COUNT_OFFSET = VBRI_OFFSET + 14;
const uint32_t VBRI_MIN_FRAME_SIZE = VBRI_OFFSET + 26;

const uint32_t XING_TAG = FROM_BIG_ENDIAN("Xing");
enum XingFlags {
  XING_HAS_NUM_FRAMES = 0x01,
  XING_HAS_NUM_BYTES = 0x02,
  XING_HAS_TOC = 0x04,
  XING_HAS_VBR_SCALE = 0x08
};

static int64_t
ParseXing(const char *aBuffer)
{
  uint32_t flags = FROM_BIG_ENDIAN(aBuffer + 4);

  if (!(flags & XING_HAS_NUM_FRAMES)) {
    NS_WARNING("VBR file without frame count. Duration estimation likely to "
               "be totally wrong.");
    return -1;
  }

  int64_t numFrames = -1;
  if (flags & XING_HAS_NUM_FRAMES) {
    numFrames = FROM_BIG_ENDIAN(aBuffer + 8);
  }

  return numFrames;
}

static int64_t
FindNumVBRFrames(const nsAutoCString& aFrame)
{
  const char *buffer = aFrame.get();
  const char *bufferEnd = aFrame.get() + aFrame.Length();

  
  if (aFrame.Length() > VBRI_MIN_FRAME_SIZE &&
      FROM_BIG_ENDIAN(buffer + VBRI_OFFSET) == VBRI_TAG) {
    return FROM_BIG_ENDIAN(buffer + VBRI_FRAME_COUNT_OFFSET);
  }

  
  for (; buffer + sizeof(XING_TAG) < bufferEnd; buffer++) {
    if (FROM_BIG_ENDIAN(buffer) == XING_TAG) {
      return ParseXing(buffer);
    }
  }

  return -1;
}







static const uint32_t MAX_SKIPPED_BYTES = 4096;

enum {
  MP3_HEADER_LENGTH   = 4,
};

MP3FrameParser::MP3FrameParser(int64_t aLength)
: mLock("MP3FrameParser.mLock"),
  mTotalID3Size(0),
  mTotalFrameSize(0),
  mFrameCount(0),
  mOffset(0),
  mLength(aLength),
  mMP3Offset(-1),
  mSamplesPerSecond(0),
  mFirstFrameEnd(-1),
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

        if (buffer <= ch) {
          return NS_ERROR_FAILURE;
        }

        ch = buffer;

        mTotalID3Size += mID3Parser.GetHeaderLength();

        
        mIsMP3 = DEFINITELY_MP3;

        mID3Parser.Reset();
      }
    }
  }

  
  
  if (aStreamOffset < mFirstFrameEnd) {
    uint64_t copyLen = std::min((int64_t)aLength, mFirstFrameEnd - aStreamOffset);
    mFirstFrame.Append((const char *)buffer, copyLen);
    buffer += copyLen;
  }

  while (buffer < bufferEnd) {
    uint16_t frameLen = mMP3Parser.ParseFrameLength(*buffer);

    if (frameLen) {
      
      
      
      
      

      
      mIsMP3 = DEFINITELY_MP3;

      
      
      mSamplesPerSecond = mMP3Parser.GetSampleRate();
      mSamplesPerFrame = mMP3Parser.GetSamplesPerFrame();

      
      
      
      mTotalFrameSize += frameLen;
      mFrameCount++;

      
      
      if (mMP3Offset > -1) {
        uint16_t skip = frameLen - sizeof(MP3Frame);
        buffer += skip ? skip : 1;
        continue;
      }

      
      
      
      mMP3Offset = aStreamOffset
        + (buffer - aBuffer)
        - (sizeof(MP3Frame) - 1);

      buffer++;

      
      
      
      mFirstFrameEnd = mMP3Offset + frameLen;
      uint64_t currOffset = buffer - aBuffer + aStreamOffset;
      uint64_t copyLen = std::min(mFirstFrameEnd - currOffset,
                                  (uint64_t)(bufferEnd - buffer));
      mFirstFrame.Append((const char *)buffer, copyLen);

      buffer += copyLen;

    } else {
      
      buffer++;
    }
  }

  *aOutBytesRead = buffer - aBuffer;

  if (mFirstFrameEnd > -1 && mFirstFrameEnd <= aStreamOffset + buffer - aBuffer) {
    
    mNumFrames = FindNumVBRFrames(mFirstFrame);
    mFirstFrameEnd = -1;
  }

  return NS_OK;
}

void MP3FrameParser::Parse(const char* aBuffer, uint32_t aLength, uint64_t aOffset)
{
  MutexAutoLock mon(mLock);

  if (HasExactDuration()) {
    
    return;
  }

  const uint8_t* buffer = reinterpret_cast<const uint8_t*>(aBuffer);
  int32_t length = aLength;
  uint64_t offset = aOffset;

  
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

    if (mFirstFrameEnd > -1) {
      NS_WARNING("Discontinuity in input while buffering first frame.");
      mFirstFrameEnd = -1;
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

  
  
  
  if (!mID3Parser.IsParsed() && mMP3Offset < 0 &&
      mOffset - mTotalID3Size > MAX_SKIPPED_BYTES) {
    mIsMP3 = NOT_MP3;
  }
}

int64_t MP3FrameParser::GetDuration()
{
  MutexAutoLock mon(mLock);

  if (!ParsedHeaders() || !mSamplesPerSecond) {
    
    return -1;
  }

  MOZ_ASSERT(mFrameCount > 0 && mTotalFrameSize > 0,
             "Frame parser should have seen at least one MP3 frame of positive length.");

  if (!mFrameCount || !mTotalFrameSize) {
    
    return -1;
  }

  double frames;
  if (mNumFrames < 0) {
    
    
    double frameSize = (double)mTotalFrameSize / mFrameCount;
    frames = (double)(mLength - mMP3Offset) / frameSize;
  } else {
    
    frames = mNumFrames;
  }

  
  double usPerFrame = USECS_PER_S * mSamplesPerFrame / mSamplesPerSecond;

  return frames * usPerFrame;
}

int64_t MP3FrameParser::GetMP3Offset()
{
  MutexAutoLock mon(mLock);
  return mMP3Offset;
}

bool MP3FrameParser::ParsedHeaders()
{
  
  
  return mMP3Offset > -1 && mFirstFrameEnd < 0;
}

bool MP3FrameParser::HasExactDuration()
{
  return ParsedHeaders() && mNumFrames > -1;
}

bool MP3FrameParser::NeedsData()
{
  
  
  
  return IsMP3() && !HasExactDuration();
}

}

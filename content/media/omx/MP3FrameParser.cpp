





#include <algorithm>
#include "nsMemory.h"
#include "MP3FrameParser.h"

namespace mozilla {





class ID3Buffer
{
public:

  enum {
    ID3_HEADER_LENGTH = 10
  };

  ID3Buffer(const uint8_t* aBuffer, uint32_t aLength)
  : mBuffer(aBuffer),
    mLength(aLength),
    mSize(0)
  {
    MOZ_ASSERT(mBuffer || !mLength);
  }

  nsresult Parse();

  int64_t GetMP3Offset() const {
    return ID3_HEADER_LENGTH + mSize;
  }

private:
  const uint8_t* mBuffer;
  uint32_t       mLength;
  uint32_t       mSize;
};

nsresult ID3Buffer::Parse()
{
  NS_ENSURE_TRUE(mBuffer && mLength >= ID3_HEADER_LENGTH, NS_ERROR_INVALID_ARG);

  if ((mBuffer[0] != 'I') ||
      (mBuffer[1] != 'D') ||
      (mBuffer[2] != '3') ||
      (mBuffer[6] & 0x80) ||
      (mBuffer[7] & 0x80) ||
      (mBuffer[8] & 0x80) ||
      (mBuffer[9] & 0x80)) {
    return NS_ERROR_INVALID_ARG;
  }

  mSize = ((static_cast<uint32_t>(mBuffer[6])<<21) |
           (static_cast<uint32_t>(mBuffer[7])<<14) |
           (static_cast<uint32_t>(mBuffer[8])<<7)  |
            static_cast<uint32_t>(mBuffer[9]));

  return NS_OK;
}





class MP3Buffer
{
public:

  enum {
    MP3_HEADER_LENGTH   = 4,
    MP3_FRAMESIZE_CONST = 144000,
    MP3_DURATION_CONST  = 8000
  };

  MP3Buffer(const uint8_t* aBuffer, uint32_t aLength)
  : mBuffer(aBuffer),
    mLength(aLength),
    mDurationUs(0),
    mNumFrames(0),
    mBitRateSum(0),
    mFrameSizeSum(0),
    mTrailing(0)
  {
    MOZ_ASSERT(mBuffer || !mLength);
  }

  static const uint8_t* FindNextHeader(const uint8_t* aBuffer, uint32_t aLength);

  nsresult Parse();

  int64_t GetDuration() const {
    return mDurationUs;
  }

  int64_t GetNumberOfFrames() const {
    return mNumFrames;
  }

  int64_t GetBitRateSum() const {
    return mBitRateSum;
  }

  int64_t GetFrameSizeSum() const {
    return mFrameSizeSum;
  }

  int64_t GetTrailing() const {
    return mTrailing;
  }

private:

  enum MP3FrameHeaderField {
    MP3_HDR_FIELD_SYNC,
    MP3_HDR_FIELD_VERSION,
    MP3_HDR_FIELD_LAYER,
    MP3_HDR_FIELD_BITRATE,
    MP3_HDR_FIELD_SAMPLERATE,
    MP3_HDR_FIELD_PADDING,
    MP3_HDR_FIELDS 
  };

  enum {
    MP3_HDR_CONST_FRAMESYNC = 0x7ff,
    MP3_HDR_CONST_VERSION   = 3,
    MP3_HDR_CONST_LAYER     = 1
  };

  static uint32_t ExtractBits(uint32_t aValue, uint32_t aOffset,
                              uint32_t aBits);
  static uint32_t ExtractFrameHeaderField(uint32_t aHeader,
                                          enum MP3FrameHeaderField aField);
  static uint32_t ExtractFrameHeader(const uint8_t* aBuffer);
  static nsresult DecodeFrameHeader(const uint8_t* aBuffer,
                                          size_t* aFrameSize,
                                          uint32_t* aBitRate,
                                          uint64_t* aDuration);

  static const uint16_t sBitRate[16];
  static const uint16_t sSampleRate[4];

  const uint8_t* mBuffer;
  uint32_t       mLength;

  
  int64_t mDurationUs;

  
  int64_t mNumFrames;

  
  int64_t mBitRateSum;

  
  int32_t mFrameSizeSum;

  
  int32_t mTrailing;
};

const uint16_t MP3Buffer::sBitRate[16] = {
  0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0
};

const uint16_t MP3Buffer::sSampleRate[4] = {
  44100, 48000, 32000, 0
};

uint32_t MP3Buffer::ExtractBits(uint32_t aValue, uint32_t aOffset, uint32_t aBits)
{
  return (aValue >> aOffset) & ((0x1ul << aBits) - 1);
}

uint32_t MP3Buffer::ExtractFrameHeaderField(uint32_t aHeader, enum MP3FrameHeaderField aField)
{
  static const uint8_t sField[MP3_HDR_FIELDS][2] = {
    {21, 11}, {19, 2}, {17, 2}, {12, 4}, {10, 2}, {9, 1}
  };

  MOZ_ASSERT(aField < MP3_HDR_FIELDS);
  return ExtractBits(aHeader, sField[aField][0], sField[aField][1]);
}

uint32_t MP3Buffer::ExtractFrameHeader(const uint8_t* aBuffer)
{
  MOZ_ASSERT(aBuffer);

  uint32_t header = (static_cast<uint32_t>(aBuffer[0])<<24) |
                    (static_cast<uint32_t>(aBuffer[1])<<16) |
                    (static_cast<uint32_t>(aBuffer[2])<<8)  |
                     static_cast<uint32_t>(aBuffer[3]);

  uint32_t frameSync = ExtractFrameHeaderField(header, MP3_HDR_FIELD_SYNC);
  uint32_t version = ExtractFrameHeaderField(header, MP3_HDR_FIELD_VERSION);
  uint32_t layer = ExtractFrameHeaderField(header, MP3_HDR_FIELD_LAYER);
  uint32_t bitRate = sBitRate[ExtractFrameHeaderField(header, MP3_HDR_FIELD_BITRATE)];
  uint32_t sampleRate = sSampleRate[ExtractFrameHeaderField(header, MP3_HDR_FIELD_SAMPLERATE)];

  
  
  
  
  
  
  
  return (frameSync == uint32_t(MP3_HDR_CONST_FRAMESYNC)) *
         (version == uint32_t(MP3_HDR_CONST_VERSION)) *
         (layer == uint32_t(MP3_HDR_CONST_LAYER)) * !!bitRate * !!sampleRate * header;
}

const uint8_t* MP3Buffer::FindNextHeader(const uint8_t* aBuffer, uint32_t aLength)
{
  MOZ_ASSERT(aBuffer || !aLength);

  
  

  while (aLength >= MP3_HEADER_LENGTH) {
    if (ExtractFrameHeader(aBuffer)) {
      break;
    }
    ++aBuffer;
    --aLength;
  }

  return aBuffer;
}

nsresult MP3Buffer::DecodeFrameHeader(const uint8_t* aBuffer,
                                      uint32_t* aFrameSize,
                                      uint32_t* aBitRate,
                                      uint64_t* aDuration)
{
  uint32_t header = ExtractFrameHeader(aBuffer);

  if (!header) {
    return NS_ERROR_INVALID_ARG;
  }

  uint32_t bitRate = sBitRate[ExtractFrameHeaderField(header, MP3_HDR_FIELD_BITRATE)];
  uint32_t sampleRate = sSampleRate[ExtractFrameHeaderField(header, MP3_HDR_FIELD_SAMPLERATE)];

  uint32_t padding = ExtractFrameHeaderField(header, MP3_HDR_FIELD_PADDING);
  uint32_t frameSize = (uint64_t(MP3_FRAMESIZE_CONST) * bitRate) / sampleRate + padding;

  MOZ_ASSERT(aBitRate);
  *aBitRate = bitRate;

  MOZ_ASSERT(aFrameSize);
  *aFrameSize = frameSize;

  MOZ_ASSERT(aDuration);
  *aDuration = (uint64_t(MP3_DURATION_CONST) * frameSize) / bitRate;

  return NS_OK;
}

nsresult MP3Buffer::Parse()
{
  
  
  

  const uint8_t* buffer = mBuffer;
  uint32_t       length = mLength;

  while (length >= MP3_HEADER_LENGTH) {

    uint32_t frameSize;
    uint32_t bitRate;
    uint64_t duration;

    nsresult rv = DecodeFrameHeader(buffer, &frameSize, &bitRate, &duration);
    NS_ENSURE_SUCCESS(rv, rv);

    mBitRateSum += bitRate;
    mDurationUs += duration;
    ++mNumFrames;

    mFrameSizeSum += frameSize;

    if (frameSize <= length) {
      length -= frameSize;
    } else {
      length = 0;
    }

    buffer += frameSize;
  }

  mTrailing = length;

  return NS_OK;
}

MP3FrameParser::MP3FrameParser(int64_t aLength)
: mBufferLength(0),
  mLock("MP3FrameParser.mLock"),
  mDurationUs(0),
  mBitRateSum(0),
  mNumFrames(0),
  mOffset(0),
  mUnhandled(0),
  mLength(aLength),
  mTrailing(0),
  mIsMP3(true)
{ }

size_t MP3FrameParser::ParseInternalBuffer(const uint8_t* aBuffer, uint32_t aLength, int64_t aOffset)
{
  if (mOffset != aOffset) {
    
    mBufferLength = 0;
    return 0;
  }

  size_t copyLength = 0;

  if (mBufferLength || !mOffset) {

    
    
    
    copyLength = std::min<size_t>(NS_ARRAY_LENGTH(mBuffer)-mBufferLength, aLength);
    memcpy(mBuffer+mBufferLength, aBuffer, copyLength*sizeof(*mBuffer));
    mBufferLength += copyLength;
  }

  if ((mBufferLength >= ID3Buffer::ID3_HEADER_LENGTH) && (mOffset < ID3Buffer::ID3_HEADER_LENGTH)) {

    
    ID3Buffer id3Buffer(mBuffer, mBufferLength);
    nsresult rv = id3Buffer.Parse();

    if (rv == NS_OK) {
      mOffset += id3Buffer.GetMP3Offset()-(mBufferLength-copyLength);
      mBufferLength = 0;
    }
  }

  if (mBufferLength >= MP3Buffer::MP3_HEADER_LENGTH) {

    
    
    MP3Buffer mp3Buffer(mBuffer, mBufferLength);
    nsresult rv = mp3Buffer.Parse();

    if (rv == NS_OK) {
      mDurationUs += mp3Buffer.GetDuration();
      mBitRateSum += mp3Buffer.GetBitRateSum();
      mNumFrames  += mp3Buffer.GetNumberOfFrames();
      mOffset     += mp3Buffer.GetFrameSizeSum()-(mBufferLength-copyLength);
      mBufferLength = 0;
    }
  }

  if (mBufferLength) {
    
    
    
    mOffset += copyLength;
    mIsMP3   = (mBufferLength < NS_ARRAY_LENGTH(mBuffer));
  } else {
    
    
    copyLength = 0;
  }

  if (mOffset > mLength) {
    mLength = mOffset;
  }

  return copyLength;
}

void MP3FrameParser::Parse(const uint8_t* aBuffer, uint32_t aLength, int64_t aOffset)
{
  MutexAutoLock mon(mLock);

  
  
  size_t bufferIncr = ParseInternalBuffer(aBuffer, aLength, aOffset);

  aBuffer += bufferIncr;
  aLength -= bufferIncr;
  aOffset += bufferIncr;

  
  
  int retries = 1;

  if (aOffset+aLength <= mOffset) {
    
    return;
  } else if (aOffset < mOffset) {
    
    aLength -= mOffset-aOffset;
    aBuffer += mOffset-aOffset;
    aOffset  = mOffset;
  } else if (aOffset > mOffset) {
    
    mUnhandled += aOffset-mOffset;

    
    
    
    
    
    retries = 5;
  }

  uint32_t trailing = 0;

  while (retries) {

    MP3Buffer mp3Buffer(aBuffer, aLength);
    nsresult rv = mp3Buffer.Parse();

    if (rv != NS_OK) {
      --retries;

      if (!retries) {
        mIsMP3 = false;
        return;
      }

      
      const uint8_t *buffer = MP3Buffer::FindNextHeader(aBuffer+1, aLength-1);

      mUnhandled += buffer-aBuffer;
      mOffset     = aOffset + buffer-aBuffer;
      aLength    -= buffer-aBuffer;
      aBuffer     = buffer;
    } else {
      mDurationUs += mp3Buffer.GetDuration();
      mBitRateSum += mp3Buffer.GetBitRateSum();
      mNumFrames  += mp3Buffer.GetNumberOfFrames();
      mOffset     += mp3Buffer.GetFrameSizeSum();

      trailing = mp3Buffer.GetTrailing();
      retries = 0;
    }
  }

  if (trailing) {
    
    MOZ_ASSERT(trailing < (NS_ARRAY_LENGTH(mBuffer)*sizeof(*mBuffer)));
    memcpy(mBuffer, aBuffer+(aLength-trailing), trailing);
    mBufferLength = trailing;
  }

  if (mOffset > mLength) {
    mLength = mOffset;
  }
}

void MP3FrameParser::NotifyDataArrived(const char* aBuffer, uint32_t aLength, int64_t aOffset)
{
  Parse(reinterpret_cast<const uint8_t*>(aBuffer), aLength, aOffset);
}

int64_t MP3FrameParser::GetDuration()
{
  MutexAutoLock mon(mLock);

  if (!mNumFrames) {
    return -1; 
  }

  
  
  int64_t avgBitRate = mBitRateSum / mNumFrames;
  NS_ENSURE_TRUE(avgBitRate > 0, mDurationUs);

  MOZ_ASSERT(mLength >= mOffset);
  int64_t unhandled = mUnhandled + (mLength-mOffset);

  return mDurationUs + (uint64_t(MP3Buffer::MP3_DURATION_CONST) * unhandled) / avgBitRate;
}

}

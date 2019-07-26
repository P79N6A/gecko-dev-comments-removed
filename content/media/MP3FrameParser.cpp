





#include <algorithm>
#include "nsMemory.h"
#include "MP3FrameParser.h"
#include "VideoUtils.h"

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

  int64_t Length() const {
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
    mSampleRate(0),
    mFrameSizeSum(0)
  {
    MOZ_ASSERT(mBuffer || !mLength);
  }

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

  int16_t GetSampleRate() const {
    return mSampleRate;
  }

  int64_t GetFrameSizeSum() const {
    return mFrameSizeSum;
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
                                    uint32_t* aFrameSize,
                                    uint32_t* aBitRate,
                                    uint16_t* aSampleRate,
                                    uint64_t* aDuration);

  static const uint16_t sBitRate[16];
  static const uint16_t sSampleRate[4];

  const uint8_t* mBuffer;
  uint32_t       mLength;

  
  int64_t mDurationUs;

  
  int64_t mNumFrames;

  
  int64_t mBitRateSum;

  
  int16_t mSampleRate;

  
  int32_t mFrameSizeSum;
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

nsresult MP3Buffer::DecodeFrameHeader(const uint8_t* aBuffer,
                                      uint32_t* aFrameSize,
                                      uint32_t* aBitRate,
                                      uint16_t* aSampleRate,
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

  MOZ_ASSERT(aSampleRate);
  *aSampleRate = sampleRate;

  return NS_OK;
}

nsresult MP3Buffer::Parse()
{
  
  
  

  const uint8_t* buffer = mBuffer;
  uint32_t length = mLength;

  while (length >= MP3_HEADER_LENGTH) {

    uint32_t frameSize;
    uint32_t bitRate;
    uint16_t sampleRate;
    uint64_t duration;

    nsresult rv = DecodeFrameHeader(buffer, &frameSize, &bitRate,
                                    &sampleRate, &duration);
    if (NS_FAILED(rv)) {
      return rv;
    }

    mBitRateSum += bitRate;
    mDurationUs += duration;
    ++mNumFrames;

    mFrameSizeSum += frameSize;

    mSampleRate = sampleRate;

    if (frameSize <= length) {
      length -= frameSize;
    } else {
      length = 0;
    }

    buffer += frameSize;
  }

  return NS_OK;
}




static const uint32_t MAX_SKIPPED_BYTES = 200 * 1024;





static const uint32_t SAMPLES_PER_FRAME = 1152;

MP3FrameParser::MP3FrameParser(int64_t aLength)
: mBufferLength(0),
  mLock("MP3FrameParser.mLock"),
  mDurationUs(0),
  mBitRateSum(0),
  mTotalFrameSize(0),
  mNumFrames(0),
  mOffset(0),
  mLength(aLength),
  mMP3Offset(-1),
  mSkippedBytes(0),
  mSampleRate(0),
  mIsMP3(MAYBE_MP3)
{ }

nsresult MP3FrameParser::ParseBuffer(const uint8_t* aBuffer,
                                     uint32_t aLength,
                                     int64_t aStreamOffset,
                                     uint32_t* aOutBytesRead)
{
  
  
  uint32_t bufferOffset = 0;
  uint32_t headersParsed = 0;
  while (bufferOffset < aLength) {
    const uint8_t* buffer = aBuffer + bufferOffset;
    const uint32_t length = aLength - bufferOffset;
    if (mMP3Offset == -1) {
      
      
      if (length < ID3Buffer::ID3_HEADER_LENGTH) {
        
        break;
      }
      ID3Buffer id3Buffer(buffer, length);
      if (NS_SUCCEEDED(id3Buffer.Parse())) {
        bufferOffset += id3Buffer.Length();
        
        headersParsed++;
        continue;
      }
    }
    if (length < MP3Buffer::MP3_HEADER_LENGTH) {
      
      break;
    }
    MP3Buffer mp3Buffer(buffer, length);
    if (NS_SUCCEEDED(mp3Buffer.Parse())) {
      headersParsed++;
      if (mMP3Offset == -1) {
        mMP3Offset = aStreamOffset + bufferOffset;
      }
      mDurationUs += mp3Buffer.GetDuration();
      mBitRateSum += mp3Buffer.GetBitRateSum();
      mTotalFrameSize += mp3Buffer.GetFrameSizeSum();
      mSampleRate = mp3Buffer.GetSampleRate();
      mNumFrames += mp3Buffer.GetNumberOfFrames();
      bufferOffset += mp3Buffer.GetFrameSizeSum();
    } else {
      
      ++bufferOffset;
    }
  }
  if (headersParsed == 0) {
    if (mIsMP3 == MAYBE_MP3) {
      mSkippedBytes += aLength;
      if (mSkippedBytes > MAX_SKIPPED_BYTES) {
        mIsMP3 = NOT_MP3;
        return NS_ERROR_FAILURE;
      }
    }
  } else {
    mIsMP3 = DEFINITELY_MP3;
    mSkippedBytes = 0;
  }
  *aOutBytesRead = bufferOffset;
  return NS_OK;
}

void MP3FrameParser::Parse(const char* aBuffer, uint32_t aLength, int64_t aOffset)
{
  MutexAutoLock mon(mLock);

  const uint8_t* buffer = reinterpret_cast<const uint8_t*>(aBuffer);
  const int64_t lastChunkEnd = mOffset + mBufferLength;
  if (aOffset + aLength <= lastChunkEnd) {
    
    return;
  } else if (aOffset < lastChunkEnd) {
    
    aLength -= lastChunkEnd - aOffset;
    buffer += lastChunkEnd - aOffset;
    aOffset = lastChunkEnd;
  } else if (aOffset > lastChunkEnd) {
    
    mOffset += aOffset - lastChunkEnd;
    mSkippedBytes = 0;
  }

  if (mBufferLength > 0) {
    
    
    
    uint32_t copyLength = std::min<size_t>(NS_ARRAY_LENGTH(mBuffer)-mBufferLength, aLength);
    memcpy(mBuffer+mBufferLength, buffer, copyLength*sizeof(*mBuffer));
    
    int64_t streamOffset = mOffset - mBufferLength;
    uint32_t bufferLength = mBufferLength + copyLength;
    uint32_t bytesRead = 0;
    if (NS_FAILED(ParseBuffer(mBuffer,
                              bufferLength,
                              streamOffset,
                              &bytesRead))) {
      return;
    }
    MOZ_ASSERT(bytesRead >= mBufferLength, "Parse should leave original buffer");
    
    
    uint32_t adjust = bytesRead - mBufferLength;
    aOffset += adjust;
    aLength -= adjust;
    mBufferLength = 0;
  }

  uint32_t bytesRead = 0;
  if (NS_FAILED(ParseBuffer(buffer,
                            aLength,
                            aOffset,
                            &bytesRead))) {
    return;
  }
  mOffset += bytesRead;

  if (bytesRead < aLength) {
    
    
    uint32_t trailing = aLength - bytesRead;
    MOZ_ASSERT(trailing < (NS_ARRAY_LENGTH(mBuffer)*sizeof(mBuffer[0])));
    memcpy(mBuffer, buffer+(aLength-trailing), trailing);
    mBufferLength = trailing;
  }

  if (mOffset > mLength) {
    mLength = mOffset;
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

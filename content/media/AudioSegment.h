




#ifndef MOZILLA_AUDIOSEGMENT_H_
#define MOZILLA_AUDIOSEGMENT_H_

#include "MediaSegment.h"
#include "nsISupportsImpl.h"
#include "nsAudioStream.h"
#include "SharedBuffer.h"

namespace mozilla {

struct AudioChunk {
  typedef nsAudioStream::SampleFormat SampleFormat;

  
  void SliceTo(TrackTicks aStart, TrackTicks aEnd)
  {
    NS_ASSERTION(aStart >= 0 && aStart < aEnd && aEnd <= mDuration,
                 "Slice out of bounds");
    if (mBuffer) {
      mOffset += int32_t(aStart);
    }
    mDuration = aEnd - aStart;
  }
  TrackTicks GetDuration() const { return mDuration; }
  bool CanCombineWithFollowing(const AudioChunk& aOther) const
  {
    if (aOther.mBuffer != mBuffer) {
      return false;
    }
    if (mBuffer) {
      NS_ASSERTION(aOther.mBufferFormat == mBufferFormat && aOther.mBufferLength == mBufferLength,
                   "Wrong metadata about buffer");
      return aOther.mOffset == mOffset + mDuration && aOther.mVolume == mVolume;
    }
    return true;
  }
  bool IsNull() const { return mBuffer == nullptr; }
  void SetNull(TrackTicks aDuration)
  {
    mBuffer = nullptr;
    mDuration = aDuration;
    mOffset = 0;
    mVolume = 1.0f;
  }

  TrackTicks mDuration;           
  nsRefPtr<SharedBuffer> mBuffer; 
  int32_t mBufferLength;          
  SampleFormat mBufferFormat;     
  int32_t mOffset;                
  float mVolume;                  
};





class AudioSegment : public MediaSegmentBase<AudioSegment, AudioChunk> {
public:
  typedef nsAudioStream::SampleFormat SampleFormat;

  static int GetSampleSize(SampleFormat aFormat)
  {
    switch (aFormat) {
    case nsAudioStream::FORMAT_U8: return 1;
    case nsAudioStream::FORMAT_S16: return 2;
    case nsAudioStream::FORMAT_FLOAT32: return 4;
    }
    NS_ERROR("Bad format");
    return 0;
  }

  AudioSegment() : MediaSegmentBase<AudioSegment, AudioChunk>(AUDIO), mChannels(0) {}

  bool IsInitialized()
  {
    return mChannels > 0;
  }
  void Init(int32_t aChannels)
  {
    NS_ASSERTION(aChannels > 0, "Bad number of channels");
    NS_ASSERTION(!IsInitialized(), "Already initialized");
    mChannels = aChannels;
  }
  int32_t GetChannels()
  {
    NS_ASSERTION(IsInitialized(), "Not initialized");
    return mChannels;
  }
  



  SampleFormat GetFirstFrameFormat()
  {
    for (ChunkIterator ci(*this); !ci.IsEnded(); ci.Next()) {
      if (ci->mBuffer) {
        return ci->mBufferFormat;
      }
    }
    return nsAudioStream::FORMAT_FLOAT32;
  }
  void AppendFrames(already_AddRefed<SharedBuffer> aBuffer, int32_t aBufferLength,
                    int32_t aStart, int32_t aEnd, SampleFormat aFormat)
  {
    NS_ASSERTION(mChannels > 0, "Not initialized");
    AudioChunk* chunk = AppendChunk(aEnd - aStart);
    chunk->mBuffer = aBuffer;
    chunk->mBufferFormat = aFormat;
    chunk->mBufferLength = aBufferLength;
    chunk->mOffset = aStart;
    chunk->mVolume = 1.0f;
  }
  void ApplyVolume(float aVolume);
  



  void WriteTo(nsAudioStream* aOutput);

  
  void InitFrom(const AudioSegment& aOther)
  {
    NS_ASSERTION(mChannels == 0, "Channels already set");
    mChannels = aOther.mChannels;
  }
  void CheckCompatible(const AudioSegment& aOther) const
  {
    NS_ASSERTION(aOther.mChannels == mChannels, "Non-matching channels");
  }
  static Type StaticType() { return AUDIO; }

protected:
  int32_t mChannels;
};

}

#endif 

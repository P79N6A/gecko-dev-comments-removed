




#ifndef MOZILLA_AUDIOSEGMENT_H_
#define MOZILLA_AUDIOSEGMENT_H_

#include "MediaSegment.h"
#include "AudioSampleFormat.h"
#include "SharedBuffer.h"
#include "WebAudioUtils.h"
#ifdef MOZILLA_INTERNAL_API
#include "mozilla/TimeStamp.h"
#endif
#include <float.h>

namespace mozilla {

template<typename T>
class SharedChannelArrayBuffer : public ThreadSharedObject {
public:
  explicit SharedChannelArrayBuffer(nsTArray<nsTArray<T> >* aBuffers)
  {
    mBuffers.SwapElements(*aBuffers);
  }

  virtual size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const override
  {
    size_t amount = 0;
    amount += mBuffers.SizeOfExcludingThis(aMallocSizeOf);
    for (size_t i = 0; i < mBuffers.Length(); i++) {
      amount += mBuffers[i].SizeOfExcludingThis(aMallocSizeOf);
    }

    return amount;
  }

  virtual size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const override
  {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }

  nsTArray<nsTArray<T> > mBuffers;
};

class AudioMixer;




const int GUESS_AUDIO_CHANNELS = 2;



const uint32_t WEBAUDIO_BLOCK_SIZE_BITS = 7;
const uint32_t WEBAUDIO_BLOCK_SIZE = 1 << WEBAUDIO_BLOCK_SIZE_BITS;

void InterleaveAndConvertBuffer(const void** aSourceChannels,
                                AudioSampleFormat aSourceFormat,
                                int32_t aLength, float aVolume,
                                int32_t aChannels,
                                AudioDataValue* aOutput);






void DownmixAndInterleave(const nsTArray<const void*>& aChannelData,
                          AudioSampleFormat aSourceFormat, int32_t aDuration,
                          float aVolume, uint32_t aOutputChannels,
                          AudioDataValue* aOutput);









struct AudioChunk {
  typedef mozilla::AudioSampleFormat SampleFormat;

  
  void SliceTo(StreamTime aStart, StreamTime aEnd)
  {
    MOZ_ASSERT(aStart >= 0 && aStart < aEnd && aEnd <= mDuration,
               "Slice out of bounds");
    if (mBuffer) {
      MOZ_ASSERT(aStart < INT32_MAX, "Can't slice beyond 32-bit sample lengths");
      for (uint32_t channel = 0; channel < mChannelData.Length(); ++channel) {
        mChannelData[channel] = AddAudioSampleOffset(mChannelData[channel],
            mBufferFormat, int32_t(aStart));
      }
    }
    mDuration = aEnd - aStart;
  }
  StreamTime GetDuration() const { return mDuration; }
  bool CanCombineWithFollowing(const AudioChunk& aOther) const
  {
    if (aOther.mBuffer != mBuffer) {
      return false;
    }
    if (mBuffer) {
      NS_ASSERTION(aOther.mBufferFormat == mBufferFormat,
                   "Wrong metadata about buffer");
      NS_ASSERTION(aOther.mChannelData.Length() == mChannelData.Length(),
                   "Mismatched channel count");
      if (mDuration > INT32_MAX) {
        return false;
      }
      for (uint32_t channel = 0; channel < mChannelData.Length(); ++channel) {
        if (aOther.mChannelData[channel] != AddAudioSampleOffset(mChannelData[channel],
            mBufferFormat, int32_t(mDuration))) {
          return false;
        }
      }
    }
    return true;
  }
  bool IsNull() const { return mBuffer == nullptr; }
  void SetNull(StreamTime aDuration)
  {
    mBuffer = nullptr;
    mChannelData.Clear();
    mDuration = aDuration;
    mVolume = 1.0f;
    mBufferFormat = AUDIO_FORMAT_SILENCE;
  }

  bool IsSilentOrSubnormal() const
  {
    if (!mBuffer) {
      return true;
    }

    for (uint32_t i = 0, length = mChannelData.Length(); i < length; ++i) {
      const float* channel = static_cast<const float*>(mChannelData[i]);
      for (StreamTime frame = 0; frame < mDuration; ++frame) {
        if (fabs(channel[frame]) >= FLT_MIN) {
          return false;
        }
      }
    }

    return true;
  }

  int ChannelCount() const { return mChannelData.Length(); }

  bool IsMuted() const { return mVolume == 0.0f; }

  size_t SizeOfExcludingThisIfUnshared(MallocSizeOf aMallocSizeOf) const
  {
    return SizeOfExcludingThis(aMallocSizeOf, true);
  }

  size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf, bool aUnshared) const
  {
    size_t amount = 0;

    
    
    
    if (mBuffer && (!aUnshared || !mBuffer->IsShared())) {
      amount += mBuffer->SizeOfIncludingThis(aMallocSizeOf);
    }

    
    amount += mChannelData.SizeOfExcludingThis(aMallocSizeOf);
    return amount;
  }

  StreamTime mDuration; 
  nsRefPtr<ThreadSharedObject> mBuffer; 
  nsTArray<const void*> mChannelData; 
  float mVolume; 
  SampleFormat mBufferFormat; 
#ifdef MOZILLA_INTERNAL_API
  mozilla::TimeStamp mTimeStamp;           
#endif
};





class AudioSegment : public MediaSegmentBase<AudioSegment, AudioChunk> {
public:
  typedef mozilla::AudioSampleFormat SampleFormat;

  AudioSegment() : MediaSegmentBase<AudioSegment, AudioChunk>(AUDIO) {}

  
  template<typename T>
  void Resample(SpeexResamplerState* aResampler, uint32_t aInRate, uint32_t aOutRate)
  {
    mDuration = 0;
#ifdef DEBUG
    uint32_t segmentChannelCount = ChannelCount();
#endif

    for (ChunkIterator ci(*this); !ci.IsEnded(); ci.Next()) {
      nsAutoTArray<nsTArray<T>, GUESS_AUDIO_CHANNELS> output;
      nsAutoTArray<const T*, GUESS_AUDIO_CHANNELS> bufferPtrs;
      AudioChunk& c = *ci;
      
      if (c.IsNull()) {
        c.mDuration = (c.mDuration * aOutRate) / aInRate;
        mDuration += c.mDuration;
        continue;
      }
      uint32_t channels = c.mChannelData.Length();
      MOZ_ASSERT(channels == segmentChannelCount);
      output.SetLength(channels);
      bufferPtrs.SetLength(channels);
#if !defined(MOZILLA_XPCOMRT_API)

      uint32_t inFrames = c.mDuration;
#endif 
      
      NS_ASSERTION((UINT32_MAX - aInRate + 1) / c.mDuration >= aOutRate,
                   "Dropping samples");
      uint32_t outSize = (c.mDuration * aOutRate + aInRate - 1) / aInRate;
      for (uint32_t i = 0; i < channels; i++) {
        T* out = output[i].AppendElements(outSize);
        uint32_t outFrames = outSize;

#if !defined(MOZILLA_XPCOMRT_API)

        const T* in = static_cast<const T*>(c.mChannelData[i]);
        dom::WebAudioUtils::SpeexResamplerProcess(aResampler, i,
                                                  in, &inFrames,
                                                  out, &outFrames);
        MOZ_ASSERT(inFrames == c.mDuration);
#endif 

        bufferPtrs[i] = out;
        output[i].SetLength(outFrames);
      }
      MOZ_ASSERT(channels > 0);
      c.mDuration = output[0].Length();
      c.mBuffer = new mozilla::SharedChannelArrayBuffer<T>(&output);
      for (uint32_t i = 0; i < channels; i++) {
        c.mChannelData[i] = bufferPtrs[i];
      }
      mDuration += c.mDuration;
    }
  }

  void ResampleChunks(SpeexResamplerState* aResampler,
                      uint32_t aInRate,
                      uint32_t aOutRate);

  void AppendFrames(already_AddRefed<ThreadSharedObject> aBuffer,
                    const nsTArray<const float*>& aChannelData,
                    int32_t aDuration)
  {
    AudioChunk* chunk = AppendChunk(aDuration);
    chunk->mBuffer = aBuffer;
    for (uint32_t channel = 0; channel < aChannelData.Length(); ++channel) {
      chunk->mChannelData.AppendElement(aChannelData[channel]);
    }
    chunk->mVolume = 1.0f;
    chunk->mBufferFormat = AUDIO_FORMAT_FLOAT32;
#ifdef MOZILLA_INTERNAL_API
    chunk->mTimeStamp = TimeStamp::Now();
#endif
  }
  void AppendFrames(already_AddRefed<ThreadSharedObject> aBuffer,
                    const nsTArray<const int16_t*>& aChannelData,
                    int32_t aDuration)
  {
    AudioChunk* chunk = AppendChunk(aDuration);
    chunk->mBuffer = aBuffer;
    for (uint32_t channel = 0; channel < aChannelData.Length(); ++channel) {
      chunk->mChannelData.AppendElement(aChannelData[channel]);
    }
    chunk->mVolume = 1.0f;
    chunk->mBufferFormat = AUDIO_FORMAT_S16;
#ifdef MOZILLA_INTERNAL_API
    chunk->mTimeStamp = TimeStamp::Now();
#endif
  }
  
  
  AudioChunk* AppendAndConsumeChunk(AudioChunk* aChunk)
  {
    AudioChunk* chunk = AppendChunk(aChunk->mDuration);
    chunk->mBuffer = aChunk->mBuffer.forget();
    chunk->mChannelData.SwapElements(aChunk->mChannelData);
    chunk->mVolume = aChunk->mVolume;
    chunk->mBufferFormat = aChunk->mBufferFormat;
#ifdef MOZILLA_INTERNAL_API
    chunk->mTimeStamp = TimeStamp::Now();
#endif
    return chunk;
  }
  void ApplyVolume(float aVolume);
  void WriteTo(uint64_t aID, AudioMixer& aMixer, uint32_t aChannelCount, uint32_t aSampleRate);

  int ChannelCount() {
    NS_WARN_IF_FALSE(!mChunks.IsEmpty(),
        "Cannot query channel count on a AudioSegment with no chunks.");
    
    
    for (ChunkIterator ci(*this); !ci.IsEnded(); ci.Next()) {
      if (ci->ChannelCount()) {
        return ci->ChannelCount();
      }
    }
    return 0;
  }

  bool IsNull() const {
    for (ChunkIterator ci(*const_cast<AudioSegment*>(this)); !ci.IsEnded();
         ci.Next()) {
      if (!ci->IsNull()) {
        return false;
      }
    }
    return true;
  }

  static Type StaticType() { return AUDIO; }

  virtual size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const override
  {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }
};

} 

#endif 

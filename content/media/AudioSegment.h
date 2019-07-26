




#ifndef MOZILLA_AUDIOSEGMENT_H_
#define MOZILLA_AUDIOSEGMENT_H_

#include "MediaSegment.h"
#include "AudioSampleFormat.h"
#include "SharedBuffer.h"
#include "WebAudioUtils.h"
#ifdef MOZILLA_INTERNAL_API
#include "mozilla/TimeStamp.h"
#endif

namespace mozilla {

template<typename T>
class SharedChannelArrayBuffer : public ThreadSharedObject {
public:
  SharedChannelArrayBuffer(nsTArray<nsTArray<T>>* aBuffers)
  {
    mBuffers.SwapElements(*aBuffers);
  }
  nsTArray<nsTArray<T>> mBuffers;
};

class AudioStream;
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

  
  void SliceTo(TrackTicks aStart, TrackTicks aEnd)
  {
    NS_ASSERTION(aStart >= 0 && aStart < aEnd && aEnd <= mDuration,
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
  TrackTicks GetDuration() const { return mDuration; }
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
  void SetNull(TrackTicks aDuration)
  {
    mBuffer = nullptr;
    mChannelData.Clear();
    mDuration = aDuration;
    mVolume = 1.0f;
  }
  int ChannelCount() const { return mChannelData.Length(); }

  TrackTicks mDuration; 
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

    for (ChunkIterator ci(*this); !ci.IsEnded(); ci.Next()) {
      nsAutoTArray<nsTArray<T>, GUESS_AUDIO_CHANNELS> output;
      nsAutoTArray<const T*, GUESS_AUDIO_CHANNELS> bufferPtrs;
      AudioChunk& c = *ci;
      uint32_t channels = c.mChannelData.Length();
      output.SetLength(channels);
      bufferPtrs.SetLength(channels);
      uint32_t inFrames = c.mDuration,
      outFrames = c.mDuration * aOutRate / aInRate;
      for (uint32_t i = 0; i < channels; i++) {
        const T* in = static_cast<const T*>(c.mChannelData[i]);
        T* out = output[i].AppendElements(outFrames);

        dom::WebAudioUtils::SpeexResamplerProcess(aResampler, i,
                                                  in, &inFrames,
                                                  out, &outFrames);

        bufferPtrs[i] = out;
        output[i].SetLength(outFrames);
      }
      c.mBuffer = new mozilla::SharedChannelArrayBuffer<T>(&output);
      for (uint32_t i = 0; i < channels; i++) {
        c.mChannelData[i] = bufferPtrs[i];
      }
      c.mDuration = outFrames;
      mDuration += c.mDuration;
    }
  }

  void ResampleChunks(SpeexResamplerState* aResampler);

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
  void WriteTo(uint64_t aID, AudioStream* aOutput, AudioMixer* aMixer = nullptr);

  int ChannelCount() {
    NS_WARN_IF_FALSE(!mChunks.IsEmpty(),
        "Cannot query channel count on a AudioSegment with no chunks.");
    return mChunks.IsEmpty() ? 0 : mChunks[0].mChannelData.Length();
  }

  static Type StaticType() { return AUDIO; }
};

}

#endif 

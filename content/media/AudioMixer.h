




#ifndef MOZILLA_AUDIOMIXER_H_
#define MOZILLA_AUDIOMIXER_H_

#include "AudioSampleFormat.h"
#include "nsTArray.h"
#include "mozilla/PodOperations.h"

namespace mozilla {
typedef void(*MixerFunc)(AudioDataValue* aMixedBuffer,
                         AudioSampleFormat aFormat,
                         uint32_t aChannels,
                         uint32_t aFrames,
                         uint32_t aSampleRate);













class AudioMixer
{
public:
  AudioMixer(MixerFunc aCallback)
    : mCallback(aCallback),
      mFrames(0),
      mChannels(0),
      mSampleRate(0)
  { }

  

  void FinishMixing() {
    mCallback(mMixedAudio.Elements(),
              AudioSampleTypeToFormat<AudioDataValue>::Format,
              mChannels,
              mFrames,
              mSampleRate);
    PodZero(mMixedAudio.Elements(), mMixedAudio.Length());
    mSampleRate = mChannels = mFrames = 0;
  }

  
  void Mix(AudioDataValue* aSamples,
           uint32_t aChannels,
           uint32_t aFrames,
           uint32_t aSampleRate) {
    if (!mFrames && !mChannels) {
      mFrames = aFrames;
      mChannels = aChannels;
      mSampleRate = aSampleRate;
      EnsureCapacityAndSilence();
    }

    MOZ_ASSERT(aFrames == mFrames);
    MOZ_ASSERT(aChannels == mChannels);
    MOZ_ASSERT(aSampleRate == mSampleRate);

    for (uint32_t i = 0; i < aFrames * aChannels; i++) {
      mMixedAudio[i] += aSamples[i];
    }
  }
private:
  void EnsureCapacityAndSilence() {
    if (mFrames * mChannels > mMixedAudio.Length()) {
      mMixedAudio.SetLength(mFrames* mChannels);
    }
    PodZero(mMixedAudio.Elements(), mMixedAudio.Length());
  }

  
  MixerFunc mCallback;
  
  uint32_t mFrames;
  
  uint32_t mChannels;
  
  uint32_t mSampleRate;
  
  nsTArray<AudioDataValue> mMixedAudio;
};
}

#endif 

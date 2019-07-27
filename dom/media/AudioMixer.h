




#ifndef MOZILLA_AUDIOMIXER_H_
#define MOZILLA_AUDIOMIXER_H_

#include "AudioSampleFormat.h"
#include "nsTArray.h"
#include "mozilla/PodOperations.h"
#include "mozilla/LinkedList.h"
#include "AudioStream.h"

namespace mozilla {

struct MixerCallbackReceiver {
  virtual void MixerCallback(AudioDataValue* aMixedBuffer,
                             AudioSampleFormat aFormat,
                             uint32_t aChannels,
                             uint32_t aFrames,
                             uint32_t aSampleRate) = 0;
};












class AudioMixer
{
public:
  AudioMixer()
    : mFrames(0),
      mChannels(0),
      mSampleRate(0)
  { }

  ~AudioMixer()
  {
    MixerCallback* cb;
    while ((cb = mCallbacks.popFirst())) {
      delete cb;
    }
  }

  void StartMixing()
  {
    mSampleRate = mChannels = mFrames = 0;
  }

  

  void FinishMixing() {
    MOZ_ASSERT(mChannels && mFrames && mSampleRate, "Mix not called for this cycle?");
    for (MixerCallback* cb = mCallbacks.getFirst();
         cb != nullptr; cb = cb->getNext()) {
      cb->mReceiver->MixerCallback(mMixedAudio.Elements(),
                                   AudioSampleTypeToFormat<AudioDataValue>::Format,
                                   mChannels,
                                   mFrames,
                                   mSampleRate);
    }
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

  void AddCallback(MixerCallbackReceiver* aReceiver) {
    mCallbacks.insertBack(new MixerCallback(aReceiver));
  }

  bool FindCallback(MixerCallbackReceiver* aReceiver) {
    for (MixerCallback* cb = mCallbacks.getFirst();
         cb != nullptr; cb = cb->getNext()) {
      if (cb->mReceiver == aReceiver) {
        return true;
      }
    }
    return false;
  }

  bool RemoveCallback(MixerCallbackReceiver* aReceiver) {
    for (MixerCallback* cb = mCallbacks.getFirst();
         cb != nullptr; cb = cb->getNext()) {
      if (cb->mReceiver == aReceiver) {
        cb->remove();
        delete cb;
        return true;
      }
    }
    return false;
  }
private:
  void EnsureCapacityAndSilence() {
    if (mFrames * mChannels > mMixedAudio.Length()) {
      mMixedAudio.SetLength(mFrames* mChannels);
    }
    PodZero(mMixedAudio.Elements(), mMixedAudio.Length());
  }

  class MixerCallback : public LinkedListElement<MixerCallback>
  {
  public:
    explicit MixerCallback(MixerCallbackReceiver* aReceiver)
      : mReceiver(aReceiver)
    { }
    MixerCallbackReceiver* mReceiver;
  };

  
  LinkedList<MixerCallback> mCallbacks;
  
  uint32_t mFrames;
  
  uint32_t mChannels;
  
  uint32_t mSampleRate;
  
  nsTArray<AudioDataValue> mMixedAudio;
};
}

#endif 

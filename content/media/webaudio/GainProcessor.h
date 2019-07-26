





#ifndef GainProcessor_h_
#define GainProcessor_h_

#include "AudioNodeStream.h"
#include "AudioDestinationNode.h"
#include "WebAudioUtils.h"

namespace mozilla {
namespace dom {



class GainProcessor
{
public:
  explicit GainProcessor(AudioDestinationNode* aDestination)
    : mSource(nullptr)
    , mDestination(static_cast<AudioNodeStream*> (aDestination->Stream()))
    , mGain(1.f)
  {
  }

  void SetSourceStream(AudioNodeStream* aSource)
  {
    mSource = aSource;
  }

  void SetGainParameter(const AudioParamTimeline& aValue)
  {
    MOZ_ASSERT(mSource && mDestination);
    mGain = aValue;
    WebAudioUtils::ConvertAudioParamToTicks(mGain, mSource, mDestination);
  }

  void ProcessGain(AudioNodeStream* aStream,
                   float aInputVolume,
                   const nsTArray<const void*>& aInputChannelData,
                   AudioChunk* aOutput)
  {
    MOZ_ASSERT(mSource == aStream, "Invalid source stream");

    if (mGain.HasSimpleValue()) {
      
      aOutput->mVolume *= mGain.GetValue();
    } else {
      
      
      

      
      
      float computedGain[WEBAUDIO_BLOCK_SIZE];
      for (size_t counter = 0; counter < WEBAUDIO_BLOCK_SIZE; ++counter) {
        TrackTicks tick = aStream->GetCurrentPosition();
        computedGain[counter] = mGain.GetValueAtTime(tick, counter) * aInputVolume;
      }

      
      MOZ_ASSERT(aInputChannelData.Length() == aOutput->mChannelData.Length());
      for (size_t channel = 0; channel < aOutput->mChannelData.Length(); ++channel) {
        const float* inputBuffer = static_cast<const float*> (aInputChannelData[channel]);
        float* buffer = static_cast<float*> (const_cast<void*>
                          (aOutput->mChannelData[channel]));
        AudioBlockCopyChannelWithScale(inputBuffer, computedGain, buffer);
      }
    }
  }

protected:
  AudioNodeStream* mSource;
  AudioNodeStream* mDestination;
  AudioParamTimeline mGain;
};

}
}

#endif


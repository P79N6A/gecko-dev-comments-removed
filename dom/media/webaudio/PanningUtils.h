





#ifndef PANNING_UTILS_H
#define PANNING_UTILS_H

#include "AudioSegment.h"
#include "AudioNodeEngine.h"

namespace mozilla {
namespace dom {

template<typename T>
void
GainMonoToStereo(const AudioChunk& aInput, AudioChunk* aOutput,
                 T aGainL, T aGainR)
{
  float* outputL = static_cast<float*>(const_cast<void*>(aOutput->mChannelData[0]));
  float* outputR = static_cast<float*>(const_cast<void*>(aOutput->mChannelData[1]));
  const float* input = static_cast<float*>(const_cast<void*>(aInput.mChannelData[0]));

  MOZ_ASSERT(aInput.ChannelCount() == 1);
  MOZ_ASSERT(aOutput->ChannelCount() == 2);

  AudioBlockPanMonoToStereo(input, aGainL, aGainR, outputL, outputR);
}



template<typename T, typename U>
void
GainStereoToStereo(const AudioChunk& aInput, AudioChunk* aOutput,
                   T aGainL, T aGainR, U aOnLeft)
{
  float* outputL = static_cast<float*>(const_cast<void*>(aOutput->mChannelData[0]));
  float* outputR = static_cast<float*>(const_cast<void*>(aOutput->mChannelData[1]));
  const float* inputL = static_cast<float*>(const_cast<void*>(aInput.mChannelData[0]));
  const float* inputR = static_cast<float*>(const_cast<void*>(aInput.mChannelData[1]));

  MOZ_ASSERT(aInput.ChannelCount() == 2);
  MOZ_ASSERT(aOutput->ChannelCount() == 2);

  AudioBlockPanStereoToStereo(inputL, inputR, aGainL, aGainR, aOnLeft, outputL, outputR);
}



template<typename T, typename U>
void ApplyStereoPanning(const AudioChunk& aInput, AudioChunk* aOutput,
                        T aGainL, T aGainR, U aOnLeft)
{
  if (aInput.mChannelData.Length() == 1) {
    GainMonoToStereo(aInput, aOutput, aGainL, aGainR);
  } else {
    GainStereoToStereo(aInput, aOutput, aGainL, aGainR, aOnLeft);
  }
}

} 
} 

#endif 

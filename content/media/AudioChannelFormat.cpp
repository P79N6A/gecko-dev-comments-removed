




#include "AudioChannelFormat.h"

#include <algorithm>

namespace mozilla {

enum {
  SURROUND_L,
  SURROUND_R,
  SURROUND_C,
  SURROUND_LFE,
  SURROUND_SL,
  SURROUND_SR
};

static const uint32_t CUSTOM_CHANNEL_LAYOUTS = 6;

uint32_t
GetAudioChannelsSuperset(uint32_t aChannels1, uint32_t aChannels2)
{
  if (aChannels1 == 3 && aChannels2 == 4) {
    
    
    return 5;
  }
  return std::max(aChannels1, aChannels2);
}

void
AudioChannelsUpMix(nsTArray<const void*>* aChannelArray,
                   uint32_t aOutputChannelCount,
                   const void* aZeroChannel)
{
  uint32_t inputChannelCount = aChannelArray->Length();
  uint32_t outputChannelCount =
    GetAudioChannelsSuperset(aOutputChannelCount, inputChannelCount);
  NS_ASSERTION(outputChannelCount > inputChannelCount,
               "No up-mix needed");
  NS_ASSERTION(inputChannelCount > 0, "Bad number of channels");
  NS_ASSERTION(outputChannelCount > 0, "Bad number of channels");

  aChannelArray->SetLength(outputChannelCount);

  if (inputChannelCount < CUSTOM_CHANNEL_LAYOUTS) {
    const void* surroundChannels[CUSTOM_CHANNEL_LAYOUTS] =
      { aZeroChannel, aZeroChannel, aZeroChannel,
        aZeroChannel, aZeroChannel, aZeroChannel
      };
    
    switch (inputChannelCount) {
    case 1:
      surroundChannels[SURROUND_C] = aChannelArray->ElementAt(0);
      break;
    case 2:
      surroundChannels[SURROUND_L] = aChannelArray->ElementAt(0);
      surroundChannels[SURROUND_R] = aChannelArray->ElementAt(1);
      break;
    case 3:
      surroundChannels[SURROUND_L] = aChannelArray->ElementAt(0);
      surroundChannels[SURROUND_R] = aChannelArray->ElementAt(1);
      surroundChannels[SURROUND_C] = aChannelArray->ElementAt(2);
      break;
    case 4:
      surroundChannels[SURROUND_L] = aChannelArray->ElementAt(0);
      surroundChannels[SURROUND_R] = aChannelArray->ElementAt(1);
      surroundChannels[SURROUND_SL] = aChannelArray->ElementAt(2);
      surroundChannels[SURROUND_SR] = aChannelArray->ElementAt(3);
      break;
    case 5:
      surroundChannels[SURROUND_L] = aChannelArray->ElementAt(0);
      surroundChannels[SURROUND_R] = aChannelArray->ElementAt(1);
      surroundChannels[SURROUND_C] = aChannelArray->ElementAt(2);
      surroundChannels[SURROUND_SL] = aChannelArray->ElementAt(3);
      surroundChannels[SURROUND_SR] = aChannelArray->ElementAt(4);
      break;
    }

    if (outputChannelCount < CUSTOM_CHANNEL_LAYOUTS) {
      
      switch (outputChannelCount) {
      case 2:
        
        aChannelArray->ElementAt(0) = surroundChannels[SURROUND_C];
        aChannelArray->ElementAt(1) = surroundChannels[SURROUND_C];
        break;
      case 3:
        aChannelArray->ElementAt(0) = surroundChannels[SURROUND_L];
        aChannelArray->ElementAt(1) = surroundChannels[SURROUND_R];
        aChannelArray->ElementAt(2) = surroundChannels[SURROUND_C];
        break;
      case 4:
        
        NS_ASSERTION(inputChannelCount != 3,
                     "3->4 upmix not supported directly");
        if (inputChannelCount == 1) {
          
          
          aChannelArray->ElementAt(0) = surroundChannels[SURROUND_C];
          aChannelArray->ElementAt(1) = surroundChannels[SURROUND_C];
        } else {
          aChannelArray->ElementAt(0) = surroundChannels[SURROUND_L];
          aChannelArray->ElementAt(1) = surroundChannels[SURROUND_R];
        }
        aChannelArray->ElementAt(2) = surroundChannels[SURROUND_SL];
        aChannelArray->ElementAt(3) = surroundChannels[SURROUND_SR];
        break;
      case 5:
        aChannelArray->ElementAt(0) = surroundChannels[SURROUND_L];
        aChannelArray->ElementAt(1) = surroundChannels[SURROUND_R];
        aChannelArray->ElementAt(2) = surroundChannels[SURROUND_C];
        aChannelArray->ElementAt(3) = surroundChannels[SURROUND_SL];
        aChannelArray->ElementAt(4) = surroundChannels[SURROUND_SR];
      }
      return;
    }

    memcpy(aChannelArray->Elements(), surroundChannels, sizeof(surroundChannels));
    inputChannelCount = CUSTOM_CHANNEL_LAYOUTS;
  }

  for (uint32_t i = inputChannelCount; i < outputChannelCount; ++i) {
    aChannelArray->ElementAt(i) = aZeroChannel;
  }
}








struct DownMixMatrix {
  
  
  uint8_t mInputDestination[CUSTOM_CHANNEL_LAYOUTS];
  
  
  uint8_t mCExtraDestination;
  float mInputCoefficient[CUSTOM_CHANNEL_LAYOUTS];
};

static const int IGNORE = CUSTOM_CHANNEL_LAYOUTS;
static const float IGNORE_F = 0.0f;

static const DownMixMatrix
gDownMixMatrices[CUSTOM_CHANNEL_LAYOUTS*(CUSTOM_CHANNEL_LAYOUTS - 1)/2] =
{
  
  { { 0, 0 }, IGNORE, { 0.5f, 0.5f } },
  { { 0, 0, 0 }, IGNORE, { 0.3333f, 0.3333f, 0.3333f } },
  { { 0, 0, 0, 0 }, IGNORE, { 0.25f, 0.25f, 0.25f, 0.25f } },
  { { 0, 0, 0, 0, 0 }, IGNORE, { 0.7071f, 0.7071f, 1.0f, 0.5f, 0.5f } },
  { { 0, 0, 0, IGNORE, 0, 0 }, IGNORE, { 0.7071f, 0.7071f, 1.0f, IGNORE_F, 0.5f, 0.5f } },
  
  { { 0, 1, 0 }, 1, { 1.0f, 1.0f, 0.7071f } },
  { { 0, 1, 0, 1 }, IGNORE, { 0.5f, 0.5f, 0.5f, 0.5f } },
  { { 0, 1, 0, 0, 1 }, 1, { 1.0f, 1.0f, 0.7071f, 0.7071f, 0.7071f } },
  { { 0, 1, 0, IGNORE, 0, 1 }, 1, { 1.0f, 1.0f, 0.7071f, IGNORE_F, 0.7071f, 0.7071f } },
  
  { { 0, 1, 0, 1 }, IGNORE, { 0.25f, 0.25f, 0.25f, 0.25f } },
  { { 0, 1, 2, 0, 1 }, IGNORE, { 0.5f, 0.5f, 1.0f, 0.5f, 0.5f } },
  { { 0, 1, 2, IGNORE, 0, 1 }, IGNORE, { 0.5f, 0.5f, 1.0f, IGNORE_F, 0.5f, 0.5f } },
  
  { { 0, 1, 0, 2, 3 }, 1, { 1.0f, 1.0f, 0.7071f, 1.0f, 1.0f } },
  { { 0, 1, 0, IGNORE, 2, 3 }, 1, { 1.0f, 1.0f, 0.7071f, IGNORE_F, 1.0f, 1.0f } },
  
  { { 0, 1, 2, IGNORE, 3, 4 }, IGNORE, { 1.0f, 1.0f, 1.0f, IGNORE_F, 1.0f, 1.0f } }
};

static const int gDownMixMatrixIndexByOutputChannels[CUSTOM_CHANNEL_LAYOUTS - 1] =
  { 0, 5, 9, 12, 14 };

void
AudioChannelsDownMix(const nsTArray<const void*>& aChannelArray,
                     float** aOutputChannels,
                     uint32_t aOutputChannelCount,
                     uint32_t aDuration)
{
  uint32_t inputChannelCount = aChannelArray.Length();
  const void* const* inputChannels = aChannelArray.Elements();
  NS_ASSERTION(inputChannelCount > aOutputChannelCount, "Nothing to do");

  if (aOutputChannelCount >= 6) {
    
    for (uint32_t o = 0; o < aOutputChannelCount; ++o) {
      memcpy(aOutputChannels[o], inputChannels[o], aDuration*sizeof(float));
    }
    return;
  }

  
  inputChannelCount = std::min<uint32_t>(6, inputChannelCount);

  const DownMixMatrix& m = gDownMixMatrices[
    gDownMixMatrixIndexByOutputChannels[aOutputChannelCount - 1] +
    inputChannelCount - aOutputChannelCount - 1];

  
  
  for (uint32_t s = 0; s < aDuration; ++s) {
    
    
    float outputChannels[CUSTOM_CHANNEL_LAYOUTS];
    memset(outputChannels, 0, sizeof(float)*(CUSTOM_CHANNEL_LAYOUTS - 1));
    for (uint32_t c = 0; c < inputChannelCount; ++c) {
      outputChannels[m.mInputDestination[c]] +=
        m.mInputCoefficient[c]*(static_cast<const float*>(inputChannels[c]))[s];
    }
    
    if (m.mCExtraDestination != IGNORE) {
      outputChannels[m.mCExtraDestination] +=
        m.mInputCoefficient[SURROUND_C]*(static_cast<const float*>(inputChannels[SURROUND_C]))[s];
    }

    for (uint32_t c = 0; c < aOutputChannelCount; ++c) {
      aOutputChannels[c][s] = outputChannels[c];
    }
  }
}

}

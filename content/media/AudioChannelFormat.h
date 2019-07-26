




#ifndef MOZILLA_AUDIOCHANNELFORMAT_H_
#define MOZILLA_AUDIOCHANNELFORMAT_H_

#include "nsTArray.h"

namespace mozilla {






















uint32_t
GetAudioChannelsSuperset(uint32_t aChannels1, uint32_t aChannels2);













void
AudioChannelsUpMix(nsTArray<const void*>* aChannelArray,
                   uint32_t aOutputChannelCount,
                   const void* aZeroChannel);







void
AudioChannelsDownMix(const nsTArray<const void*>& aChannelArray,
                     float** aOutputChannels,
                     uint32_t aOutputChannelCount,
                     uint32_t aDuration);



} 

#endif 

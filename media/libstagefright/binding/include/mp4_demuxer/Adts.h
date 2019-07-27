



#ifndef ADTS_H_
#define ADTS_H_

#include <stdint.h>

namespace mozilla {
class MediaRawData;
}

namespace mp4_demuxer
{

class Adts
{
public:
  static int8_t GetFrequencyIndex(uint32_t aSamplesPerSecond);
  static bool ConvertSample(uint16_t aChannelCount, int8_t aFrequencyIndex,
                            int8_t aProfile, mozilla::MediaRawData* aSample);
};
}

#endif

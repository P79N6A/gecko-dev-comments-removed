



#include "mp4_demuxer/Adts.h"
#include "mp4_demuxer/DecoderData.h"
#include "media/stagefright/MediaBuffer.h"

namespace mp4_demuxer
{

int8_t
Adts::GetFrequencyIndex(uint16_t aSamplesPerSecond)
{
  static const int freq_lookup[] = { 96000, 88200, 64000, 48000, 44100,
                                     32000, 24000, 22050, 16000, 12000,
                                     11025, 8000,  7350,  0 };

  int8_t i = 0;
  while (aSamplesPerSecond < freq_lookup[i]) {
    i++;
  }

  if (!freq_lookup[i]) {
    return -1;
  }

  return i;
}

bool
Adts::ConvertEsdsToAdts(uint16_t aChannelCount, int8_t aFrequencyIndex,
                        int8_t aProfile, MP4Sample* aSample)
{
  static const int kADTSHeaderSize = 7;

  size_t newSize = aSample->size + kADTSHeaderSize;

  
  if (newSize > aSample->mMediaBuffer->size() || newSize >= (1 << 13) ||
      aChannelCount < 0 || aChannelCount > 15 || aFrequencyIndex < 0 ||
      aProfile < 1 || aProfile > 4)
    return false;

  
  aSample->adts_buffer = new uint8_t[newSize];
  memcpy(aSample->adts_buffer + kADTSHeaderSize, aSample->data, aSample->size);
  aSample->data = aSample->adts_buffer;

  aSample->size = newSize;
  aSample->data[0] = 0xff;
  aSample->data[1] = 0xf1;
  aSample->data[2] =
    ((aProfile - 1) << 6) + (aFrequencyIndex << 2) + (aChannelCount >> 2);
  aSample->data[3] = ((aChannelCount & 0x3) << 6) + (newSize >> 11);
  aSample->data[4] = (newSize & 0x7ff) >> 3;
  aSample->data[5] = ((newSize & 7) << 5) + 0x1f;
  aSample->data[6] = 0xfc;

  return true;
}
}

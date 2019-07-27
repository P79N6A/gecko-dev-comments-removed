















#ifndef GMP_AUDIO_CODEC_h_
#define GMP_AUDIO_CODEC_h_

#include <stdint.h>

enum GMPAudioCodecType
{
  kGMPAudioCodecAAC,
  kGMPAudioCodecVorbis,
  kGMPAudioCodecInvalid 
};

struct GMPAudioCodec
{
  GMPAudioCodecType mCodecType;
  uint32_t mChannelCount;
  uint32_t mBitsPerChannel;
  uint32_t mSamplesPerSecond;

  
  
  
  const uint8_t* mExtraData;
  uint32_t       mExtraDataLen;
};

#endif 

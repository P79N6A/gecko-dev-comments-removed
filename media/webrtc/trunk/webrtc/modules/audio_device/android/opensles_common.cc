









#include "webrtc/modules/audio_device/android/opensles_common.h"

#include <assert.h>

#include "webrtc/modules/audio_device/android/audio_common.h"

using webrtc::kNumChannels;

namespace webrtc_opensl {

SLDataFormat_PCM CreatePcmConfiguration(int sample_rate) {
  SLDataFormat_PCM configuration;
  configuration.formatType = SL_DATAFORMAT_PCM;
  configuration.numChannels = kNumChannels;
  
  
  
  
  configuration.samplesPerSec = sample_rate * 1000;
  configuration.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
  configuration.containerSize = SL_PCMSAMPLEFORMAT_FIXED_16;
  configuration.channelMask = SL_SPEAKER_FRONT_CENTER;
  if (2 == configuration.numChannels) {
    configuration.channelMask =
        SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
  }
  configuration.endianness = SL_BYTEORDER_LITTLEENDIAN;
  return configuration;
}

}  

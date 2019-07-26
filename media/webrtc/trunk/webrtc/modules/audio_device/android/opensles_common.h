









#ifndef WEBRTC_MODULES_AUDIO_DEVICE_ANDROID_OPENSLES_COMMON_H_
#define WEBRTC_MODULES_AUDIO_DEVICE_ANDROID_OPENSLES_COMMON_H_

#include <SLES/OpenSLES.h>

namespace webrtc_opensl {

enum {
  kDefaultSampleRate = 44100,
  kNumChannels = 1
};


class PlayoutDelayProvider {
 public:
  virtual int PlayoutDelayMs() = 0;

 protected:
  PlayoutDelayProvider() {}
  virtual ~PlayoutDelayProvider() {}
};

SLDataFormat_PCM CreatePcmConfiguration(int sample_rate);

}  

#endif  

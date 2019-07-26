









#ifndef WEBRTC_MODULES_AUDIO_DEVICE_ANDROID_AUDIO_COMMON_H_
#define WEBRTC_MODULES_AUDIO_DEVICE_ANDROID_AUDIO_COMMON_H_

namespace webrtc {

enum {
  kDefaultSampleRate = 44100,
  kNumChannels = 1,
  kDefaultBufSizeInSamples = kDefaultSampleRate * 10 / 1000,
};

class PlayoutDelayProvider {
 public:
  virtual int PlayoutDelayMs() = 0;

 protected:
  PlayoutDelayProvider() {}
  virtual ~PlayoutDelayProvider() {}
};

}  

#endif  

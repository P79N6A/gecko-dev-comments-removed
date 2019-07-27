









#ifndef WEBRTC_COMMON_AUDIO_VAD_INCLUDE_VAD_H_
#define WEBRTC_COMMON_AUDIO_VAD_INCLUDE_VAD_H_

#include "webrtc/base/checks.h"
#include "webrtc/common_audio/vad/include/webrtc_vad.h"
#include "webrtc/typedefs.h"

namespace webrtc {


class Vad {
 public:
  enum Aggressiveness {
    kVadNormal = 0,
    kVadLowBitrate = 1,
    kVadAggressive = 2,
    kVadVeryAggressive = 3
  };

  enum Activity { kPassive = 0, kActive = 1, kError = -1 };

  explicit Vad(enum Aggressiveness mode);

  virtual ~Vad();

  enum Activity VoiceActivity(const int16_t* audio,
                              size_t num_samples,
                              int sample_rate_hz);

 private:
  VadInst* handle_;
};

}  
#endif

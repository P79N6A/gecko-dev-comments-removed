









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_RESAMPLER_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_RESAMPLER_H_

#include "webrtc/common_audio/resampler/include/push_resampler.h"
#include "webrtc/typedefs.h"

namespace webrtc {

namespace acm1 {

class ACMResampler {
 public:
  ACMResampler();
  ~ACMResampler();

  int16_t Resample10Msec(const int16_t* in_audio,
                         const int32_t in_freq_hz,
                         int16_t* out_audio,
                         const int32_t out_freq_hz,
                         uint8_t num_audio_channels);

 private:
  PushResampler resampler_;
};

}  

}  

#endif  

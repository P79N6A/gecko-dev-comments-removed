









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_ACM_RESAMPLER_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_ACM_RESAMPLER_H_

#include "webrtc/common_audio/resampler/include/push_resampler.h"
#include "webrtc/typedefs.h"

namespace webrtc {
namespace acm2 {

class ACMResampler {
 public:
  ACMResampler();
  ~ACMResampler();

  int Resample10Msec(const int16_t* in_audio,
                     int in_freq_hz,
                     int out_freq_hz,
                     int num_audio_channels,
                     int out_capacity_samples,
                     int16_t* out_audio);

 private:
  PushResampler<int16_t> resampler_;
};

}  
}  

#endif  

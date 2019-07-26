









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_ACM_RESAMPLER_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_ACM_RESAMPLER_H_

#include "webrtc/common_audio/resampler/include/resampler.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class CriticalSectionWrapper;

namespace acm2 {

class ACMResampler {
 public:
  ACMResampler();
  ~ACMResampler();

  int Resample10Msec(const int16_t* in_audio,
                     int in_freq_hz,
                     int out_freq_hz,
                     int num_audio_channels,
                     int16_t* out_audio);

 private:
  
  Resampler resampler_;
  CriticalSectionWrapper* resampler_crit_sect_;
};

}  

}  

#endif  

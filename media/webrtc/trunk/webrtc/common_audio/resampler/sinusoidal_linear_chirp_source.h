












#ifndef WEBRTC_COMMON_AUDIO_RESAMPLER_SINUSOIDAL_LINEAR_CHIRP_SOURCE_H_
#define WEBRTC_COMMON_AUDIO_RESAMPLER_SINUSOIDAL_LINEAR_CHIRP_SOURCE_H_

#include "webrtc/common_audio/resampler/sinc_resampler.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"

namespace webrtc {




class SinusoidalLinearChirpSource : public SincResamplerCallback {
 public:
  
  
  SinusoidalLinearChirpSource(int sample_rate, int samples,
                              double max_frequency, double delay_samples);

  virtual ~SinusoidalLinearChirpSource() {}

  virtual void Run(int frames, float* destination) OVERRIDE;

  double Frequency(int position);

 private:
  enum {
    kMinFrequency = 5
  };

  double sample_rate_;
  int total_samples_;
  double max_frequency_;
  double k_;
  int current_index_;
  double delay_samples_;

  DISALLOW_COPY_AND_ASSIGN(SinusoidalLinearChirpSource);
};

}  

#endif












#define _USE_MATH_DEFINES

#include "webrtc/common_audio/resampler/sinusoidal_linear_chirp_source.h"

#include <math.h>

namespace webrtc {

SinusoidalLinearChirpSource::SinusoidalLinearChirpSource(int sample_rate,
  int samples, double max_frequency, double delay_samples)
    : sample_rate_(sample_rate),
      total_samples_(samples),
      max_frequency_(max_frequency),
      current_index_(0),
      delay_samples_(delay_samples) {
  
  double duration = static_cast<double>(total_samples_) / sample_rate_;
  k_ = (max_frequency_ - kMinFrequency) / duration;
}

void SinusoidalLinearChirpSource::Run(int frames, float* destination) {
  for (int i = 0; i < frames; ++i, ++current_index_) {
    
    if (Frequency(current_index_) > 0.5 * sample_rate_) {
      destination[i] = 0;
    } else {
      
      double t = (static_cast<double>(current_index_) - delay_samples_) /
          sample_rate_;
      if (t < 0) {
        destination[i] = 0;
      } else {
        
        destination[i] =
            sin(2 * M_PI * (kMinFrequency * t + (k_ / 2) * t * t));
      }
    }
  }
}

double SinusoidalLinearChirpSource::Frequency(int position) {
  return kMinFrequency + (position - delay_samples_) *
      (max_frequency_ - kMinFrequency) / total_samples_;
}

}  

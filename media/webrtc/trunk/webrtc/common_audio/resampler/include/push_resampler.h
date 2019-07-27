









#ifndef WEBRTC_COMMON_AUDIO_RESAMPLER_INCLUDE_PUSH_RESAMPLER_H_
#define WEBRTC_COMMON_AUDIO_RESAMPLER_INCLUDE_PUSH_RESAMPLER_H_

#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class PushSincResampler;



template <typename T>
class PushResampler {
 public:
  PushResampler();
  virtual ~PushResampler();

  
  
  int InitializeIfNeeded(int src_sample_rate_hz, int dst_sample_rate_hz,
                         int num_channels);

  
  
  int Resample(const T* src, int src_length, T* dst, int dst_capacity);

 private:
  scoped_ptr<PushSincResampler> sinc_resampler_;
  scoped_ptr<PushSincResampler> sinc_resampler_right_;
  int src_sample_rate_hz_;
  int dst_sample_rate_hz_;
  int num_channels_;
  scoped_ptr<T[]> src_left_;
  scoped_ptr<T[]> src_right_;
  scoped_ptr<T[]> dst_left_;
  scoped_ptr<T[]> dst_right_;
};

}  

#endif  











#ifndef WEBRTC_COMMON_AUDIO_RESAMPLER_PUSH_SINC_RESAMPLER_H_
#define WEBRTC_COMMON_AUDIO_RESAMPLER_PUSH_SINC_RESAMPLER_H_

#include "webrtc/common_audio/resampler/sinc_resampler.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/typedefs.h"

namespace webrtc {



class PushSincResampler : public SincResamplerCallback {
 public:
  
  
  
  PushSincResampler(int src_block_size, int dst_block_size);
  virtual ~PushSincResampler();

  
  
  
  
  
  int Resample(const int16_t* source, int source_length,
               int16_t* destination, int destination_capacity);

  
  virtual void Run(float* destination, int frames);

 private:
  scoped_ptr<SincResampler> resampler_;
  scoped_array<float> float_buffer_;
  const int16_t* source_ptr_;
  const int dst_size_;

  DISALLOW_COPY_AND_ASSIGN(PushSincResampler);
};

}  

#endif  

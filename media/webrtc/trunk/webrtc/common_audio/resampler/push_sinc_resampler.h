









#ifndef WEBRTC_COMMON_AUDIO_RESAMPLER_PUSH_SINC_RESAMPLER_H_
#define WEBRTC_COMMON_AUDIO_RESAMPLER_PUSH_SINC_RESAMPLER_H_

#include "webrtc/common_audio/resampler/sinc_resampler.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/typedefs.h"

namespace webrtc {



class PushSincResampler : public SincResamplerCallback {
 public:
  
  
  
  PushSincResampler(int source_frames, int destination_frames);
  virtual ~PushSincResampler();

  
  
  
  
  
  int Resample(const int16_t* source, int source_frames,
               int16_t* destination, int destination_capacity);

  
  virtual void Run(int frames, float* destination) OVERRIDE;

  SincResampler* get_resampler_for_testing() { return resampler_.get(); }

 private:
  scoped_ptr<SincResampler> resampler_;
  scoped_array<float> float_buffer_;
  const int16_t* source_ptr_;
  const int destination_frames_;

  
  bool first_pass_;

  
  int source_available_;

  DISALLOW_COPY_AND_ASSIGN(PushSincResampler);
};

}  

#endif  

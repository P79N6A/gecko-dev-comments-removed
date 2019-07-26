









#include "webrtc/common_audio/include/audio_util.h"
#include "webrtc/common_audio/resampler/push_sinc_resampler.h"

#include <string.h>

namespace webrtc {

PushSincResampler::PushSincResampler(int source_frames,
                                     int destination_frames)
    : resampler_(new SincResampler(source_frames * 1.0 / destination_frames,
                                   source_frames, this)),
      float_buffer_(new float[destination_frames]),
      source_ptr_(NULL),
      destination_frames_(destination_frames),
      first_pass_(true),
      source_available_(0) {
}

PushSincResampler::~PushSincResampler() {
}

int PushSincResampler::Resample(const int16_t* source,
                                int source_length,
                                int16_t* destination,
                                int destination_capacity) {
  assert(source_length == resampler_->request_frames());
  assert(destination_capacity >= destination_frames_);
  
  
  source_ptr_ = source;
  source_available_ = source_length;

  
  
  
  
  
  
  
  
  
  
  
  
  
  if (first_pass_)
    resampler_->Resample(resampler_->ChunkSize(), float_buffer_.get());

  resampler_->Resample(destination_frames_, float_buffer_.get());
  for (int i = 0; i < destination_frames_; ++i)
    destination[i] = RoundToInt16(ClampInt16(float_buffer_[i]));
  source_ptr_ = NULL;
  return destination_frames_;
}

void PushSincResampler::Run(int frames, float* destination) {
  assert(source_ptr_ != NULL);
  
  
  assert(source_available_ == frames);

  if (first_pass_) {
    
    
    memset(destination, 0, frames * sizeof(float));
    first_pass_ = false;
  } else {
    for (int i = 0; i < frames; ++i)
      destination[i] = static_cast<float>(source_ptr_[i]);
    source_available_ -= frames;
  }
}

}  

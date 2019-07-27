









#include "webrtc/common_audio/include/audio_util.h"

#include <assert.h>
#include <string.h>

#include "webrtc/common_audio/resampler/push_sinc_resampler.h"

namespace webrtc {

PushSincResampler::PushSincResampler(int source_frames, int destination_frames)
    : resampler_(new SincResampler(source_frames * 1.0 / destination_frames,
                                   source_frames,
                                   this)),
      source_ptr_(NULL),
      source_ptr_int_(NULL),
      destination_frames_(destination_frames),
      first_pass_(true),
      source_available_(0) {}

PushSincResampler::~PushSincResampler() {
}

int PushSincResampler::Resample(const int16_t* source,
                                int source_length,
                                int16_t* destination,
                                int destination_capacity) {
  if (!float_buffer_.get())
    float_buffer_.reset(new float[destination_frames_]);

  source_ptr_int_ = source;
  
  Resample(NULL, source_length, float_buffer_.get(), destination_frames_);
  FloatS16ToS16(float_buffer_.get(), destination_frames_, destination);
  source_ptr_int_ = NULL;
  return destination_frames_;
}

int PushSincResampler::Resample(const float* source,
                                int source_length,
                                float* destination,
                                int destination_capacity) {
  assert(source_length == resampler_->request_frames());
  assert(destination_capacity >= destination_frames_);
  
  
  source_ptr_ = source;
  source_available_ = source_length;

  
  
  
  
  
  
  
  
  
  
  
  
  
  if (first_pass_)
    resampler_->Resample(resampler_->ChunkSize(), destination);

  resampler_->Resample(destination_frames_, destination);
  source_ptr_ = NULL;
  return destination_frames_;
}

void PushSincResampler::Run(int frames, float* destination) {
  
  
  assert(source_available_ == frames);

  if (first_pass_) {
    
    
    memset(destination, 0, frames * sizeof(float));
    first_pass_ = false;
    return;
  }

  if (source_ptr_) {
    memcpy(destination, source_ptr_, frames * sizeof(float));
  } else {
    for (int i = 0; i < frames; ++i)
      destination[i] = static_cast<float>(source_ptr_int_[i]);
  }
  source_available_ -= frames;
}

}  

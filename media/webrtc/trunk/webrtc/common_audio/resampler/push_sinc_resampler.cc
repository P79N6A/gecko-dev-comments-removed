









#include "webrtc/common_audio/resampler/push_sinc_resampler.h"

#include <cmath>

#include <algorithm>

namespace webrtc {

PushSincResampler::PushSincResampler(int src_block_size,
                                     int dst_block_size)
    : resampler_(NULL),
      float_buffer_(NULL),
      source_ptr_(NULL),
      dst_size_(dst_block_size) {
  resampler_.reset(new SincResampler(src_block_size * 1.0 / dst_block_size,
                                     this, src_block_size));
  float_buffer_.reset(new float[dst_block_size]);
}

PushSincResampler::~PushSincResampler() {
}

int PushSincResampler::Resample(const int16_t* source,
                                int source_length,
                                int16_t* destination,
                                int destination_capacity) {
  assert(source_length == resampler_->BlockSize());
  assert(destination_capacity >= dst_size_);
  
  
  source_ptr_ = source;
  resampler_->Resample(float_buffer_.get(), dst_size_);
  for (int i = 0; i < dst_size_; ++i) {
    float clipped = std::max(std::min(float_buffer_[i], 32767.0f), -32768.0f);
    destination[i] = static_cast<int16_t>(std::floor(clipped + 0.5));
  }
  source_ptr_ = NULL;
  return dst_size_;
}

void PushSincResampler::Run(float* destination, int frames) {
  assert(source_ptr_ != NULL);
  assert(frames >= resampler_->BlockSize());
  
  
  
  int i = 0;
  for (; i < frames - resampler_->BlockSize(); ++i) {
    destination[i] = 0;
  }
  for (int j = 0; i < frames; ++i, ++j) {
    destination[i] = static_cast<float>(source_ptr_[j]);
  }
}

}  

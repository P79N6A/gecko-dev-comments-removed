









#include "webrtc/modules/audio_device/android/fine_audio_buffer.h"

#include <memory.h>
#include <stdio.h>
#include <algorithm>

#include "webrtc/modules/audio_device/audio_device_buffer.h"

namespace webrtc {

FineAudioBuffer::FineAudioBuffer(AudioDeviceBuffer* device_buffer,
                                 int desired_frame_size_bytes,
                                 int sample_rate)
    : device_buffer_(device_buffer),
      desired_frame_size_bytes_(desired_frame_size_bytes),
      sample_rate_(sample_rate),
      samples_per_10_ms_(sample_rate_ * 10 / 1000),
      bytes_per_10_ms_(samples_per_10_ms_ * sizeof(int16_t)),
      cached_buffer_start_(0),
      cached_bytes_(0) {
  cache_buffer_.reset(new int8_t[bytes_per_10_ms_]);
}

FineAudioBuffer::~FineAudioBuffer() {
}

int FineAudioBuffer::RequiredBufferSizeBytes() {
  
  
  
  return desired_frame_size_bytes_ + bytes_per_10_ms_;
}

void FineAudioBuffer::GetBufferData(int8_t* buffer) {
  if (desired_frame_size_bytes_ <= cached_bytes_) {
    memcpy(buffer, &cache_buffer_.get()[cached_buffer_start_],
           desired_frame_size_bytes_);
    cached_buffer_start_ += desired_frame_size_bytes_;
    cached_bytes_ -= desired_frame_size_bytes_;
    assert(cached_buffer_start_ + cached_bytes_ < bytes_per_10_ms_);
    return;
  }
  memcpy(buffer, &cache_buffer_.get()[cached_buffer_start_], cached_bytes_);
  
  
  
  int8_t* unwritten_buffer = &buffer[cached_bytes_];
  int bytes_left = desired_frame_size_bytes_ - cached_bytes_;
  
  int number_of_requests = 1 + (bytes_left - 1) / (bytes_per_10_ms_);
  for (int i = 0; i < number_of_requests; ++i) {
    device_buffer_->RequestPlayoutData(samples_per_10_ms_);
    int num_out = device_buffer_->GetPlayoutData(unwritten_buffer);
    if (num_out != samples_per_10_ms_) {
      assert(num_out == 0);
      cached_bytes_ = 0;
      return;
    }
    unwritten_buffer += bytes_per_10_ms_;
    assert(bytes_left >= 0);
    bytes_left -= bytes_per_10_ms_;
  }
  assert(bytes_left <= 0);
  
  
  int cache_location = desired_frame_size_bytes_;
  int8_t* cache_ptr = &buffer[cache_location];
  cached_bytes_ = number_of_requests * bytes_per_10_ms_ -
      (desired_frame_size_bytes_ - cached_bytes_);
  
  
  assert(cached_bytes_ <= bytes_per_10_ms_);
  assert(-bytes_left == cached_bytes_);
  cached_buffer_start_ = 0;
  memcpy(cache_buffer_.get(), cache_ptr, cached_bytes_);
}

}  

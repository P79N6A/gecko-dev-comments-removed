









#ifndef WEBRTC_MODULES_AUDIO_DEVICE_ANDROID_FINE_AUDIO_BUFFER_H_
#define WEBRTC_MODULES_AUDIO_DEVICE_ANDROID_FINE_AUDIO_BUFFER_H_

#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class AudioDeviceBuffer;






class FineAudioBuffer {
 public:
  
  
  
  
  
  
  FineAudioBuffer(AudioDeviceBuffer* device_buffer,
                  int desired_frame_size_bytes,
                  int sample_rate);
  ~FineAudioBuffer();

  
  
  
  
  int RequiredBufferSizeBytes();

  
  
  void GetBufferData(int8_t* buffer);

 private:
  
  AudioDeviceBuffer* device_buffer_;
  int desired_frame_size_bytes_;  
  int sample_rate_;
  int samples_per_10_ms_;
  
  int bytes_per_10_ms_;

  
  scoped_array<int8_t> cache_buffer_;
  int cached_buffer_start_;  
  int cached_bytes_;  
};

}  

#endif  

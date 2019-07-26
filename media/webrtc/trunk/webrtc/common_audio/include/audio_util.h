









#ifndef WEBRTC_COMMON_AUDIO_INCLUDE_AUDIO_UTIL_H_
#define WEBRTC_COMMON_AUDIO_INCLUDE_AUDIO_UTIL_H_

#include "webrtc/typedefs.h"

namespace webrtc {


static inline float ClampInt16(float value) {
  const float kMaxInt16 = 32767.f;
  const float kMinInt16 = -32768.f;
  return value < kMinInt16 ? kMinInt16 :
      (value > kMaxInt16 ? kMaxInt16 : value);
}



static inline int16_t RoundToInt16(float value) {
  return static_cast<int16_t>(value < 0.f ? value - 0.5f : value + 0.5f);
}





void Deinterleave(const int16_t* interleaved, int samples_per_channel,
                  int num_channels, int16_t** deinterleaved);




void Interleave(const int16_t* const* deinterleaved, int samples_per_channel,
                int num_channels, int16_t* interleaved);

}  

#endif  

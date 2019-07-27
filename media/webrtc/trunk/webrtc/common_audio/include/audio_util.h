









#ifndef WEBRTC_COMMON_AUDIO_INCLUDE_AUDIO_UTIL_H_
#define WEBRTC_COMMON_AUDIO_INCLUDE_AUDIO_UTIL_H_

#include <limits>

#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/typedefs.h"

namespace webrtc {

typedef std::numeric_limits<int16_t> limits_int16;





static inline int16_t FloatToS16(float v) {
  if (v > 0)
    return v >= 1 ? limits_int16::max() :
                    static_cast<int16_t>(v * limits_int16::max() + 0.5f);
  return v <= -1 ? limits_int16::min() :
                   static_cast<int16_t>(-v * limits_int16::min() - 0.5f);
}

static inline float S16ToFloat(int16_t v) {
  static const float kMaxInt16Inverse = 1.f / limits_int16::max();
  static const float kMinInt16Inverse = 1.f / limits_int16::min();
  return v * (v > 0 ? kMaxInt16Inverse : -kMinInt16Inverse);
}

static inline int16_t FloatS16ToS16(float v) {
  static const float kMaxRound = limits_int16::max() - 0.5f;
  static const float kMinRound = limits_int16::min() + 0.5f;
  if (v > 0)
    return v >= kMaxRound ? limits_int16::max() :
                            static_cast<int16_t>(v + 0.5f);
  return v <= kMinRound ? limits_int16::min() :
                          static_cast<int16_t>(v - 0.5f);
}

static inline float FloatToFloatS16(float v) {
  return v * (v > 0 ? limits_int16::max() : -limits_int16::min());
}

static inline float FloatS16ToFloat(float v) {
  static const float kMaxInt16Inverse = 1.f / limits_int16::max();
  static const float kMinInt16Inverse = 1.f / limits_int16::min();
  return v * (v > 0 ? kMaxInt16Inverse : -kMinInt16Inverse);
}

void FloatToS16(const float* src, size_t size, int16_t* dest);
void S16ToFloat(const int16_t* src, size_t size, float* dest);
void FloatS16ToS16(const float* src, size_t size, int16_t* dest);
void FloatToFloatS16(const float* src, size_t size, float* dest);
void FloatS16ToFloat(const float* src, size_t size, float* dest);





template <typename T>
void Deinterleave(const T* interleaved, int samples_per_channel,
                  int num_channels, T* const* deinterleaved) {
  for (int i = 0; i < num_channels; ++i) {
    T* channel = deinterleaved[i];
    int interleaved_idx = i;
    for (int j = 0; j < samples_per_channel; ++j) {
      channel[j] = interleaved[interleaved_idx];
      interleaved_idx += num_channels;
    }
  }
}




template <typename T>
void Interleave(const T* const* deinterleaved, int samples_per_channel,
                int num_channels, T* interleaved) {
  for (int i = 0; i < num_channels; ++i) {
    const T* channel = deinterleaved[i];
    int interleaved_idx = i;
    for (int j = 0; j < samples_per_channel; ++j) {
      interleaved[interleaved_idx] = channel[j];
      interleaved_idx += num_channels;
    }
  }
}

}  

#endif  

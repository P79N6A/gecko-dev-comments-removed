









#include "webrtc/common_audio/include/audio_util.h"

#include "webrtc/typedefs.h"

namespace webrtc {

void FloatToS16(const float* src, size_t size, int16_t* dest) {
  for (size_t i = 0; i < size; ++i)
    dest[i] = FloatToS16(src[i]);
}

void S16ToFloat(const int16_t* src, size_t size, float* dest) {
  for (size_t i = 0; i < size; ++i)
    dest[i] = S16ToFloat(src[i]);
}

void FloatS16ToS16(const float* src, size_t size, int16_t* dest) {
  for (size_t i = 0; i < size; ++i)
    dest[i] = FloatS16ToS16(src[i]);
}

void FloatToFloatS16(const float* src, size_t size, float* dest) {
  for (size_t i = 0; i < size; ++i)
    dest[i] = FloatToFloatS16(src[i]);
}

void FloatS16ToFloat(const float* src, size_t size, float* dest) {
  for (size_t i = 0; i < size; ++i)
    dest[i] = FloatS16ToFloat(src[i]);
}

}  

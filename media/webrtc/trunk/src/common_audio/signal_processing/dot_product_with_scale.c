









#include "signal_processing_library.h"

int32_t WebRtcSpl_DotProductWithScale(const int16_t* vector1,
                                      const int16_t* vector2,
                                      int length,
                                      int scaling) {
  int32_t sum = 0;
  int i = 0;

  
  for (i = 0; i < length - 3; i += 4) {
    sum += (vector1[i + 0] * vector2[i + 0]) >> scaling;
    sum += (vector1[i + 1] * vector2[i + 1]) >> scaling;
    sum += (vector1[i + 2] * vector2[i + 2]) >> scaling;
    sum += (vector1[i + 3] * vector2[i + 3]) >> scaling;
  }
  for (; i < length; i++) {
    sum += (vector1[i] * vector2[i]) >> scaling;
  }

  return sum;
}

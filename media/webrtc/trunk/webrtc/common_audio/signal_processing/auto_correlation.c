









#include "signal_processing_library.h"

int WebRtcSpl_AutoCorrelation(const int16_t* in_vector,
                              int in_vector_length,
                              int order,
                              int32_t* result,
                              int* scale) {
  int32_t sum = 0;
  int i = 0, j = 0;
  int16_t smax = 0;
  int scaling = 0;

  if (order > in_vector_length) {
    
    return -1;
  } else if (order < 0) {
    order = in_vector_length;
  }

  
  smax = WebRtcSpl_MaxAbsValueW16(in_vector, in_vector_length);

  
  
  if (smax == 0) {
    scaling = 0;
  } else {
    
    int nbits = WebRtcSpl_GetSizeInBits(in_vector_length);
    
    int t = WebRtcSpl_NormW32(WEBRTC_SPL_MUL(smax, smax));

    if (t > nbits) {
      scaling = 0;
    } else {
      scaling = nbits - t;
    }
  }

  
  for (i = 0; i < order + 1; i++) {
    sum = 0;
    
    for (j = 0; j < in_vector_length - i - 3; j += 4) {
      sum += (in_vector[j + 0] * in_vector[i + j + 0]) >> scaling;
      sum += (in_vector[j + 1] * in_vector[i + j + 1]) >> scaling;
      sum += (in_vector[j + 2] * in_vector[i + j + 2]) >> scaling;
      sum += (in_vector[j + 3] * in_vector[i + j + 3]) >> scaling;
    }
    for (; j < in_vector_length - i; j++) {
      sum += (in_vector[j] * in_vector[i + j]) >> scaling;
    }
    *result++ = sum;
  }

  *scale = scaling;
  return order + 1;
}

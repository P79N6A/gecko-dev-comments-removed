









#include "webrtc/common_audio/signal_processing/include/real_fft.h"

#include <stdlib.h>

#include "webrtc/common_audio/signal_processing/include/signal_processing_library.h"

struct RealFFT {
  int order;
};

struct RealFFT* WebRtcSpl_CreateRealFFTC(int order) {
  struct RealFFT* self = NULL;

  if (order > kMaxFFTOrder || order < 0) {
    return NULL;
  }

  self = malloc(sizeof(struct RealFFT));
  if (self == NULL) {
    return NULL;
  }
  self->order = order;

  return self;
}

void WebRtcSpl_FreeRealFFTC(struct RealFFT* self) {
  if (self != NULL) {
    free(self);
  }
}





int WebRtcSpl_RealForwardFFTC(struct RealFFT* self,
                              const int16_t* real_data_in,
                              int16_t* complex_data_out) {
  int i = 0;
  int j = 0;
  int result = 0;
  int n = 1 << self->order;
  
  
  int16_t complex_buffer[2 << kMaxFFTOrder];

  
  for (i = 0, j = 0; i < n; i += 1, j += 2) {
    complex_buffer[j] = real_data_in[i];
    complex_buffer[j + 1] = 0;
  };

  WebRtcSpl_ComplexBitReverse(complex_buffer, self->order);
  result = WebRtcSpl_ComplexFFT(complex_buffer, self->order, 1);

  
  
  memcpy(complex_data_out, complex_buffer, sizeof(int16_t) * (n + 2));

  return result;
}

int WebRtcSpl_RealInverseFFTC(struct RealFFT* self,
                              const int16_t* complex_data_in,
                              int16_t* real_data_out) {
  int i = 0;
  int j = 0;
  int result = 0;
  int n = 1 << self->order;
  
  int16_t complex_buffer[2 << kMaxFFTOrder];

  
  
  
  memcpy(complex_buffer, complex_data_in, sizeof(int16_t) * (n + 2));
  for (i = n + 2; i < 2 * n; i += 2) {
    complex_buffer[i] = complex_data_in[2 * n - i];
    complex_buffer[i + 1] = -complex_data_in[2 * n - i + 1];
  }

  WebRtcSpl_ComplexBitReverse(complex_buffer, self->order);
  result = WebRtcSpl_ComplexIFFT(complex_buffer, self->order, 1);

  
  for (i = 0, j = 0; i < n; i += 1, j += 2) {
    real_data_out[i] = complex_buffer[j];
  }

  return result;
}

#if defined(WEBRTC_DETECT_ARM_NEON) || defined(WEBRTC_ARCH_ARM_NEON)


struct RealFFT* WebRtcSpl_CreateRealFFTNeon(int order) {
  return WebRtcSpl_CreateRealFFTC(order);
}

void WebRtcSpl_FreeRealFFTNeon(struct RealFFT* self) {
  WebRtcSpl_FreeRealFFTC(self);
}

int WebRtcSpl_RealForwardFFTNeon(struct RealFFT* self,
                                 const int16_t* real_data_in,
                                 int16_t* complex_data_out) {
  return WebRtcSpl_RealForwardFFTC(self, real_data_in, complex_data_out);
}

int WebRtcSpl_RealInverseFFTNeon(struct RealFFT* self,
                                 const int16_t* complex_data_in,
                                 int16_t* real_data_out) {
  return WebRtcSpl_RealInverseFFTC(self, complex_data_in, real_data_out);
}
#endif  

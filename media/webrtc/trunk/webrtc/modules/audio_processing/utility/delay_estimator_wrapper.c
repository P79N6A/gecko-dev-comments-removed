









#include "delay_estimator_wrapper.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "delay_estimator.h"
#include "modules/audio_processing/utility/delay_estimator_internal.h"



static const int kBandFirst = 12;
static const int kBandLast = 43;

static __inline uint32_t SetBit(uint32_t in, int pos) {
  uint32_t mask = (1 << pos);
  uint32_t out = (in | mask);

  return out;
}











static void MeanEstimatorFloat(float new_value,
                               float scale,
                               float* mean_value) {
  assert(scale < 1.0f);
  *mean_value += (new_value - *mean_value) * scale;
}












static uint32_t BinarySpectrumFix(uint16_t* spectrum,
                                  SpectrumType* threshold_spectrum,
                                  int q_domain,
                                  int* threshold_initialized) {
  int i = kBandFirst;
  uint32_t out = 0;

  assert(q_domain < 16);

  if (!(*threshold_initialized)) {
    
    
    for (i = kBandFirst; i <= kBandLast; i++) {
      if (spectrum[i] > 0) {
        
        int32_t spectrum_q15 = ((int32_t) spectrum[i]) << (15 - q_domain);
        threshold_spectrum[i].int32_ = (spectrum_q15 >> 1);
        *threshold_initialized = 1;
      }
    }
  }
  for (i = kBandFirst; i <= kBandLast; i++) {
    
    int32_t spectrum_q15 = ((int32_t) spectrum[i]) << (15 - q_domain);
    
    WebRtc_MeanEstimatorFix(spectrum_q15, 6, &(threshold_spectrum[i].int32_));
    
    if (spectrum_q15 > threshold_spectrum[i].int32_) {
      out = SetBit(out, i - kBandFirst);
    }
  }

  return out;
}

static uint32_t BinarySpectrumFloat(float* spectrum,
                                    SpectrumType* threshold_spectrum,
                                    int* threshold_initialized) {
  int i = kBandFirst;
  uint32_t out = 0;
  const float kScale = 1 / 64.0;

  if (!(*threshold_initialized)) {
    
    
    for (i = kBandFirst; i <= kBandLast; i++) {
      if (spectrum[i] > 0.0f) {
        threshold_spectrum[i].float_ = (spectrum[i] / 2);
        *threshold_initialized = 1;
      }
    }
  }

  for (i = kBandFirst; i <= kBandLast; i++) {
    
    MeanEstimatorFloat(spectrum[i], kScale, &(threshold_spectrum[i].float_));
    
    if (spectrum[i] > threshold_spectrum[i].float_) {
      out = SetBit(out, i - kBandFirst);
    }
  }

  return out;
}

void WebRtc_FreeDelayEstimator(void* handle) {
  DelayEstimator* self = (DelayEstimator*) handle;

  if (handle == NULL) {
    return;
  }

  free(self->mean_far_spectrum);
  self->mean_far_spectrum = NULL;

  free(self->mean_near_spectrum);
  self->mean_near_spectrum = NULL;

  WebRtc_FreeBinaryDelayEstimator(self->binary_handle);
  self->binary_handle = NULL;

  free(self);
}

void* WebRtc_CreateDelayEstimator(int spectrum_size, int max_delay,
                                  int lookahead) {
  DelayEstimator* self = NULL;

  
  
  
  assert(kBandLast - kBandFirst < 32);

  if (spectrum_size >= kBandLast) {
    self = malloc(sizeof(DelayEstimator));
  }

  if (self != NULL) {
    int memory_fail = 0;

    self->mean_far_spectrum = NULL;
    self->mean_near_spectrum = NULL;

    self->binary_handle = WebRtc_CreateBinaryDelayEstimator(max_delay,
                                                            lookahead);
    memory_fail |= (self->binary_handle == NULL);

    
    self->mean_far_spectrum = malloc(spectrum_size * sizeof(SpectrumType));
    memory_fail |= (self->mean_far_spectrum == NULL);

    self->mean_near_spectrum = malloc(spectrum_size * sizeof(SpectrumType));
    memory_fail |= (self->mean_near_spectrum == NULL);

    self->spectrum_size = spectrum_size;

    if (memory_fail) {
      WebRtc_FreeDelayEstimator(self);
      self = NULL;
    }
  }

  return self;
}

int WebRtc_InitDelayEstimator(void* handle) {
  DelayEstimator* self = (DelayEstimator*) handle;

  if (self == NULL) {
    return -1;
  }

  
  WebRtc_InitBinaryDelayEstimator(self->binary_handle);

  
  memset(self->mean_far_spectrum, 0,
         sizeof(SpectrumType) * self->spectrum_size);
  memset(self->mean_near_spectrum, 0,
         sizeof(SpectrumType) * self->spectrum_size);
  
  self->far_spectrum_initialized = 0;
  self->near_spectrum_initialized = 0;

  return 0;
}

int WebRtc_DelayEstimatorProcessFix(void* handle,
                                    uint16_t* far_spectrum,
                                    uint16_t* near_spectrum,
                                    int spectrum_size,
                                    int far_q,
                                    int near_q) {
  DelayEstimator* self = (DelayEstimator*) handle;
  uint32_t binary_far_spectrum = 0;
  uint32_t binary_near_spectrum = 0;

  if (self == NULL) {
    return -1;
  }
  if (far_spectrum == NULL) {
    
    return -1;
  }
  if (near_spectrum == NULL) {
    
    return -1;
  }
  if (spectrum_size != self->spectrum_size) {
    
    return -1;
  }
  if (far_q > 15) {
    
    return -1;
  }
  if (near_q > 15) {
    
    return -1;
  }

  
  binary_far_spectrum = BinarySpectrumFix(far_spectrum,
                                          self->mean_far_spectrum,
                                          far_q,
                                          &(self->far_spectrum_initialized));
  binary_near_spectrum = BinarySpectrumFix(near_spectrum,
                                           self->mean_near_spectrum,
                                           near_q,
                                           &(self->near_spectrum_initialized));

  return WebRtc_ProcessBinarySpectrum(self->binary_handle,
                                      binary_far_spectrum,
                                      binary_near_spectrum);
}

int WebRtc_DelayEstimatorProcessFloat(void* handle,
                                      float* far_spectrum,
                                      float* near_spectrum,
                                      int spectrum_size) {
  DelayEstimator* self = (DelayEstimator*) handle;
  uint32_t binary_far_spectrum = 0;
  uint32_t binary_near_spectrum = 0;

  if (self == NULL) {
    return -1;
  }
  if (far_spectrum == NULL) {
    
    return -1;
  }
  if (near_spectrum == NULL) {
    
    return -1;
  }
  if (spectrum_size != self->spectrum_size) {
    
    return -1;
  }

  
  binary_far_spectrum = BinarySpectrumFloat(far_spectrum,
                                            self->mean_far_spectrum,
                                            &(self->far_spectrum_initialized));
  binary_near_spectrum = BinarySpectrumFloat(near_spectrum,
                                             self->mean_near_spectrum,
                                             &(self->near_spectrum_initialized));

  return WebRtc_ProcessBinarySpectrum(self->binary_handle,
                                      binary_far_spectrum,
                                      binary_near_spectrum);
}

int WebRtc_last_delay(void* handle) {
  DelayEstimator* self = (DelayEstimator*) handle;

  if (self == NULL) {
    return -1;
  }

  return WebRtc_binary_last_delay(self->binary_handle);
}

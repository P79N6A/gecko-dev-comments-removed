









#include "webrtc/modules/audio_processing/utility/delay_estimator_wrapper.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "webrtc/modules/audio_processing/utility/delay_estimator.h"
#include "webrtc/modules/audio_processing/utility/delay_estimator_internal.h"
#include "webrtc/system_wrappers/interface/compile_assert_c.h"



enum { kBandFirst = 12 };
enum { kBandLast = 43 };

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












static uint32_t BinarySpectrumFix(const uint16_t* spectrum,
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

static uint32_t BinarySpectrumFloat(const float* spectrum,
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

void WebRtc_FreeDelayEstimatorFarend(void* handle) {
  DelayEstimatorFarend* self = (DelayEstimatorFarend*) handle;

  if (handle == NULL) {
    return;
  }

  free(self->mean_far_spectrum);
  self->mean_far_spectrum = NULL;

  WebRtc_FreeBinaryDelayEstimatorFarend(self->binary_farend);
  self->binary_farend = NULL;

  free(self);
}

void* WebRtc_CreateDelayEstimatorFarend(int spectrum_size, int history_size) {
  DelayEstimatorFarend* self = NULL;

  
  
  COMPILE_ASSERT(kBandLast - kBandFirst < 32);

  if (spectrum_size >= kBandLast) {
    self = malloc(sizeof(DelayEstimatorFarend));
  }

  if (self != NULL) {
    int memory_fail = 0;

    
    self->binary_farend = WebRtc_CreateBinaryDelayEstimatorFarend(history_size);
    memory_fail |= (self->binary_farend == NULL);

    
    self->mean_far_spectrum = malloc(spectrum_size * sizeof(SpectrumType));
    memory_fail |= (self->mean_far_spectrum == NULL);

    self->spectrum_size = spectrum_size;

    if (memory_fail) {
      WebRtc_FreeDelayEstimatorFarend(self);
      self = NULL;
    }
  }

  return self;
}

int WebRtc_InitDelayEstimatorFarend(void* handle) {
  DelayEstimatorFarend* self = (DelayEstimatorFarend*) handle;

  if (self == NULL) {
    return -1;
  }

  
  WebRtc_InitBinaryDelayEstimatorFarend(self->binary_farend);

  
  memset(self->mean_far_spectrum, 0,
         sizeof(SpectrumType) * self->spectrum_size);
  
  self->far_spectrum_initialized = 0;

  return 0;
}

void WebRtc_SoftResetDelayEstimatorFarend(void* handle, int delay_shift) {
  DelayEstimatorFarend* self = (DelayEstimatorFarend*) handle;
  assert(self != NULL);
  WebRtc_SoftResetBinaryDelayEstimatorFarend(self->binary_farend, delay_shift);
}

int WebRtc_AddFarSpectrumFix(void* handle,
                             const uint16_t* far_spectrum,
                             int spectrum_size,
                             int far_q) {
  DelayEstimatorFarend* self = (DelayEstimatorFarend*) handle;
  uint32_t binary_spectrum = 0;

  if (self == NULL) {
    return -1;
  }
  if (far_spectrum == NULL) {
    
    return -1;
  }
  if (spectrum_size != self->spectrum_size) {
    
    return -1;
  }
  if (far_q > 15) {
    
    return -1;
  }

  
  binary_spectrum = BinarySpectrumFix(far_spectrum, self->mean_far_spectrum,
                                      far_q, &(self->far_spectrum_initialized));
  WebRtc_AddBinaryFarSpectrum(self->binary_farend, binary_spectrum);

  return 0;
}

int WebRtc_AddFarSpectrumFloat(void* handle,
                               const float* far_spectrum,
                               int spectrum_size) {
  DelayEstimatorFarend* self = (DelayEstimatorFarend*) handle;
  uint32_t binary_spectrum = 0;

  if (self == NULL) {
    return -1;
  }
  if (far_spectrum == NULL) {
    
    return -1;
  }
  if (spectrum_size != self->spectrum_size) {
    
    return -1;
  }

  
  binary_spectrum = BinarySpectrumFloat(far_spectrum, self->mean_far_spectrum,
                                        &(self->far_spectrum_initialized));
  WebRtc_AddBinaryFarSpectrum(self->binary_farend, binary_spectrum);

  return 0;
}

void WebRtc_FreeDelayEstimator(void* handle) {
  DelayEstimator* self = (DelayEstimator*) handle;

  if (handle == NULL) {
    return;
  }

  free(self->mean_near_spectrum);
  self->mean_near_spectrum = NULL;

  WebRtc_FreeBinaryDelayEstimator(self->binary_handle);
  self->binary_handle = NULL;

  free(self);
}

void* WebRtc_CreateDelayEstimator(void* farend_handle, int max_lookahead) {
  DelayEstimator* self = NULL;
  DelayEstimatorFarend* farend = (DelayEstimatorFarend*) farend_handle;

  if (farend_handle != NULL) {
    self = malloc(sizeof(DelayEstimator));
  }

  if (self != NULL) {
    int memory_fail = 0;

    
    self->binary_handle =
        WebRtc_CreateBinaryDelayEstimator(farend->binary_farend, max_lookahead);
    memory_fail |= (self->binary_handle == NULL);

    
    self->mean_near_spectrum = malloc(farend->spectrum_size *
                                      sizeof(SpectrumType));
    memory_fail |= (self->mean_near_spectrum == NULL);

    self->spectrum_size = farend->spectrum_size;

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

  
  memset(self->mean_near_spectrum, 0,
         sizeof(SpectrumType) * self->spectrum_size);
  
  self->near_spectrum_initialized = 0;

  return 0;
}

int WebRtc_SoftResetDelayEstimator(void* handle, int delay_shift) {
  DelayEstimator* self = (DelayEstimator*) handle;
  assert(self != NULL);
  return WebRtc_SoftResetBinaryDelayEstimator(self->binary_handle, delay_shift);
}

int WebRtc_set_history_size(void* handle, int history_size) {
  DelayEstimator* self = handle;

  if ((self == NULL) || (history_size <= 1)) {
    return -1;
  }
  return WebRtc_AllocateHistoryBufferMemory(self->binary_handle, history_size);
}

int WebRtc_history_size(const void* handle) {
  const DelayEstimator* self = handle;

  if (self == NULL) {
    return -1;
  }
  if (self->binary_handle->farend->history_size !=
      self->binary_handle->history_size) {
    
    return -1;
  }
  return self->binary_handle->history_size;
}

int WebRtc_set_lookahead(void* handle, int lookahead) {
  DelayEstimator* self = (DelayEstimator*) handle;
  assert(self != NULL);
  assert(self->binary_handle != NULL);
  if ((lookahead > self->binary_handle->near_history_size - 1) ||
      (lookahead < 0)) {
    return -1;
  }
  self->binary_handle->lookahead = lookahead;
  return self->binary_handle->lookahead;
}

int WebRtc_lookahead(void* handle) {
  DelayEstimator* self = (DelayEstimator*) handle;
  assert(self != NULL);
  assert(self->binary_handle != NULL);
  return self->binary_handle->lookahead;
}

int WebRtc_set_allowed_offset(void* handle, int allowed_offset) {
  DelayEstimator* self = (DelayEstimator*) handle;

  if ((self == NULL) || (allowed_offset < 0)) {
    return -1;
  }
  self->binary_handle->allowed_offset = allowed_offset;
  return 0;
}

int WebRtc_get_allowed_offset(const void* handle) {
  const DelayEstimator* self = (const DelayEstimator*) handle;

  if (self == NULL) {
    return -1;
  }
  return self->binary_handle->allowed_offset;
}

int WebRtc_enable_robust_validation(void* handle, int enable) {
  DelayEstimator* self = (DelayEstimator*) handle;

  if (self == NULL) {
    return -1;
  }
  if ((enable < 0) || (enable > 1)) {
    return -1;
  }
  assert(self->binary_handle != NULL);
  self->binary_handle->robust_validation_enabled = enable;
  return 0;
}

int WebRtc_is_robust_validation_enabled(const void* handle) {
  const DelayEstimator* self = (const DelayEstimator*) handle;

  if (self == NULL) {
    return -1;
  }
  return self->binary_handle->robust_validation_enabled;
}

int WebRtc_DelayEstimatorProcessFix(void* handle,
                                    const uint16_t* near_spectrum,
                                    int spectrum_size,
                                    int near_q) {
  DelayEstimator* self = (DelayEstimator*) handle;
  uint32_t binary_spectrum = 0;

  if (self == NULL) {
    return -1;
  }
  if (near_spectrum == NULL) {
    
    return -1;
  }
  if (spectrum_size != self->spectrum_size) {
    
    return -1;
  }
  if (near_q > 15) {
    
    return -1;
  }

  
  binary_spectrum = BinarySpectrumFix(near_spectrum,
                                      self->mean_near_spectrum,
                                      near_q,
                                      &(self->near_spectrum_initialized));

  return WebRtc_ProcessBinarySpectrum(self->binary_handle, binary_spectrum);
}

int WebRtc_DelayEstimatorProcessFloat(void* handle,
                                      const float* near_spectrum,
                                      int spectrum_size) {
  DelayEstimator* self = (DelayEstimator*) handle;
  uint32_t binary_spectrum = 0;

  if (self == NULL) {
    return -1;
  }
  if (near_spectrum == NULL) {
    
    return -1;
  }
  if (spectrum_size != self->spectrum_size) {
    
    return -1;
  }

  
  binary_spectrum = BinarySpectrumFloat(near_spectrum, self->mean_near_spectrum,
                                        &(self->near_spectrum_initialized));

  return WebRtc_ProcessBinarySpectrum(self->binary_handle, binary_spectrum);
}

int WebRtc_last_delay(void* handle) {
  DelayEstimator* self = (DelayEstimator*) handle;

  if (self == NULL) {
    return -1;
  }

  return WebRtc_binary_last_delay(self->binary_handle);
}

float WebRtc_last_delay_quality(void* handle) {
  DelayEstimator* self = (DelayEstimator*) handle;
  assert(self != NULL);
  return WebRtc_binary_last_delay_quality(self->binary_handle);
}

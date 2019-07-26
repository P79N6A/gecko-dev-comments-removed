











#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_UTILITY_DELAY_ESTIMATOR_INTERNAL_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_UTILITY_DELAY_ESTIMATOR_INTERNAL_H_

#include "modules/audio_processing/utility/delay_estimator.h"
#include "typedefs.h"

typedef union {
  float float_;
  int32_t int32_;
} SpectrumType;

typedef struct {
  
  SpectrumType* mean_far_spectrum;
  SpectrumType* mean_near_spectrum;
  
  int far_spectrum_initialized;
  int near_spectrum_initialized;

  int spectrum_size;

  
  BinaryDelayEstimator* binary_handle;
} DelayEstimator;

#endif  

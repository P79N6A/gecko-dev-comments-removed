












#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_UTILITY_DELAY_ESTIMATOR_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_UTILITY_DELAY_ESTIMATOR_H_

#include "typedefs.h"

typedef struct {
  
  int32_t* mean_bit_counts;
  int* far_bit_counts;

  
  
  int32_t* bit_counts;

  
  uint32_t* binary_far_history;
  uint32_t* binary_near_history;

  
  int32_t minimum_probability;
  int last_delay_probability;

  
  int last_delay;

  
  int history_size;

  
  int near_history_size;
} BinaryDelayEstimator;







void WebRtc_FreeBinaryDelayEstimator(BinaryDelayEstimator* handle);


BinaryDelayEstimator* WebRtc_CreateBinaryDelayEstimator(int max_delay,
                                                        int lookahead);









void WebRtc_InitBinaryDelayEstimator(BinaryDelayEstimator* handle);

















int WebRtc_ProcessBinarySpectrum(BinaryDelayEstimator* handle,
                                 uint32_t binary_far_spectrum,
                                 uint32_t binary_near_spectrum);












int WebRtc_binary_last_delay(BinaryDelayEstimator* handle);












void WebRtc_MeanEstimatorFix(int32_t new_value,
                             int factor,
                             int32_t* mean_value);


#endif  

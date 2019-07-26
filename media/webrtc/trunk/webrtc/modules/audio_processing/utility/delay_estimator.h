












#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_UTILITY_DELAY_ESTIMATOR_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_UTILITY_DELAY_ESTIMATOR_H_

#include "webrtc/typedefs.h"

typedef struct {
  
  int* far_bit_counts;
  
  uint32_t* binary_far_history;
  int history_size;
} BinaryDelayEstimatorFarend;

typedef struct {
  
  int32_t* mean_bit_counts;
  
  
  int32_t* bit_counts;

  
  uint32_t* binary_near_history;
  int near_history_size;

  
  int32_t minimum_probability;
  int last_delay_probability;

  
  int last_delay;

  
  int robust_validation_enabled;
  int allowed_offset;
  int last_candidate_delay;
  int compare_delay;
  int candidate_hits;
  float* histogram;
  float last_delay_histogram;

  
  BinaryDelayEstimatorFarend* farend;
} BinaryDelayEstimator;








void WebRtc_FreeBinaryDelayEstimatorFarend(BinaryDelayEstimatorFarend* self);














BinaryDelayEstimatorFarend* WebRtc_CreateBinaryDelayEstimatorFarend(
    int history_size);










void WebRtc_InitBinaryDelayEstimatorFarend(BinaryDelayEstimatorFarend* self);













void WebRtc_AddBinaryFarSpectrum(BinaryDelayEstimatorFarend* self,
                                 uint32_t binary_far_spectrum);












void WebRtc_FreeBinaryDelayEstimator(BinaryDelayEstimator* self);


































BinaryDelayEstimator* WebRtc_CreateBinaryDelayEstimator(
    BinaryDelayEstimatorFarend* farend, int lookahead);










void WebRtc_InitBinaryDelayEstimator(BinaryDelayEstimator* self);


















int WebRtc_ProcessBinarySpectrum(BinaryDelayEstimator* self,
                                 uint32_t binary_near_spectrum);











int WebRtc_binary_last_delay(BinaryDelayEstimator* self);













int WebRtc_binary_last_delay_quality(BinaryDelayEstimator* self);












void WebRtc_MeanEstimatorFix(int32_t new_value,
                             int factor,
                             int32_t* mean_value);


#endif  














#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_UTILITY_DELAY_ESTIMATOR_WRAPPER_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_UTILITY_DELAY_ESTIMATOR_WRAPPER_H_

#include "typedefs.h"





void WebRtc_FreeDelayEstimator(void* handle);

























void* WebRtc_CreateDelayEstimator(int spectrum_size, int max_delay,
                                  int lookahead);









int WebRtc_InitDelayEstimator(void* handle);






















int WebRtc_DelayEstimatorProcessFix(void* handle,
                                    uint16_t* far_spectrum,
                                    uint16_t* near_spectrum,
                                    int spectrum_size,
                                    int far_q,
                                    int near_q);


int WebRtc_DelayEstimatorProcessFloat(void* handle,
                                      float* far_spectrum,
                                      float* near_spectrum,
                                      int spectrum_size);












int WebRtc_last_delay(void* handle);

#endif  

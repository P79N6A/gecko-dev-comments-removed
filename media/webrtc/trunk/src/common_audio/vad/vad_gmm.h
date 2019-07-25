











#ifndef WEBRTC_COMMON_AUDIO_VAD_VAD_GMM_H_
#define WEBRTC_COMMON_AUDIO_VAD_VAD_GMM_H_

#include "typedefs.h"

















int32_t WebRtcVad_GaussianProbability(int16_t input,
                                      int16_t mean,
                                      int16_t std,
                                      int16_t* delta);

#endif  

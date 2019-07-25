













#ifndef WEBRTC_COMMON_AUDIO_VAD_VAD_FILTERBANK_H_
#define WEBRTC_COMMON_AUDIO_VAD_VAD_FILTERBANK_H_

#include "typedefs.h"
#include "vad_core.h"





















int16_t WebRtcVad_CalculateFeatures(VadInstT* self, const int16_t* data_in,
                                    int data_length, int16_t* features);

#endif  

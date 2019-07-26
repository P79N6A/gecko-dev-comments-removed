












#ifndef WEBRTC_COMMON_AUDIO_VAD_VAD_SP_H_
#define WEBRTC_COMMON_AUDIO_VAD_VAD_SP_H_

#include "typedefs.h"
#include "vad_core.h"














void WebRtcVad_Downsampling(int16_t* signal_in,
                            int16_t* signal_out,
                            int32_t* filter_state,
                            int in_length);















int16_t WebRtcVad_FindMinimum(VadInstT* handle,
                              int16_t feature_value,
                              int channel);

#endif  

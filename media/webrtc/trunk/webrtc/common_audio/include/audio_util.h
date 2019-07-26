









#ifndef WEBRTC_COMMON_AUDIO_INCLUDE_AUDIO_UTIL_H_
#define WEBRTC_COMMON_AUDIO_INCLUDE_AUDIO_UTIL_H_

#include "webrtc/typedefs.h"

namespace webrtc {





void Deinterleave(const int16_t* interleaved, int samples_per_channel,
                  int num_channels, int16_t** deinterleaved);




void Interleave(const int16_t* const* deinterleaved, int samples_per_channel,
                int num_channels, int16_t* interleaved);

}  

#endif  

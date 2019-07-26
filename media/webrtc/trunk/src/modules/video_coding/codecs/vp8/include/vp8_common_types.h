









#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_VP8_COMMON_TYPES_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_VP8_COMMON_TYPES_H_

#include "common_types.h"

namespace webrtc {



static const float
  kVp8LayerRateAlloction[kMaxTemporalStreams][kMaxTemporalStreams] = {
    {1.0f, 0, 0, 0},  
    {0.6f, 1.0f , 0 , 0},  
    {0.4f, 0.6f , 1.0f, 0},  
    {0.25f, 0.4f, 0.6f, 1.0f}  
};

}  
#endif  

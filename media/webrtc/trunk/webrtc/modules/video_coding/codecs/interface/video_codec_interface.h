









#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_INTERFACE_VIDEO_CODEC_INTERFACE_H
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_INTERFACE_VIDEO_CODEC_INTERFACE_H

#include <vector>

#include "webrtc/common_types.h"
#include "webrtc/common_video/interface/i420_video_frame.h"
#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/modules/video_coding/codecs/interface/video_error_codes.h"
#include "webrtc/typedefs.h"
#include "webrtc/video_decoder.h"
#include "webrtc/video_encoder.h"

namespace webrtc
{

class RTPFragmentationHeader; 



struct CodecSpecificInfoVP8 {
  bool hasReceivedSLI;
  uint8_t pictureIdSLI;
  bool hasReceivedRPSI;
  uint64_t pictureIdRPSI;
  int16_t pictureId;  
  bool nonReference;
  uint8_t simulcastIdx;
  uint8_t temporalIdx;
  bool layerSync;
  int tl0PicIdx;  
  int8_t keyIdx;  
};

struct CodecSpecificInfoVP9 {
  bool hasReceivedSLI;
  uint8_t pictureIdSLI;
  bool hasReceivedRPSI;
  uint64_t pictureIdRPSI;
  int16_t pictureId;  
  bool nonReference;
  uint8_t temporalIdx;
  bool layerSync;
  int tl0PicIdx;  
  int8_t keyIdx;  
};

struct CodecSpecificInfoGeneric {
  uint8_t simulcast_idx;
};

struct CodecSpecificInfoH264 {};

union CodecSpecificInfoUnion {
  CodecSpecificInfoGeneric generic;
  CodecSpecificInfoVP8 VP8;
  CodecSpecificInfoVP9 VP9;
  CodecSpecificInfoH264 H264;
};




struct CodecSpecificInfo
{
    VideoCodecType   codecType;
    CodecSpecificInfoUnion codecSpecific;
};

}  

#endif 

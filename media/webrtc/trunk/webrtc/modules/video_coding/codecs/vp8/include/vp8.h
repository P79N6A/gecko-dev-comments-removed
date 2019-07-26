











#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_VP8_INCLUDE_VP8_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_VP8_INCLUDE_VP8_H_

#include "modules/video_coding/codecs/interface/video_codec_interface.h"

namespace webrtc {

class VP8Encoder : public VideoEncoder {
 public:
  static VP8Encoder* Create();

  virtual ~VP8Encoder() {};
};  


class VP8Decoder : public VideoDecoder {
 public:
  static VP8Decoder* Create();

  virtual ~VP8Decoder() {};
};  
}  

#endif 












#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_VP9_INCLUDE_VP9_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_VP9_INCLUDE_VP9_H_

#include "webrtc/modules/video_coding/codecs/interface/video_codec_interface.h"

namespace webrtc {

class VP9Encoder : public VideoEncoder {
 public:
  static VP9Encoder* Create();

  virtual ~VP9Encoder() {}
};


class VP9Decoder : public VideoDecoder {
 public:
  static VP9Decoder* Create();

  virtual ~VP9Decoder() {}
};
}  

#endif  

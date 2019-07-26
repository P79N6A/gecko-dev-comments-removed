










#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_H264_INCLUDE_H264_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_H264_INCLUDE_H264_H_

#include "webrtc/modules/video_coding/codecs/interface/video_codec_interface.h"

namespace webrtc {

class H264Encoder : public VideoEncoder {
 public:
  static H264Encoder* Create();

  virtual ~H264Encoder() {}
};  

class H264Decoder : public VideoDecoder {
 public:
  static H264Decoder* Create();

  virtual ~H264Decoder() {}
};  

}  

#endif  

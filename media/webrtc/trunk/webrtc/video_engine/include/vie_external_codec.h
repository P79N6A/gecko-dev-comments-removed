









#ifndef WEBRTC_VIDEO_ENGINE_INCLUDE_VIE_EXTERNAL_CODEC_H_
#define WEBRTC_VIDEO_ENGINE_INCLUDE_VIE_EXTERNAL_CODEC_H_

#include "common_types.h"

namespace webrtc {

class VideoDecoder;
class VideoEncoder;
class VideoEngine;

class WEBRTC_DLLEXPORT ViEExternalCodec {
 public:
  static ViEExternalCodec* GetInterface(VideoEngine* video_engine);

  virtual int Release() = 0;

  virtual int RegisterExternalSendCodec(const int video_channel,
                                        const unsigned char pl_type,
                                        VideoEncoder* encoder) = 0;

  virtual int DeRegisterExternalSendCodec(const int video_channel,
                                          const unsigned char pl_type) = 0;

  virtual int RegisterExternalReceiveCodec(const int video_channel,
                                           const unsigned int pl_type,
                                           VideoDecoder* decoder,
                                           bool decoder_render = false,
                                           int render_delay = 0) = 0;

  virtual int DeRegisterExternalReceiveCodec(const int video_channel,
                                             const unsigned char pl_type) = 0;

 protected:
  ViEExternalCodec() {}
  virtual ~ViEExternalCodec() {}
};

}  

#endif  

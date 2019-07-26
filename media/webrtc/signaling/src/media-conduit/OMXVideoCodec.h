



#ifndef OMX_VIDEO_CODEC_H_
#define OMX_VIDEO_CODEC_H_

#include "MediaConduitInterface.h"

namespace mozilla {
class OMXVideoCodec {
 public:
  enum CodecType {
    CODEC_H264,
  };

  



  static VideoEncoder* CreateEncoder(CodecType aCodecType);

  



  static VideoDecoder* CreateDecoder(CodecType aCodecType);
};

}

#endif 

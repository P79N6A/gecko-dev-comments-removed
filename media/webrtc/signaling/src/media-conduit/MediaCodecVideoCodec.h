



#ifndef MediaCodecVideoCodec_h__
#define MediaCodecVideoCodec_h__

#include "MediaConduitInterface.h"

namespace mozilla {
class MediaCodecVideoCodec {
 public:
 enum CodecType {
    CODEC_VP8,
  };
  



  static VideoEncoder* CreateEncoder(CodecType aCodecType);

  



  static VideoDecoder* CreateDecoder(CodecType aCodecType);
};

}

#endif 

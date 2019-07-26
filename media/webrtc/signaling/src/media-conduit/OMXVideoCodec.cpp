



#include "OMXVideoCodec.h"

#ifdef WEBRTC_GONK
#include "WebrtcOMXH264VideoCodec.h"
#endif

namespace mozilla {

VideoEncoder*
OMXVideoCodec::CreateEncoder(CodecType aCodecType)
{
  if (aCodecType == CODEC_H264) {
    return new WebrtcOMXH264VideoEncoder();
  }
  return nullptr;
}

VideoDecoder*
OMXVideoCodec::CreateDecoder(CodecType aCodecType) {
  if (aCodecType == CODEC_H264) {
    return new WebrtcOMXH264VideoDecoder();
  }
  return nullptr;
}

}

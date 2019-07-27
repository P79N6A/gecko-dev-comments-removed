



#include "CSFLog.h"
#include "nspr.h"

#include "WebrtcMediaCodecVP8VideoCodec.h"
#include "MediaCodecVideoCodec.h"

namespace mozilla {

static const char* logTag ="MediaCodecVideoCodec";

VideoEncoder* MediaCodecVideoCodec::CreateEncoder(CodecType aCodecType) {
  CSFLogDebug(logTag,  "%s ", __FUNCTION__);
  if (aCodecType == CODEC_VP8) {
     return new WebrtcMediaCodecVP8VideoEncoder();
  }
  return nullptr;
}

VideoDecoder* MediaCodecVideoCodec::CreateDecoder(CodecType aCodecType) {
  CSFLogDebug(logTag,  "%s ", __FUNCTION__);
  if (aCodecType == CODEC_VP8) {
    return new WebrtcMediaCodecVP8VideoDecoder();
  }
  return nullptr;
}

}

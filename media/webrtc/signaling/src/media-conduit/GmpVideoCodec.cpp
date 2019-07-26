



#include "WebrtcGmpVideoCodec.h"
#include "GmpVideoCodec.h"

namespace mozilla {

VideoEncoder* GmpVideoCodec::CreateEncoder() {
  return static_cast<VideoEncoder*>(new WebrtcGmpVideoEncoder());
}

VideoDecoder* GmpVideoCodec::CreateDecoder() {
  return static_cast<VideoDecoder*>(new WebrtcGmpVideoDecoder());
}

}

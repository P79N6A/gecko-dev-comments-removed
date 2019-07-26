



#include "CSFLog.h"
#include "nspr.h"

#include "WebrtcExtVideoCodec.h"
#include "ExtVideoCodec.h"

namespace mozilla {

static const char* logTag ="ExtVideoCodec";

VideoEncoder* ExtVideoCodec::CreateEncoder() {
  CSFLogDebug(logTag,  "%s ", __FUNCTION__);
  WebrtcExtVideoEncoder *enc =
      new WebrtcExtVideoEncoder();
  return enc;
}

VideoDecoder* ExtVideoCodec::CreateDecoder() {
  CSFLogDebug(logTag,  "%s ", __FUNCTION__);
  WebrtcExtVideoDecoder *dec =
      new WebrtcExtVideoDecoder();
  return dec;
}

}

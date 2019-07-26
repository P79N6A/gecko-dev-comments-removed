









#include "webrtc/modules/audio_coding/main/acm2/acm_red.h"

#include "webrtc/modules/audio_coding/main/acm2/acm_common_defs.h"
#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc {

namespace acm2 {

ACMRED::ACMRED(int16_t codec_id) { codec_id_ = codec_id; }

ACMRED::~ACMRED() {}

int16_t ACMRED::InternalEncode(uint8_t* ,
                               int16_t* ) {
  
  
  return 0;
}

int16_t ACMRED::InternalInitEncoder(WebRtcACMCodecParams* ) {
  
  
  return 0;
}

ACMGenericCodec* ACMRED::CreateInstance(void) { return NULL; }

int16_t ACMRED::InternalCreateEncoder() {
  
  return 0;
}

void ACMRED::InternalDestructEncoderInst(void* ) {
  
}

void ACMRED::DestructEncoderSafe() {
  
}

}  

}  











#include "webrtc/modules/audio_coding/main/source/acm_red.h"

#include "webrtc/modules/audio_coding/main/acm2/acm_common_defs.h"
#include "webrtc/modules/audio_coding/main/source/acm_neteq.h"
#include "webrtc/modules/audio_coding/neteq/interface/webrtc_neteq.h"
#include "webrtc/modules/audio_coding/neteq/interface/webrtc_neteq_help_macros.h"
#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc {

namespace acm1 {

ACMRED::ACMRED(int16_t codec_id) {
  codec_id_ = codec_id;
}

ACMRED::~ACMRED() {
  return;
}

int16_t ACMRED::InternalEncode(uint8_t* ,
                               int16_t* ) {
  
  
  return 0;
}

int16_t ACMRED::DecodeSafe(uint8_t* ,
                           int16_t ,
                           int16_t* ,
                           int16_t* ,
                           int8_t* ) {
  return 0;
}

int16_t ACMRED::InternalInitEncoder(
    WebRtcACMCodecParams* ) {
  
  
  return 0;
}

int16_t ACMRED::InternalInitDecoder(
    WebRtcACMCodecParams* ) {
  
  
  return 0;
}

int32_t ACMRED::CodecDef(WebRtcNetEQ_CodecDef& codec_def,
                         const CodecInst& codec_inst) {
  if (!decoder_initialized_) {
    
    
    return -1;
  }

  
  
  
  
  SET_CODEC_PAR((codec_def), kDecoderRED, codec_inst.pltype, NULL, 8000);
  SET_RED_FUNCTIONS((codec_def));
  return 0;
}

ACMGenericCodec* ACMRED::CreateInstance(void) {
  return NULL;
}

int16_t ACMRED::InternalCreateEncoder() {
  
  return 0;
}

int16_t ACMRED::InternalCreateDecoder() {
  
  return 0;
}

void ACMRED::InternalDestructEncoderInst(void* ) {
  
  return;
}

void ACMRED::DestructEncoderSafe() {
  
  return;
}

void ACMRED::DestructDecoderSafe() {
  
  return;
}

}  

}  

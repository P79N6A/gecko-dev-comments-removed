









#include "webrtc/modules/audio_coding/main/source/acm_red.h"

#include "webrtc/modules/audio_coding/main/source/acm_common_defs.h"
#include "webrtc/modules/audio_coding/main/source/acm_neteq.h"
#include "webrtc/modules/audio_coding/neteq/interface/webrtc_neteq.h"
#include "webrtc/modules/audio_coding/neteq/interface/webrtc_neteq_help_macros.h"
#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc {

ACMRED::ACMRED(WebRtc_Word16 codec_id) {
  codec_id_ = codec_id;
}

ACMRED::~ACMRED() {
  return;
}

WebRtc_Word16 ACMRED::InternalEncode(WebRtc_UWord8* ,
                                     WebRtc_Word16* ) {
  
  
  return 0;
}

WebRtc_Word16 ACMRED::DecodeSafe(WebRtc_UWord8* ,
                                 WebRtc_Word16 ,
                                 WebRtc_Word16* ,
                                 WebRtc_Word16* ,
                                 WebRtc_Word8* ) {
  return 0;
}

WebRtc_Word16 ACMRED::InternalInitEncoder(
    WebRtcACMCodecParams* ) {
  
  
  return 0;
}

WebRtc_Word16 ACMRED::InternalInitDecoder(
    WebRtcACMCodecParams* ) {
  
  
  return 0;
}

WebRtc_Word32 ACMRED::CodecDef(WebRtcNetEQ_CodecDef& codec_def,
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

WebRtc_Word16 ACMRED::InternalCreateEncoder() {
  
  return 0;
}

WebRtc_Word16 ACMRED::InternalCreateDecoder() {
  
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

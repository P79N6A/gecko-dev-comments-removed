









#include "webrtc/modules/audio_coding/main/source/acm_dtmf_playout.h"

#include "webrtc/modules/audio_coding/main/source/acm_common_defs.h"
#include "webrtc/modules/audio_coding/main/source/acm_neteq.h"
#include "webrtc/modules/audio_coding/neteq/interface/webrtc_neteq.h"
#include "webrtc/modules/audio_coding/neteq/interface/webrtc_neteq_help_macros.h"
#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc {

#ifndef WEBRTC_CODEC_AVT

ACMDTMFPlayout::ACMDTMFPlayout(
    WebRtc_Word16 ) {
  return;
}

ACMDTMFPlayout::~ACMDTMFPlayout() {
  return;
}

WebRtc_Word16 ACMDTMFPlayout::InternalEncode(
    WebRtc_UWord8* ,
    WebRtc_Word16* ) {
  return -1;
}

WebRtc_Word16 ACMDTMFPlayout::DecodeSafe(
    WebRtc_UWord8* ,
    WebRtc_Word16 ,
    WebRtc_Word16* ,
    WebRtc_Word16* ,
    WebRtc_Word8* ) {
  return -1;
}

WebRtc_Word16 ACMDTMFPlayout::InternalInitEncoder(
    WebRtcACMCodecParams* ) {
  return -1;
}

WebRtc_Word16 ACMDTMFPlayout::InternalInitDecoder(
    WebRtcACMCodecParams* ) {
  return -1;
}

WebRtc_Word32 ACMDTMFPlayout::CodecDef(WebRtcNetEQ_CodecDef& ,
                                       const CodecInst& ) {
  return -1;
}

ACMGenericCodec* ACMDTMFPlayout::CreateInstance(void) {
  return NULL;
}

WebRtc_Word16 ACMDTMFPlayout::InternalCreateEncoder() {
  return -1;
}

WebRtc_Word16 ACMDTMFPlayout::InternalCreateDecoder() {
  return -1;
}

void ACMDTMFPlayout::InternalDestructEncoderInst(void* ) {
  return;
}

void ACMDTMFPlayout::DestructEncoderSafe() {
  return;
}

void ACMDTMFPlayout::DestructDecoderSafe() {
  return;
}

#else     

ACMDTMFPlayout::ACMDTMFPlayout(WebRtc_Word16 codec_id) {
  codec_id_ = codec_id;
}

ACMDTMFPlayout::~ACMDTMFPlayout() {
  return;
}

WebRtc_Word16 ACMDTMFPlayout::InternalEncode(
    WebRtc_UWord8* ,
    WebRtc_Word16* ) {
  return 0;
}

WebRtc_Word16 ACMDTMFPlayout::DecodeSafe(
    WebRtc_UWord8* ,
    WebRtc_Word16 ,
    WebRtc_Word16* ,
    WebRtc_Word16* ,
    WebRtc_Word8* ) {
  return 0;
}

WebRtc_Word16 ACMDTMFPlayout::InternalInitEncoder(
    WebRtcACMCodecParams* ) {
  
  
  return 0;
}

WebRtc_Word16 ACMDTMFPlayout::InternalInitDecoder(
    WebRtcACMCodecParams* ) {
  
  
  return 0;
}

WebRtc_Word32 ACMDTMFPlayout::CodecDef(WebRtcNetEQ_CodecDef& codec_def,
                                       const CodecInst& codec_inst) {
  
  
  
  
  SET_CODEC_PAR((codec_def), kDecoderAVT, codec_inst.pltype, NULL, 8000);
  SET_AVT_FUNCTIONS((codec_def));
  return 0;
}

ACMGenericCodec* ACMDTMFPlayout::CreateInstance(void) {
  return NULL;
}

WebRtc_Word16 ACMDTMFPlayout::InternalCreateEncoder() {
  
  return 0;
}

WebRtc_Word16 ACMDTMFPlayout::InternalCreateDecoder() {
  
  return 0;
}

void ACMDTMFPlayout::InternalDestructEncoderInst(void* ) {
  
  return;
}

void ACMDTMFPlayout::DestructEncoderSafe() {
  
  return;
}

void ACMDTMFPlayout::DestructDecoderSafe() {
  
  return;
}

#endif

}  

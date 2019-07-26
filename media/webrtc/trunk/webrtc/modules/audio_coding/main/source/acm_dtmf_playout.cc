









#include "webrtc/modules/audio_coding/main/source/acm_dtmf_playout.h"

#include "webrtc/modules/audio_coding/main/acm2/acm_common_defs.h"
#include "webrtc/modules/audio_coding/main/source/acm_neteq.h"
#include "webrtc/modules/audio_coding/neteq/interface/webrtc_neteq.h"
#include "webrtc/modules/audio_coding/neteq/interface/webrtc_neteq_help_macros.h"
#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc {

namespace acm1 {

#ifndef WEBRTC_CODEC_AVT

ACMDTMFPlayout::ACMDTMFPlayout(
    int16_t ) {
  return;
}

ACMDTMFPlayout::~ACMDTMFPlayout() {
  return;
}

int16_t ACMDTMFPlayout::InternalEncode(
    uint8_t* ,
    int16_t* ) {
  return -1;
}

int16_t ACMDTMFPlayout::DecodeSafe(
    uint8_t* ,
    int16_t ,
    int16_t* ,
    int16_t* ,
    int8_t* ) {
  return -1;
}

int16_t ACMDTMFPlayout::InternalInitEncoder(
    WebRtcACMCodecParams* ) {
  return -1;
}

int16_t ACMDTMFPlayout::InternalInitDecoder(
    WebRtcACMCodecParams* ) {
  return -1;
}

int32_t ACMDTMFPlayout::CodecDef(WebRtcNetEQ_CodecDef& ,
                                 const CodecInst& ) {
  return -1;
}

ACMGenericCodec* ACMDTMFPlayout::CreateInstance(void) {
  return NULL;
}

int16_t ACMDTMFPlayout::InternalCreateEncoder() {
  return -1;
}

int16_t ACMDTMFPlayout::InternalCreateDecoder() {
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

ACMDTMFPlayout::ACMDTMFPlayout(int16_t codec_id) {
  codec_id_ = codec_id;
}

ACMDTMFPlayout::~ACMDTMFPlayout() {
  return;
}

int16_t ACMDTMFPlayout::InternalEncode(
    uint8_t* ,
    int16_t* ) {
  return 0;
}

int16_t ACMDTMFPlayout::DecodeSafe(
    uint8_t* ,
    int16_t ,
    int16_t* ,
    int16_t* ,
    int8_t* ) {
  return 0;
}

int16_t ACMDTMFPlayout::InternalInitEncoder(
    WebRtcACMCodecParams* ) {
  
  
  return 0;
}

int16_t ACMDTMFPlayout::InternalInitDecoder(
    WebRtcACMCodecParams* ) {
  
  
  return 0;
}

int32_t ACMDTMFPlayout::CodecDef(WebRtcNetEQ_CodecDef& codec_def,
                                 const CodecInst& codec_inst) {
  
  
  
  
  SET_CODEC_PAR((codec_def), kDecoderAVT, codec_inst.pltype, NULL, 8000);
  SET_AVT_FUNCTIONS((codec_def));
  return 0;
}

ACMGenericCodec* ACMDTMFPlayout::CreateInstance(void) {
  return NULL;
}

int16_t ACMDTMFPlayout::InternalCreateEncoder() {
  
  return 0;
}

int16_t ACMDTMFPlayout::InternalCreateDecoder() {
  
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

}  

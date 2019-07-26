









#include "webrtc/modules/audio_coding/main/acm2/acm_dtmf_playout.h"

#ifdef WEBRTC_CODEC_AVT
#include "webrtc/modules/audio_coding/main/acm2/acm_common_defs.h"
#include "webrtc/modules/audio_coding/main/acm2/acm_receiver.h"
#include "webrtc/system_wrappers/interface/trace.h"
#endif

namespace webrtc {

namespace acm2 {

#ifndef WEBRTC_CODEC_AVT

ACMDTMFPlayout::ACMDTMFPlayout(int16_t ) { return; }

ACMDTMFPlayout::~ACMDTMFPlayout() { return; }

int16_t ACMDTMFPlayout::InternalEncode(uint8_t* ,
                                       int16_t* ) {
  return -1;
}

int16_t ACMDTMFPlayout::InternalInitEncoder(
    WebRtcACMCodecParams* ) {
  return -1;
}

ACMGenericCodec* ACMDTMFPlayout::CreateInstance(void) { return NULL; }

int16_t ACMDTMFPlayout::InternalCreateEncoder() { return -1; }

void ACMDTMFPlayout::InternalDestructEncoderInst(void* ) {
  return;
}

void ACMDTMFPlayout::DestructEncoderSafe() {
  return;
}

#else  

ACMDTMFPlayout::ACMDTMFPlayout(int16_t codec_id) { codec_id_ = codec_id; }

ACMDTMFPlayout::~ACMDTMFPlayout() { return; }

int16_t ACMDTMFPlayout::InternalEncode(uint8_t* ,
                                       int16_t* ) {
  return 0;
}

int16_t ACMDTMFPlayout::InternalInitEncoder(
    WebRtcACMCodecParams* ) {
  
  
  return 0;
}

ACMGenericCodec* ACMDTMFPlayout::CreateInstance(void) { return NULL; }

int16_t ACMDTMFPlayout::InternalCreateEncoder() {
  
  return 0;
}

void ACMDTMFPlayout::InternalDestructEncoderInst(void* ) {
  
  return;
}

void ACMDTMFPlayout::DestructEncoderSafe() {
  
  return;
}

#endif

}  

}  

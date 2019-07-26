









#include "webrtc/modules/audio_coding/main/acm2/acm_cng.h"

#include "webrtc/modules/audio_coding/codecs/cng/include/webrtc_cng.h"
#include "webrtc/modules/audio_coding/main/acm2/acm_codec_database.h"
#include "webrtc/modules/audio_coding/main/acm2/acm_common_defs.h"
#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc {

namespace acm2 {

ACMCNG::ACMCNG(int16_t codec_id) {
  encoder_inst_ptr_ = NULL;
  codec_id_ = codec_id;
  samp_freq_hz_ = ACMCodecDB::CodecFreq(codec_id_);
  return;
}

ACMCNG::~ACMCNG() {
  if (encoder_inst_ptr_ != NULL) {
    WebRtcCng_FreeEnc(encoder_inst_ptr_);
    encoder_inst_ptr_ = NULL;
  }
  return;
}





int16_t ACMCNG::InternalEncode(uint8_t* ,
                               int16_t* ) {
  return -1;
}





int16_t ACMCNG::InternalInitEncoder(WebRtcACMCodecParams* ) {
  return -1;
}

ACMGenericCodec* ACMCNG::CreateInstance(void) { return NULL; }

int16_t ACMCNG::InternalCreateEncoder() {
  if (WebRtcCng_CreateEnc(&encoder_inst_ptr_) < 0) {
    encoder_inst_ptr_ = NULL;
    return -1;
  } else {
    return 0;
  }
}

void ACMCNG::DestructEncoderSafe() {
  if (encoder_inst_ptr_ != NULL) {
    WebRtcCng_FreeEnc(encoder_inst_ptr_);
    encoder_inst_ptr_ = NULL;
  }
  encoder_exist_ = false;
  encoder_initialized_ = false;
}

void ACMCNG::InternalDestructEncoderInst(void* ptr_inst) {
  if (ptr_inst != NULL) {
    WebRtcCng_FreeEnc(static_cast<CNG_enc_inst*>(ptr_inst));
  }
  return;
}

}  

}  

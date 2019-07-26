









#include "webrtc/modules/audio_coding/main/source/acm_cng.h"

#include "webrtc/modules/audio_coding/codecs/cng/include/webrtc_cng.h"
#include "webrtc/modules/audio_coding/main/source/acm_codec_database.h"
#include "webrtc/modules/audio_coding/main/source/acm_common_defs.h"
#include "webrtc/modules/audio_coding/main/source/acm_neteq.h"
#include "webrtc/modules/audio_coding/neteq/interface/webrtc_neteq.h"
#include "webrtc/modules/audio_coding/neteq/interface/webrtc_neteq_help_macros.h"
#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc {

ACMCNG::ACMCNG(WebRtc_Word16 codec_id) {
  encoder_inst_ptr_ = NULL;
  decoder_inst_ptr_ = NULL;
  codec_id_ = codec_id;
  samp_freq_hz_ = ACMCodecDB::CodecFreq(codec_id_);
  return;
}

ACMCNG::~ACMCNG() {
  if (encoder_inst_ptr_ != NULL) {
    WebRtcCng_FreeEnc(encoder_inst_ptr_);
    encoder_inst_ptr_ = NULL;
  }
  if (decoder_inst_ptr_ != NULL) {
    WebRtcCng_FreeDec(decoder_inst_ptr_);
    decoder_inst_ptr_ = NULL;
  }
  return;
}





WebRtc_Word16 ACMCNG::InternalEncode(WebRtc_UWord8* ,
                                     WebRtc_Word16* ) {
  return -1;
}

WebRtc_Word16 ACMCNG::DecodeSafe(WebRtc_UWord8* ,
                                 WebRtc_Word16 ,
                                 WebRtc_Word16* ,
                                 WebRtc_Word16* ,
                                 WebRtc_Word8* ) {
  return 0;
}





WebRtc_Word16 ACMCNG::InternalInitEncoder(
    WebRtcACMCodecParams* ) {
  return -1;
}

WebRtc_Word16 ACMCNG::InternalInitDecoder(
    WebRtcACMCodecParams* ) {
  return WebRtcCng_InitDec(decoder_inst_ptr_);
}

WebRtc_Word32 ACMCNG::CodecDef(WebRtcNetEQ_CodecDef& codec_def,
                               const CodecInst& codec_inst) {
  if (!decoder_initialized_) {
    
    return -1;
  }
  
  
  
  

  if (samp_freq_hz_ == 8000 || samp_freq_hz_ == 16000 ||
      samp_freq_hz_ == 32000 || samp_freq_hz_ == 48000) {
    SET_CODEC_PAR((codec_def), kDecoderCNG, codec_inst.pltype,
                  decoder_inst_ptr_, samp_freq_hz_);
    SET_CNG_FUNCTIONS((codec_def));
    return 0;
  } else {
    return -1;
  }
}

ACMGenericCodec* ACMCNG::CreateInstance(void) {
  return NULL;
}

WebRtc_Word16 ACMCNG::InternalCreateEncoder() {
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

WebRtc_Word16 ACMCNG::InternalCreateDecoder() {
  if (WebRtcCng_CreateDec(&decoder_inst_ptr_) < 0) {
    decoder_inst_ptr_ = NULL;
    return -1;
  } else {
    return 0;
  }
}

void ACMCNG::DestructDecoderSafe() {
  if (decoder_inst_ptr_ != NULL) {
    WebRtcCng_FreeDec(decoder_inst_ptr_);
    decoder_inst_ptr_ = NULL;
  }
  decoder_exist_ = false;
  decoder_initialized_ = false;
}

void ACMCNG::InternalDestructEncoderInst(void* ptr_inst) {
  if (ptr_inst != NULL) {
    WebRtcCng_FreeEnc(static_cast<CNG_enc_inst*>(ptr_inst));
  }
  return;
}

}  

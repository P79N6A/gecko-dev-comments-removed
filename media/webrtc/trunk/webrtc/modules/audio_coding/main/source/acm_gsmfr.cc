









#include "webrtc/modules/audio_coding/main/source/acm_gsmfr.h"

#include "webrtc/modules/audio_coding/main/source/acm_common_defs.h"
#include "webrtc/modules/audio_coding/main/source/acm_neteq.h"
#include "webrtc/modules/audio_coding/neteq/interface/webrtc_neteq.h"
#include "webrtc/modules/audio_coding/neteq/interface/webrtc_neteq_help_macros.h"
#include "webrtc/system_wrappers/interface/trace.h"
#ifdef WEBRTC_CODEC_GSMFR



#include "gsmfr_interface.h"
#endif

namespace webrtc {

#ifndef WEBRTC_CODEC_GSMFR

ACMGSMFR::ACMGSMFR(int16_t )
    : encoder_inst_ptr_(NULL),
      decoder_inst_ptr_(NULL) {
  return;
}

ACMGSMFR::~ACMGSMFR() {
  return;
}

int16_t ACMGSMFR::InternalEncode(
    uint8_t* ,
    int16_t* ) {
  return -1;
}

int16_t ACMGSMFR::DecodeSafe(uint8_t* ,
                             int16_t ,
                             int16_t* ,
                             int16_t* ,
                             int8_t* ) {
  return -1;
}

int16_t ACMGSMFR::EnableDTX() {
  return -1;
}

int16_t ACMGSMFR::DisableDTX() {
  return -1;
}

int16_t ACMGSMFR::InternalInitEncoder(
    WebRtcACMCodecParams* ) {
  return -1;
}

int16_t ACMGSMFR::InternalInitDecoder(
    WebRtcACMCodecParams* ) {
  return -1;
}

int32_t ACMGSMFR::CodecDef(WebRtcNetEQ_CodecDef& ,
                           const CodecInst& ) {
  return -1;
}

ACMGenericCodec* ACMGSMFR::CreateInstance(void) {
  return NULL;
}

int16_t ACMGSMFR::InternalCreateEncoder() {
  return -1;
}

void ACMGSMFR::DestructEncoderSafe() {
  return;
}

int16_t ACMGSMFR::InternalCreateDecoder() {
  return -1;
}

void ACMGSMFR::DestructDecoderSafe() {
  return;
}

void ACMGSMFR::InternalDestructEncoderInst(void* ) {
  return;
}

#else     

ACMGSMFR::ACMGSMFR(int16_t codec_id)
    : encoder_inst_ptr_(NULL),
      decoder_inst_ptr_(NULL) {
  codec_id_ = codec_id;
  has_internal_dtx_ = true;
  return;
}

ACMGSMFR::~ACMGSMFR() {
  if (encoder_inst_ptr_ != NULL) {
    WebRtcGSMFR_FreeEnc(encoder_inst_ptr_);
    encoder_inst_ptr_ = NULL;
  }
  if (decoder_inst_ptr_ != NULL) {
    WebRtcGSMFR_FreeDec(decoder_inst_ptr_);
    decoder_inst_ptr_ = NULL;
  }
  return;
}

int16_t ACMGSMFR::InternalEncode(uint8_t* bitstream,
                                 int16_t* bitstream_len_byte) {
  *bitstream_len_byte = WebRtcGSMFR_Encode(encoder_inst_ptr_,
                                           &in_audio_[in_audio_ix_read_],
                                           frame_len_smpl_,
                                           (int16_t*)bitstream);
  
  
  in_audio_ix_read_ += frame_len_smpl_;
  return *bitstream_len_byte;
}

int16_t ACMGSMFR::DecodeSafe(uint8_t* ,
                             int16_t ,
                             int16_t* ,
                             int16_t* ,
                             int8_t* ) {
  return 0;
}

int16_t ACMGSMFR::EnableDTX() {
  if (dtx_enabled_) {
    return 0;
  } else if (encoder_exist_) {
    if (WebRtcGSMFR_EncoderInit(encoder_inst_ptr_, 1) < 0) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                   "EnableDTX: cannot init encoder for GSMFR");
      return -1;
    }
    dtx_enabled_ = true;
    return 0;
  } else {
    return -1;
  }
}

int16_t ACMGSMFR::DisableDTX() {
  if (!dtx_enabled_) {
    return 0;
  } else if (encoder_exist_) {
    if (WebRtcGSMFR_EncoderInit(encoder_inst_ptr_, 0) < 0) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                   "DisableDTX: cannot init encoder for GSMFR");
      return -1;
    }
    dtx_enabled_ = false;
    return 0;
  } else {
    
    return 0;
  }
}

int16_t ACMGSMFR::InternalInitEncoder(
    WebRtcACMCodecParams* codec_params) {
  if (WebRtcGSMFR_EncoderInit(encoder_inst_ptr_,
                              ((codec_params->enable_dtx) ? 1 : 0)) < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "InternalInitEncoder: cannot init encoder for GSMFR");
  }
  return 0;
}

int16_t ACMGSMFR::InternalInitDecoder(
    WebRtcACMCodecParams* ) {
  if (WebRtcGSMFR_DecoderInit(decoder_inst_ptr_) < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "InternalInitDecoder: cannot init decoder for GSMFR");
    return -1;
  }
  return 0;
}

int32_t ACMGSMFR::CodecDef(WebRtcNetEQ_CodecDef& codec_def,
                           const CodecInst& codec_inst) {
  if (!decoder_initialized_) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "CodecDef: decoder is not initialized for GSMFR");
    return -1;
  }
  
  
  
  
  SET_CODEC_PAR((codec_def), kDecoderGSMFR, codec_inst.pltype,
                decoder_inst_ptr_, 8000);
  SET_GSMFR_FUNCTIONS((codec_def));
  return 0;
}

ACMGenericCodec* ACMGSMFR::CreateInstance(void) {
  return NULL;
}

int16_t ACMGSMFR::InternalCreateEncoder() {
  if (WebRtcGSMFR_CreateEnc(&encoder_inst_ptr_) < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "InternalCreateEncoder: cannot create instance for GSMFR "
                 "encoder");
    return -1;
  }
  return 0;
}

void ACMGSMFR::DestructEncoderSafe() {
  if (encoder_inst_ptr_ != NULL) {
    WebRtcGSMFR_FreeEnc(encoder_inst_ptr_);
    encoder_inst_ptr_ = NULL;
  }
  encoder_exist_ = false;
  encoder_initialized_ = false;
}

int16_t ACMGSMFR::InternalCreateDecoder() {
  if (WebRtcGSMFR_CreateDec(&decoder_inst_ptr_) < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "InternalCreateDecoder: cannot create instance for GSMFR "
                 "decoder");
    return -1;
  }
  return 0;
}

void ACMGSMFR::DestructDecoderSafe() {
  if (decoder_inst_ptr_ != NULL) {
    WebRtcGSMFR_FreeDec(decoder_inst_ptr_);
    decoder_inst_ptr_ = NULL;
  }
  decoder_exist_ = false;
  decoder_initialized_ = false;
}

void ACMGSMFR::InternalDestructEncoderInst(void* ptr_inst) {
  if (ptr_inst != NULL) {
    WebRtcGSMFR_FreeEnc((GSMFR_encinst_t_*) ptr_inst);
  }
  return;
}

#endif

}  

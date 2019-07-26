








#include "webrtc/modules/audio_coding/main/source/acm_ilbc.h"

#include "webrtc/modules/audio_coding/main/source/acm_common_defs.h"
#include "webrtc/modules/audio_coding/main/source/acm_neteq.h"
#include "webrtc/modules/audio_coding/neteq/interface/webrtc_neteq.h"
#include "webrtc/modules/audio_coding/neteq/interface/webrtc_neteq_help_macros.h"
#include "webrtc/system_wrappers/interface/trace.h"

#ifdef WEBRTC_CODEC_ILBC
#include "webrtc/modules/audio_coding/codecs/ilbc/interface/ilbc.h"
#endif

namespace webrtc {

#ifndef WEBRTC_CODEC_ILBC

ACMILBC::ACMILBC(WebRtc_Word16 )
    : encoder_inst_ptr_(NULL),
      decoder_inst_ptr_(NULL) {
  return;
}

ACMILBC::~ACMILBC() {
  return;
}

WebRtc_Word16 ACMILBC::InternalEncode(
    WebRtc_UWord8* ,
    WebRtc_Word16* ) {
  return -1;
}

WebRtc_Word16 ACMILBC::DecodeSafe(WebRtc_UWord8* ,
                                  WebRtc_Word16 ,
                                  WebRtc_Word16* ,
                                  WebRtc_Word16* ,
                                  WebRtc_Word8* ) {
  return -1;
}

WebRtc_Word16 ACMILBC::InternalInitEncoder(
    WebRtcACMCodecParams* ) {
  return -1;
}

WebRtc_Word16 ACMILBC::InternalInitDecoder(
    WebRtcACMCodecParams* ) {
  return -1;
}

WebRtc_Word32 ACMILBC::CodecDef(WebRtcNetEQ_CodecDef& ,
                                const CodecInst& ) {
  return -1;
}

ACMGenericCodec* ACMILBC::CreateInstance(void) {
  return NULL;
}

WebRtc_Word16 ACMILBC::InternalCreateEncoder() {
  return -1;
}

void ACMILBC::DestructEncoderSafe() {
  return;
}

WebRtc_Word16 ACMILBC::InternalCreateDecoder() {
  return -1;
}

void ACMILBC::DestructDecoderSafe() {
  return;
}

void ACMILBC::InternalDestructEncoderInst(void* ) {
  return;
}

WebRtc_Word16 ACMILBC::SetBitRateSafe(const WebRtc_Word32 ) {
  return -1;
}

#else  

ACMILBC::ACMILBC(WebRtc_Word16 codec_id)
    : encoder_inst_ptr_(NULL),
      decoder_inst_ptr_(NULL) {
  codec_id_ = codec_id;
  return;
}

ACMILBC::~ACMILBC() {
  if (encoder_inst_ptr_ != NULL) {
    WebRtcIlbcfix_EncoderFree(encoder_inst_ptr_);
    encoder_inst_ptr_ = NULL;
  }
  if (decoder_inst_ptr_ != NULL) {
    WebRtcIlbcfix_DecoderFree(decoder_inst_ptr_);
    decoder_inst_ptr_ = NULL;
  }
  return;
}

WebRtc_Word16 ACMILBC::InternalEncode(WebRtc_UWord8* bitstream,
                                      WebRtc_Word16* bitstream_len_byte) {
  *bitstream_len_byte = WebRtcIlbcfix_Encode(encoder_inst_ptr_,
                                             &in_audio_[in_audio_ix_read_],
                                             frame_len_smpl_,
                                             (WebRtc_Word16*)bitstream);
  if (*bitstream_len_byte < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "InternalEncode: error in encode for ILBC");
    return -1;
  }
  
  
  in_audio_ix_read_ += frame_len_smpl_;
  return *bitstream_len_byte;
}

WebRtc_Word16 ACMILBC::DecodeSafe(WebRtc_UWord8* ,
                                  WebRtc_Word16 ,
                                  WebRtc_Word16* ,
                                  WebRtc_Word16* ,
                                  WebRtc_Word8* ) {
  return 0;
}

WebRtc_Word16 ACMILBC::InternalInitEncoder(WebRtcACMCodecParams* codec_params) {
  
  if ((160 == (codec_params->codec_inst).pacsize) ||
      (320 == (codec_params->codec_inst).pacsize)) {
    
    return WebRtcIlbcfix_EncoderInit(encoder_inst_ptr_, 20);
  } else if ((240 == (codec_params->codec_inst).pacsize) ||
      (480 == (codec_params->codec_inst).pacsize)) {
    
    return WebRtcIlbcfix_EncoderInit(encoder_inst_ptr_, 30);
  } else {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "InternalInitEncoder: invalid processing block");
    return -1;
  }
}

WebRtc_Word16 ACMILBC::InternalInitDecoder(WebRtcACMCodecParams* codec_params) {
  
  if ((160 == (codec_params->codec_inst).pacsize) ||
      (320 == (codec_params->codec_inst).pacsize)) {
    
    return WebRtcIlbcfix_DecoderInit(decoder_inst_ptr_, 20);
  } else if ((240 == (codec_params->codec_inst).pacsize) ||
      (480 == (codec_params->codec_inst).pacsize)) {
    
    return WebRtcIlbcfix_DecoderInit(decoder_inst_ptr_, 30);
  } else {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "InternalInitDecoder: invalid processing block");
    return -1;
  }
}

WebRtc_Word32 ACMILBC::CodecDef(WebRtcNetEQ_CodecDef& codec_def,
                                const CodecInst& codec_inst) {
  if (!decoder_initialized_) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "CodeDef: decoder not initialized for ILBC");
    return -1;
  }
  
  
  
  
  SET_CODEC_PAR((codec_def), kDecoderILBC, codec_inst.pltype, decoder_inst_ptr_,
                8000);
  SET_ILBC_FUNCTIONS((codec_def));
  return 0;
}

ACMGenericCodec* ACMILBC::CreateInstance(void) {
  return NULL;
}

WebRtc_Word16 ACMILBC::InternalCreateEncoder() {
  if (WebRtcIlbcfix_EncoderCreate(&encoder_inst_ptr_) < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "InternalCreateEncoder: cannot create instance for ILBC "
                 "encoder");
    return -1;
  }
  return 0;
}

void ACMILBC::DestructEncoderSafe() {
  encoder_initialized_ = false;
  encoder_exist_ = false;
  if (encoder_inst_ptr_ != NULL) {
    WebRtcIlbcfix_EncoderFree(encoder_inst_ptr_);
    encoder_inst_ptr_ = NULL;
  }
}

WebRtc_Word16 ACMILBC::InternalCreateDecoder() {
  if (WebRtcIlbcfix_DecoderCreate(&decoder_inst_ptr_) < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "InternalCreateDecoder: cannot create instance for ILBC "
                 "decoder");
    return -1;
  }
  return 0;
}

void ACMILBC::DestructDecoderSafe() {
  decoder_initialized_ = false;
  decoder_exist_ = false;
  if (decoder_inst_ptr_ != NULL) {
    WebRtcIlbcfix_DecoderFree(decoder_inst_ptr_);
    decoder_inst_ptr_ = NULL;
  }
}

void ACMILBC::InternalDestructEncoderInst(void* ptr_inst) {
  if (ptr_inst != NULL) {
    WebRtcIlbcfix_EncoderFree((iLBC_encinst_t_*) ptr_inst);
  }
  return;
}

WebRtc_Word16 ACMILBC::SetBitRateSafe(const WebRtc_Word32 rate) {
  
  if (rate == 13300) {
    WebRtcIlbcfix_EncoderInit(encoder_inst_ptr_, 30);
  } else if (rate == 15200) {
    WebRtcIlbcfix_EncoderInit(encoder_inst_ptr_, 20);
  } else {
    return -1;
  }
  encoder_params_.codec_inst.rate = rate;

  return 0;
}

#endif

}  

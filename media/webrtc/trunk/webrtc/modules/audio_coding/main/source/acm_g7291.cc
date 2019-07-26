









#include "webrtc/modules/audio_coding/main/source/acm_g7291.h"

#include "webrtc/modules/audio_coding/main/source/acm_common_defs.h"
#include "webrtc/modules/audio_coding/main/source/acm_neteq.h"
#include "webrtc/modules/audio_coding/neteq/interface/webrtc_neteq.h"
#include "webrtc/modules/audio_coding/neteq/interface/webrtc_neteq_help_macros.h"
#include "webrtc/system_wrappers/interface/trace.h"
#ifdef WEBRTC_CODEC_G729_1



#include "g7291_interface.h"
#endif

namespace webrtc {

#ifndef WEBRTC_CODEC_G729_1

ACMG729_1::ACMG729_1(WebRtc_Word16 )
    : encoder_inst_ptr_(NULL),
      decoder_inst_ptr_(NULL),
      my_rate_(32000),
      flag_8khz_(0),
      flag_g729_mode_(0) {
  return;
}

ACMG729_1::~ACMG729_1() {
  return;
}

WebRtc_Word16 ACMG729_1::InternalEncode(
    WebRtc_UWord8* ,
    WebRtc_Word16* ) {
  return -1;
}

WebRtc_Word16 ACMG729_1::DecodeSafe(WebRtc_UWord8* ,
                                    WebRtc_Word16 ,
                                    WebRtc_Word16* ,
                                    WebRtc_Word16* ,
                                    WebRtc_Word8* ) {
  return -1;
}

WebRtc_Word16 ACMG729_1::InternalInitEncoder(
    WebRtcACMCodecParams* ) {
  return -1;
}

WebRtc_Word16 ACMG729_1::InternalInitDecoder(
    WebRtcACMCodecParams* ) {
  return -1;
}

WebRtc_Word32 ACMG729_1::CodecDef(WebRtcNetEQ_CodecDef& ,
                                  const CodecInst& ) {
  return -1;
}

ACMGenericCodec* ACMG729_1::CreateInstance(void) {
  return NULL;
}

WebRtc_Word16 ACMG729_1::InternalCreateEncoder() {
  return -1;
}

void ACMG729_1::DestructEncoderSafe() {
  return;
}

WebRtc_Word16 ACMG729_1::InternalCreateDecoder() {
  return -1;
}

void ACMG729_1::DestructDecoderSafe() {
  return;
}

void ACMG729_1::InternalDestructEncoderInst(void* ) {
  return;
}

WebRtc_Word16 ACMG729_1::SetBitRateSafe(const WebRtc_Word32 ) {
  return -1;
}

#else     

struct G729_1_inst_t_;

ACMG729_1::ACMG729_1(WebRtc_Word16 codec_id)
    : encoder_inst_ptr_(NULL),
      decoder_inst_ptr_(NULL),
      my_rate_(32000),  
      flag_8khz_(0),
      flag_g729_mode_(0) {
  
  
  codec_id_ = codec_id;
  return;
}

ACMG729_1::~ACMG729_1() {
  if (encoder_inst_ptr_ != NULL) {
    WebRtcG7291_Free(encoder_inst_ptr_);
    encoder_inst_ptr_ = NULL;
  }
  if (decoder_inst_ptr_ != NULL) {
    WebRtcG7291_Free(decoder_inst_ptr_);
    decoder_inst_ptr_ = NULL;
  }
  return;
}

WebRtc_Word16 ACMG729_1::InternalEncode(WebRtc_UWord8* bitstream,
                                        WebRtc_Word16* bitstream_len_byte) {

  
  WebRtc_Word16 num_encoded_samples = 0;
  *bitstream_len_byte = 0;

  WebRtc_Word16 byte_length_frame = 0;

  
  
  WebRtc_Word16 num_20ms_frames = (frame_len_smpl_ / 320);
  
  byte_length_frame = my_rate_ / (8 * 50) * num_20ms_frames + (1 -
      flag_g729_mode_);

  
  do {
    *bitstream_len_byte = WebRtcG7291_Encode(encoder_inst_ptr_,
                                             &in_audio_[in_audio_ix_read_],
                                             (WebRtc_Word16*) bitstream,
                                             my_rate_, num_20ms_frames);

    
    
    in_audio_ix_read_ += 160;

    
    if (*bitstream_len_byte < 0) {
      
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                   "InternalEncode: Encode error for G729_1");
      *bitstream_len_byte = 0;
      return -1;
    }

    num_encoded_samples += 160;
  } while (*bitstream_len_byte == 0);

  
  if (*bitstream_len_byte != byte_length_frame) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "InternalEncode: Encode error for G729_1");
    *bitstream_len_byte = 0;
    return -1;
  }

  if (num_encoded_samples != frame_len_smpl_) {
    *bitstream_len_byte = 0;
    return -1;
  }

  return *bitstream_len_byte;
}

WebRtc_Word16 ACMG729_1::DecodeSafe(WebRtc_UWord8* ,
                                    WebRtc_Word16 ,
                                    WebRtc_Word16* ,
                                    WebRtc_Word16* ,
                                    WebRtc_Word8* ) {
  return 0;
}

WebRtc_Word16 ACMG729_1::InternalInitEncoder(
    WebRtcACMCodecParams* codec_params) {
  
  my_rate_ = codec_params->codec_inst.rate;
  return SetBitRateSafe((WebRtc_UWord32) my_rate_);
}

WebRtc_Word16 ACMG729_1::InternalInitDecoder(
    WebRtcACMCodecParams* ) {
  if (WebRtcG7291_DecoderInit(decoder_inst_ptr_) < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "InternalInitDecoder: init decoder failed for G729_1");
    return -1;
  }
  return 0;
}

WebRtc_Word32 ACMG729_1::CodecDef(WebRtcNetEQ_CodecDef& codec_def,
                                  const CodecInst& codec_inst) {
  if (!decoder_initialized_) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "CodeDef: Decoder uninitialized for G729_1");
    return -1;
  }

  
  
  
  
  SET_CODEC_PAR((codec_def), kDecoderG729_1, codec_inst.pltype,
                decoder_inst_ptr_, 16000);
  SET_G729_1_FUNCTIONS((codec_def));
  return 0;
}

ACMGenericCodec* ACMG729_1::CreateInstance(void) {
  return NULL;
}

WebRtc_Word16 ACMG729_1::InternalCreateEncoder() {
  if (WebRtcG7291_Create(&encoder_inst_ptr_) < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "InternalCreateEncoder: create encoder failed for G729_1");
    return -1;
  }
  return 0;
}

void ACMG729_1::DestructEncoderSafe() {
  encoder_exist_ = false;
  encoder_initialized_ = false;
  if (encoder_inst_ptr_ != NULL) {
    WebRtcG7291_Free(encoder_inst_ptr_);
    encoder_inst_ptr_ = NULL;
  }
}

WebRtc_Word16 ACMG729_1::InternalCreateDecoder() {
  if (WebRtcG7291_Create(&decoder_inst_ptr_) < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "InternalCreateDecoder: create decoder failed for G729_1");
    return -1;
  }
  return 0;
}

void ACMG729_1::DestructDecoderSafe() {
  decoder_exist_ = false;
  decoder_initialized_ = false;
  if (decoder_inst_ptr_ != NULL) {
    WebRtcG7291_Free(decoder_inst_ptr_);
    decoder_inst_ptr_ = NULL;
  }
}

void ACMG729_1::InternalDestructEncoderInst(void* ptr_inst) {
  if (ptr_inst != NULL) {
    
  }
  return;
}

WebRtc_Word16 ACMG729_1::SetBitRateSafe(const WebRtc_Word32 rate) {
  
  
  
  
  switch (rate) {
    case 8000: {
      my_rate_ = 8000;
      break;
    }
    case 12000: {
      my_rate_ = 12000;
      break;
    }
    case 14000: {
      my_rate_ = 14000;
      break;
    }
    case 16000: {
      my_rate_ = 16000;
      break;
    }
    case 18000: {
      my_rate_ = 18000;
      break;
    }
    case 20000: {
      my_rate_ = 20000;
      break;
    }
    case 22000: {
      my_rate_ = 22000;
      break;
    }
    case 24000: {
      my_rate_ = 24000;
      break;
    }
    case 26000: {
      my_rate_ = 26000;
      break;
    }
    case 28000: {
      my_rate_ = 28000;
      break;
    }
    case 30000: {
      my_rate_ = 30000;
      break;
    }
    case 32000: {
      my_rate_ = 32000;
      break;
    }
    default: {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                   "SetBitRateSafe: Invalid rate G729_1");
      return -1;
    }
  }

  
  if (WebRtcG7291_EncoderInit(encoder_inst_ptr_, my_rate_, flag_8khz_,
                              flag_g729_mode_) >= 0) {
    encoder_params_.codec_inst.rate = my_rate_;
    return 0;
  } else {
    return -1;
  }
}

#endif

}  

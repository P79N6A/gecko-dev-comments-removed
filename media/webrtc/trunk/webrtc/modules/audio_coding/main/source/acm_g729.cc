









#include "webrtc/modules/audio_coding/main/source/acm_g729.h"

#include "webrtc/modules/audio_coding/main/source/acm_common_defs.h"
#include "webrtc/modules/audio_coding/main/source/acm_neteq.h"
#include "webrtc/modules/audio_coding/neteq/interface/webrtc_neteq.h"
#include "webrtc/modules/audio_coding/neteq/interface/webrtc_neteq_help_macros.h"
#include "webrtc/system_wrappers/interface/trace.h"

#ifdef WEBRTC_CODEC_G729



#include "g729_interface.h"
#endif

namespace webrtc {

#ifndef WEBRTC_CODEC_G729

ACMG729::ACMG729(WebRtc_Word16 )
: encoder_inst_ptr_(NULL),
  decoder_inst_ptr_(NULL) {
  return;
}

ACMG729::~ACMG729() {
  return;
}

WebRtc_Word16 ACMG729::InternalEncode(
    WebRtc_UWord8* ,
    WebRtc_Word16* ) {
  return -1;
}

WebRtc_Word16 ACMG729::EnableDTX() {
  return -1;
}

WebRtc_Word16 ACMG729::DisableDTX() {
  return -1;
}

WebRtc_Word32 ACMG729::ReplaceInternalDTXSafe(
    const bool ) {
  return -1;
}

WebRtc_Word32 ACMG729::IsInternalDTXReplacedSafe(
    bool* ) {
  return -1;
}

WebRtc_Word16 ACMG729::DecodeSafe(WebRtc_UWord8* ,
                                  WebRtc_Word16 ,
                                  WebRtc_Word16* ,
                                  WebRtc_Word16* ,
                                  WebRtc_Word8* ) {
  return -1;
}

WebRtc_Word16 ACMG729::InternalInitEncoder(
    WebRtcACMCodecParams* ) {
  return -1;
}

WebRtc_Word16 ACMG729::InternalInitDecoder(
    WebRtcACMCodecParams* ) {
  return -1;
}

WebRtc_Word32 ACMG729::CodecDef(WebRtcNetEQ_CodecDef& ,
                                const CodecInst& ) {
  return -1;
}

ACMGenericCodec* ACMG729::CreateInstance(void) {
  return NULL;
}

WebRtc_Word16 ACMG729::InternalCreateEncoder() {
  return -1;
}

void ACMG729::DestructEncoderSafe() {
  return;
}

WebRtc_Word16 ACMG729::InternalCreateDecoder() {
  return -1;
}

void ACMG729::DestructDecoderSafe() {
  return;
}

void ACMG729::InternalDestructEncoderInst(void* ) {
  return;
}

#else     
ACMG729::ACMG729(WebRtc_Word16 codec_id)
    : encoder_inst_ptr_(NULL),
      decoder_inst_ptr_(NULL) {
  codec_id_ = codec_id;
  has_internal_dtx_ = true;
  return;
}

ACMG729::~ACMG729() {
  if (encoder_inst_ptr_ != NULL) {
    
    WebRtcG729_FreeEnc(encoder_inst_ptr_);
    encoder_inst_ptr_ = NULL;
  }
  if (decoder_inst_ptr_ != NULL) {
    
    WebRtcG729_FreeDec(decoder_inst_ptr_);
    decoder_inst_ptr_ = NULL;
  }
  return;
}

WebRtc_Word16 ACMG729::InternalEncode(WebRtc_UWord8* bitstream,
                                      WebRtc_Word16* bitstream_len_byte) {
  
  WebRtc_Word16 num_encoded_samples = 0;
  WebRtc_Word16 tmp_len_byte = 0;
  WebRtc_Word16 vad_decision = 0;
  *bitstream_len_byte = 0;
  while (num_encoded_samples < frame_len_smpl_) {
    
    
    tmp_len_byte = WebRtcG729_Encode(
        encoder_inst_ptr_, &in_audio_[in_audio_ix_read_], 80,
        (WebRtc_Word16*)(&(bitstream[*bitstream_len_byte])));

    
    
    in_audio_ix_read_ += 80;

    
    if (tmp_len_byte < 0) {
      
      *bitstream_len_byte = 0;
      return -1;
    }

    
    *bitstream_len_byte += tmp_len_byte;
    switch (tmp_len_byte) {
      case 0: {
        if (0 == num_encoded_samples) {
          
          
          
          
          return 0;
        }
        break;
      }
      case 2: {
        
        if (has_internal_dtx_ && dtx_enabled_) {
          vad_decision = 0;
          for (WebRtc_Word16 n = 0; n < MAX_FRAME_SIZE_10MSEC; n++) {
            vad_label_[n] = vad_decision;
          }
        }
        
        
        return *bitstream_len_byte;
      }
      case 10: {
        vad_decision = 1;
        
        break;
      }
      default: {
        return -1;
      }
    }

    
    num_encoded_samples += 80;
  }

  
  if (has_internal_dtx_ && !vad_decision && dtx_enabled_) {
    for (WebRtc_Word16 n = 0; n < MAX_FRAME_SIZE_10MSEC; n++) {
      vad_label_[n] = vad_decision;
    }
  }

  
  return *bitstream_len_byte;
}

WebRtc_Word16 ACMG729::EnableDTX() {
  if (dtx_enabled_) {
    
    return 0;
  } else if (encoder_exist_) {
    
    if (WebRtcG729_EncoderInit(encoder_inst_ptr_, 1) < 0) {
      return -1;
    }
    dtx_enabled_ = true;
    return 0;
  } else {
    return -1;
  }
}

WebRtc_Word16 ACMG729::DisableDTX() {
  if (!dtx_enabled_) {
    
    return 0;
  } else if (encoder_exist_) {
    
    if (WebRtcG729_EncoderInit(encoder_inst_ptr_, 0) < 0) {
      return -1;
    }
    dtx_enabled_ = false;
    return 0;
  } else {
    
    return 0;
  }
}

WebRtc_Word32 ACMG729::ReplaceInternalDTXSafe(const bool replace_internal_dtx) {
  
  

  if (replace_internal_dtx == has_internal_dtx_) {
    
    bool old_enable_dtx = dtx_enabled_;
    bool old_enable_vad = vad_enabled_;
    ACMVADMode old_mode = vad_mode_;
    if (replace_internal_dtx) {
      
      DisableDTX();
    } else {
      
      ACMGenericCodec::DisableDTX();
    }
    has_internal_dtx_ = !replace_internal_dtx;
    WebRtc_Word16 status = SetVADSafe(old_enable_dtx, old_enable_vad, old_mode);
    
    
    if (status == 1) {
      vad_enabled_ = true;
      return status;
    } else if (status < 0) {
      has_internal_dtx_ = replace_internal_dtx;
      return -1;
    }
  }
  return 0;
}

WebRtc_Word32 ACMG729::IsInternalDTXReplacedSafe(bool* internal_dtx_replaced) {
  
  *internal_dtx_replaced = !has_internal_dtx_;
  return 0;
}

WebRtc_Word16 ACMG729::DecodeSafe(WebRtc_UWord8* ,
                                  WebRtc_Word16 ,
                                  WebRtc_Word16* ,
                                  WebRtc_Word16* ,
                                  WebRtc_Word8* ) {
  
  return 0;
}

WebRtc_Word16 ACMG729::InternalInitEncoder(WebRtcACMCodecParams* codec_params) {
  
  return WebRtcG729_EncoderInit(encoder_inst_ptr_,
                                ((codec_params->enable_dtx) ? 1 : 0));
}

WebRtc_Word16 ACMG729::InternalInitDecoder(
    WebRtcACMCodecParams* ) {
  
  return WebRtcG729_DecoderInit(decoder_inst_ptr_);
}

WebRtc_Word32 ACMG729::CodecDef(WebRtcNetEQ_CodecDef& codec_def,
                                const CodecInst& codec_inst) {
  if (!decoder_initialized_) {
    
    
    return -1;
  }

  
  
  
  
  SET_CODEC_PAR((codec_def), kDecoderG729, codec_inst.pltype, decoder_inst_ptr_,
                8000);
  SET_G729_FUNCTIONS((codec_def));
  return 0;
}

ACMGenericCodec* ACMG729::CreateInstance(void) {
  
  return NULL;
}

WebRtc_Word16 ACMG729::InternalCreateEncoder() {
  
  return WebRtcG729_CreateEnc(&encoder_inst_ptr_);
}

void ACMG729::DestructEncoderSafe() {
  
  encoder_exist_ = false;
  encoder_initialized_ = false;
  if (encoder_inst_ptr_ != NULL) {
    WebRtcG729_FreeEnc(encoder_inst_ptr_);
    encoder_inst_ptr_ = NULL;
  }
}

WebRtc_Word16 ACMG729::InternalCreateDecoder() {
  
  return WebRtcG729_CreateDec(&decoder_inst_ptr_);
}

void ACMG729::DestructDecoderSafe() {
  
  decoder_exist_ = false;
  decoder_initialized_ = false;
  if (decoder_inst_ptr_ != NULL) {
    WebRtcG729_FreeDec(decoder_inst_ptr_);
    decoder_inst_ptr_ = NULL;
  }
}

void ACMG729::InternalDestructEncoderInst(void* ptr_inst) {
  if (ptr_inst != NULL) {
    WebRtcG729_FreeEnc((G729_encinst_t_*) ptr_inst);
  }
  return;
}

#endif

}  

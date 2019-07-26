









#include "webrtc/modules/audio_coding/main/source/acm_g722.h"

#include "webrtc/modules/audio_coding/codecs/g722/include/g722_interface.h"
#include "webrtc/modules/audio_coding/main/source/acm_codec_database.h"
#include "webrtc/modules/audio_coding/main/acm2/acm_common_defs.h"
#include "webrtc/modules/audio_coding/main/source/acm_neteq.h"
#include "webrtc/modules/audio_coding/neteq/interface/webrtc_neteq.h"
#include "webrtc/modules/audio_coding/neteq/interface/webrtc_neteq_help_macros.h"
#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc {

namespace acm1 {

#ifndef WEBRTC_CODEC_G722

ACMG722::ACMG722(int16_t )
    : ptr_enc_str_(NULL),
      ptr_dec_str_(NULL),
      encoder_inst_ptr_(NULL),
      encoder_inst_ptr_right_(NULL),
      decoder_inst_ptr_(NULL) {}

ACMG722::~ACMG722() {}

int32_t ACMG722::Add10MsDataSafe(
    const uint32_t ,
    const int16_t* ,
    const uint16_t ,
    const uint8_t ) {
  return -1;
}

int16_t ACMG722::InternalEncode(
    uint8_t* ,
    int16_t* ) {
  return -1;
}

int16_t ACMG722::DecodeSafe(uint8_t* ,
                            int16_t ,
                            int16_t* ,
                            int16_t* ,
                            int8_t* ) {
  return -1;
}

int16_t ACMG722::InternalInitEncoder(
    WebRtcACMCodecParams* ) {
  return -1;
}

int16_t ACMG722::InternalInitDecoder(
    WebRtcACMCodecParams* ) {
  return -1;
}

int32_t ACMG722::CodecDef(WebRtcNetEQ_CodecDef& ,
                          const CodecInst& ) {
  return -1;
}

ACMGenericCodec* ACMG722::CreateInstance(void) {
  return NULL;
}

int16_t ACMG722::InternalCreateEncoder() {
  return -1;
}

void ACMG722::DestructEncoderSafe() {
  return;
}

int16_t ACMG722::InternalCreateDecoder() {
  return -1;
}

void ACMG722::DestructDecoderSafe() {
  return;
}

void ACMG722::InternalDestructEncoderInst(void* ) {
  return;
}

void ACMG722::SplitStereoPacket(uint8_t* ,
                                int32_t* ) {}

#else     


struct ACMG722EncStr {
  G722EncInst* inst;  
  G722EncInst* inst_right;  
};
struct ACMG722DecStr {
  G722DecInst* inst;  
  G722DecInst* inst_right;  
};

ACMG722::ACMG722(int16_t codec_id)
    : encoder_inst_ptr_(NULL),
      encoder_inst_ptr_right_(NULL),
      decoder_inst_ptr_(NULL) {
  
  ptr_enc_str_ = new ACMG722EncStr;
  if (ptr_enc_str_ != NULL) {
    ptr_enc_str_->inst = NULL;
    ptr_enc_str_->inst_right = NULL;
  }
  
  ptr_dec_str_ = new ACMG722DecStr;
  if (ptr_dec_str_ != NULL) {
    ptr_dec_str_->inst = NULL;
    ptr_dec_str_->inst_right = NULL;  
  }
  codec_id_ = codec_id;
  return;
}

ACMG722::~ACMG722() {
  
  if (ptr_enc_str_ != NULL) {
    if (ptr_enc_str_->inst != NULL) {
      WebRtcG722_FreeEncoder(ptr_enc_str_->inst);
      ptr_enc_str_->inst = NULL;
    }
    if (ptr_enc_str_->inst_right != NULL) {
      WebRtcG722_FreeEncoder(ptr_enc_str_->inst_right);
      ptr_enc_str_->inst_right = NULL;
    }
    delete ptr_enc_str_;
    ptr_enc_str_ = NULL;
  }
  
  if (ptr_dec_str_ != NULL) {
    if (ptr_dec_str_->inst != NULL) {
      WebRtcG722_FreeDecoder(ptr_dec_str_->inst);
      ptr_dec_str_->inst = NULL;
    }
    if (ptr_dec_str_->inst_right != NULL) {
      WebRtcG722_FreeDecoder(ptr_dec_str_->inst_right);
      ptr_dec_str_->inst_right = NULL;
    }
    delete ptr_dec_str_;
    ptr_dec_str_ = NULL;
  }
  return;
}

int32_t ACMG722::Add10MsDataSafe(const uint32_t timestamp,
                                 const int16_t* data,
                                 const uint16_t length_smpl,
                                 const uint8_t audio_channel) {
  return ACMGenericCodec::Add10MsDataSafe((timestamp >> 1), data, length_smpl,
                                          audio_channel);
}

int16_t ACMG722::InternalEncode(uint8_t* bitstream,
                                int16_t* bitstream_len_byte) {
  
  if (num_channels_ == 2) {
    int16_t left_channel[960];
    int16_t right_channel[960];
    uint8_t out_left[480];
    uint8_t out_right[480];
    int16_t len_in_bytes;
    for (int i = 0, j = 0; i < frame_len_smpl_ * 2; i += 2, j++) {
      left_channel[j] = in_audio_[in_audio_ix_read_ + i];
      right_channel[j] = in_audio_[in_audio_ix_read_ + i + 1];
    }
    len_in_bytes = WebRtcG722_Encode(encoder_inst_ptr_, left_channel,
                                     frame_len_smpl_,
                                     (int16_t*)out_left);
    len_in_bytes += WebRtcG722_Encode(encoder_inst_ptr_right_, right_channel,
                                      frame_len_smpl_,
                                      (int16_t*)out_right);
    *bitstream_len_byte = len_in_bytes;

    
    for (int i = 0, j = 0; i < len_in_bytes; i += 2, j++) {
      bitstream[i] = (out_left[j] & 0xF0) + (out_right[j] >> 4);
      bitstream[i + 1] = ((out_left[j] & 0x0F) << 4) + (out_right[j] & 0x0F);
    }
  } else {
    *bitstream_len_byte = WebRtcG722_Encode(encoder_inst_ptr_,
                                            &in_audio_[in_audio_ix_read_],
                                            frame_len_smpl_,
                                            (int16_t*)bitstream);
  }

  
  
  in_audio_ix_read_ += frame_len_smpl_ * num_channels_;
  return *bitstream_len_byte;
}

int16_t ACMG722::DecodeSafe(uint8_t* ,
                            int16_t ,
                            int16_t* ,
                            int16_t* ,
                            int8_t* ) {
  return 0;
}

int16_t ACMG722::InternalInitEncoder(WebRtcACMCodecParams* codec_params) {
  if (codec_params->codec_inst.channels == 2) {
    
    if (ptr_enc_str_->inst_right == NULL) {
      WebRtcG722_CreateEncoder(&ptr_enc_str_->inst_right);
      if (ptr_enc_str_->inst_right == NULL) {
        return -1;
      }
    }
    encoder_inst_ptr_right_ = ptr_enc_str_->inst_right;
    if (WebRtcG722_EncoderInit(encoder_inst_ptr_right_) < 0) {
      return -1;
    }
  }

  return WebRtcG722_EncoderInit(encoder_inst_ptr_);
}

int16_t ACMG722::InternalInitDecoder(
    WebRtcACMCodecParams* ) {
  return WebRtcG722_DecoderInit(decoder_inst_ptr_);
}

int32_t ACMG722::CodecDef(WebRtcNetEQ_CodecDef& codec_def,
                          const CodecInst& codec_inst) {
  if (!decoder_initialized_) {
    
    return -1;
  }
  
  
  
  
  if (codec_inst.channels == 1) {
    SET_CODEC_PAR(codec_def, kDecoderG722, codec_inst.pltype, decoder_inst_ptr_,
                  16000);
  } else {
    SET_CODEC_PAR(codec_def, kDecoderG722_2ch, codec_inst.pltype,
                  decoder_inst_ptr_, 16000);
  }
  SET_G722_FUNCTIONS(codec_def);
  return 0;
}

ACMGenericCodec* ACMG722::CreateInstance(void) {
  return NULL;
}

int16_t ACMG722::InternalCreateEncoder() {
  if (ptr_enc_str_ == NULL) {
    
    
    
    return -1;
  }
  WebRtcG722_CreateEncoder(&ptr_enc_str_->inst);
  if (ptr_enc_str_->inst == NULL) {
    return -1;
  }
  encoder_inst_ptr_ = ptr_enc_str_->inst;
  return 0;
}

void ACMG722::DestructEncoderSafe() {
  if (ptr_enc_str_ != NULL) {
    if (ptr_enc_str_->inst != NULL) {
      WebRtcG722_FreeEncoder(ptr_enc_str_->inst);
      ptr_enc_str_->inst = NULL;
    }
  }
  encoder_exist_ = false;
  encoder_initialized_ = false;
}

int16_t ACMG722::InternalCreateDecoder() {
  if (ptr_dec_str_ == NULL) {
    
    
    
    return -1;
  }

  WebRtcG722_CreateDecoder(&ptr_dec_str_->inst);
  if (ptr_dec_str_->inst == NULL) {
    return -1;
  }
  decoder_inst_ptr_ = ptr_dec_str_->inst;
  return 0;
}

void ACMG722::DestructDecoderSafe() {
  decoder_exist_ = false;
  decoder_initialized_ = false;
  if (ptr_dec_str_ != NULL) {
    if (ptr_dec_str_->inst != NULL) {
      WebRtcG722_FreeDecoder(ptr_dec_str_->inst);
      ptr_dec_str_->inst = NULL;
    }
  }
}

void ACMG722::InternalDestructEncoderInst(void* ptr_inst) {
  if (ptr_inst != NULL) {
    WebRtcG722_FreeEncoder(static_cast<G722EncInst*>(ptr_inst));
  }
  return;
}



void ACMG722::SplitStereoPacket(uint8_t* payload, int32_t* payload_length) {
  uint8_t right_byte;

  
  assert(payload != NULL);
  assert(*payload_length > 0);

  
  
  
  for (int i = 0; i < *payload_length; i += 2) {
    right_byte = ((payload[i] & 0x0F) << 4) + (payload[i + 1] & 0x0F);
    payload[i] = (payload[i] & 0xF0) + (payload[i + 1] >> 4);
    payload[i + 1] = right_byte;
  }

  
  
  
  
  for (int i = 0; i < *payload_length / 2; i++) {
    right_byte = payload[i + 1];
    memmove(&payload[i + 1], &payload[i + 2], *payload_length - i - 2);
    payload[*payload_length - 1] = right_byte;
  }
}

#endif

}  

}  

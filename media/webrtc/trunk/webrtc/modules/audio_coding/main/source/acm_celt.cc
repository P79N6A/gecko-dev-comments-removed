









#include "webrtc/modules/audio_coding/main/source/acm_celt.h"

#include "webrtc/modules/audio_coding/main/source/acm_common_defs.h"
#include "webrtc/modules/audio_coding/main/source/acm_neteq.h"
#include "webrtc/modules/audio_coding/neteq/interface/webrtc_neteq.h"
#include "webrtc/modules/audio_coding/neteq/interface/webrtc_neteq_help_macros.h"
#include "webrtc/system_wrappers/interface/trace.h"

#ifdef WEBRTC_CODEC_CELT


#include "celt_interface.h"
#endif

namespace webrtc {

#ifndef WEBRTC_CODEC_CELT

ACMCELT::ACMCELT(int16_t )
    : enc_inst_ptr_(NULL),
      dec_inst_ptr_(NULL),
      sampling_freq_(0),
      bitrate_(0),
      channels_(1),
      dec_channels_(1) {
  return;
}

ACMCELT::~ACMCELT() {
  return;
}

int16_t ACMCELT::InternalEncode(uint8_t* ,
                                int16_t* ) {
  return -1;
}

int16_t ACMCELT::DecodeSafe(uint8_t* ,
                            int16_t ,
                            int16_t* ,
                            int16_t* ,
                            WebRtc_Word8* ) {
  return -1;
}

int16_t ACMCELT::InternalInitEncoder(WebRtcACMCodecParams* ) {
  return -1;
}

int16_t ACMCELT::InternalInitDecoder(WebRtcACMCodecParams* ) {
  return -1;
}

int32_t ACMCELT::CodecDef(WebRtcNetEQ_CodecDef& ,
                          const CodecInst& ) {
  return -1;
}

ACMGenericCodec* ACMCELT::CreateInstance(void) {
  return NULL;
}

int16_t ACMCELT::InternalCreateEncoder() {
  return -1;
}

void ACMCELT::DestructEncoderSafe() {
  return;
}

int16_t ACMCELT::InternalCreateDecoder() {
  return -1;
}

void ACMCELT::DestructDecoderSafe() {
  return;
}

void ACMCELT::InternalDestructEncoderInst(void* ) {
  return;
}

bool ACMCELT::IsTrueStereoCodec() {
  return true;
}

int16_t ACMCELT::SetBitRateSafe(const int32_t ) {
  return -1;
}

void ACMCELT::SplitStereoPacket(uint8_t* ,
                                int32_t* ) {}

#else  

ACMCELT::ACMCELT(int16_t codec_id)
    : enc_inst_ptr_(NULL),
      dec_inst_ptr_(NULL),
      sampling_freq_(32000),  
      bitrate_(64000),  
      channels_(1),  
      dec_channels_(1) {  
  
  codec_id_ = codec_id;

  return;
}

ACMCELT::~ACMCELT() {
  if (enc_inst_ptr_ != NULL) {
    WebRtcCelt_FreeEnc(enc_inst_ptr_);
    enc_inst_ptr_ = NULL;
  }
  if (dec_inst_ptr_ != NULL) {
    WebRtcCelt_FreeDec(dec_inst_ptr_);
    dec_inst_ptr_ = NULL;
  }
  return;
}

int16_t ACMCELT::InternalEncode(uint8_t* bitstream,
                                int16_t* bitstream_len_byte) {
  *bitstream_len_byte = 0;

  
  *bitstream_len_byte = WebRtcCelt_Encode(enc_inst_ptr_,
                                          &in_audio_[in_audio_ix_read_],
                                          bitstream);

  
  
  in_audio_ix_read_ += frame_len_smpl_ * channels_;

  if (*bitstream_len_byte < 0) {
    
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "InternalEncode: Encode error for Celt");
    *bitstream_len_byte = 0;
    return -1;
  }

  return *bitstream_len_byte;
}

int16_t ACMCELT::DecodeSafe(uint8_t* ,
                            int16_t ,
                            int16_t* ,
                            int16_t* ,
                            WebRtc_Word8* ) {
  return 0;
}

int16_t ACMCELT::InternalInitEncoder(WebRtcACMCodecParams* codec_params) {
  
  int16_t status = SetBitRateSafe((codec_params->codec_inst).rate);
  if (status < 0) {
    return -1;
  }

  
  if (codec_params->codec_inst.channels != channels_) {
    WebRtcCelt_FreeEnc(enc_inst_ptr_);
    enc_inst_ptr_ = NULL;
    
    channels_ = codec_params->codec_inst.channels;
    if (WebRtcCelt_CreateEnc(&enc_inst_ptr_, channels_) < 0) {
      return -1;
    }
  }

  
  if (WebRtcCelt_EncoderInit(enc_inst_ptr_, channels_, bitrate_) >= 0) {
    return 0;
  } else {
    return -1;
  }
}

int16_t ACMCELT::InternalInitDecoder(WebRtcACMCodecParams* codec_params) {
  
  if (codec_params->codec_inst.channels != dec_channels_) {
    WebRtcCelt_FreeDec(dec_inst_ptr_);
    dec_inst_ptr_ = NULL;
    
    dec_channels_ = codec_params->codec_inst.channels;
    if (WebRtcCelt_CreateDec(&dec_inst_ptr_, dec_channels_) < 0) {
      return -1;
    }
  }

  
  if (WebRtcCelt_DecoderInit(dec_inst_ptr_) < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "InternalInitDecoder: init decoder failed for Celt.");
    return -1;
  }
  if (WebRtcCelt_DecoderInitSlave(dec_inst_ptr_) < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "InternalInitDecoder: init decoder failed for Celt.");
    return -1;
  }
  return 0;
}

int32_t ACMCELT::CodecDef(WebRtcNetEQ_CodecDef& codec_def,
                          const CodecInst& codec_inst) {
  if (!decoder_initialized_) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "CodecDef: Decoder uninitialized for Celt");
    return -1;
  }

  
  
  
  
  if (codec_inst.channels == 1) {
    SET_CODEC_PAR(codec_def, kDecoderCELT_32, codec_inst.pltype, dec_inst_ptr_,
                  32000);
  } else {
    SET_CODEC_PAR(codec_def, kDecoderCELT_32_2ch, codec_inst.pltype,
                  dec_inst_ptr_, 32000);
  }

  
  
  if (is_master_) {
    SET_CELT_FUNCTIONS(codec_def);
  } else {
    SET_CELTSLAVE_FUNCTIONS(codec_def);
  }
  return 0;
}

ACMGenericCodec* ACMCELT::CreateInstance(void) {
  return NULL;
}

int16_t ACMCELT::InternalCreateEncoder() {
  if (WebRtcCelt_CreateEnc(&enc_inst_ptr_, num_channels_) < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "InternalCreateEncoder: create encoder failed for Celt");
    return -1;
  }
  channels_ = num_channels_;
  return 0;
}

void ACMCELT::DestructEncoderSafe() {
  encoder_exist_ = false;
  encoder_initialized_ = false;
  if (enc_inst_ptr_ != NULL) {
    WebRtcCelt_FreeEnc(enc_inst_ptr_);
    enc_inst_ptr_ = NULL;
  }
}

int16_t ACMCELT::InternalCreateDecoder() {
  if (WebRtcCelt_CreateDec(&dec_inst_ptr_, dec_channels_) < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "InternalCreateDecoder: create decoder failed for Celt");
    return -1;
  }

  return 0;
}

void ACMCELT::DestructDecoderSafe() {
  decoder_exist_ = false;
  decoder_initialized_ = false;
  if (dec_inst_ptr_ != NULL) {
    WebRtcCelt_FreeDec(dec_inst_ptr_);
    dec_inst_ptr_ = NULL;
  }
}

void ACMCELT::InternalDestructEncoderInst(void* ptr_inst) {
  if (ptr_inst != NULL) {
    WebRtcCelt_FreeEnc(static_cast<CELT_encinst_t*>(ptr_inst));
  }
  return;
}

bool ACMCELT::IsTrueStereoCodec() {
  return true;
}

int16_t ACMCELT::SetBitRateSafe(const int32_t rate) {
  
  if ((rate >= 48000) && (rate <= 128000)) {
    
    bitrate_ = rate;

    
    if (WebRtcCelt_EncoderInit(enc_inst_ptr_, channels_, bitrate_) >= 0) {
      return 0;
    } else {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                   "SetBitRateSafe: Failed to initiate Celt with rate %d",
                   rate);
      return -1;
    }
  } else {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "SetBitRateSafe: Invalid rate Celt, %d", rate);
    return -1;
  }
}


void ACMCELT::SplitStereoPacket(uint8_t* payload, int32_t* payload_length) {
  
  assert(payload != NULL);
  assert(*payload_length > 0);

  
  memcpy(&payload[*payload_length], &payload[0],
         sizeof(uint8_t) * (*payload_length));
  
  *payload_length *= 2;
}

#endif

}  

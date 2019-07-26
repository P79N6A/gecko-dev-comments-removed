









#include "webrtc/modules/audio_coding/main/source/acm_speex.h"

#include "webrtc/modules/audio_coding/main/source/acm_codec_database.h"
#include "webrtc/modules/audio_coding/main/source/acm_common_defs.h"
#include "webrtc/modules/audio_coding/main/source/acm_neteq.h"
#include "webrtc/modules/audio_coding/neteq/interface/webrtc_neteq.h"
#include "webrtc/modules/audio_coding/neteq/interface/webrtc_neteq_help_macros.h"
#include "webrtc/system_wrappers/interface/trace.h"

#ifdef WEBRTC_CODEC_SPEEX


#include "speex_interface.h"
#endif

namespace webrtc {

#ifndef WEBRTC_CODEC_SPEEX
ACMSPEEX::ACMSPEEX(WebRtc_Word16 )
    : encoder_inst_ptr_(NULL),
      decoder_inst_ptr_(NULL),
      compl_mode_(0),
      vbr_enabled_(false),
      encoding_rate_(-1),
      sampling_frequency_(-1),
      samples_in_20ms_audio_(-1) {
  return;
}

ACMSPEEX::~ACMSPEEX() {
  return;
}

WebRtc_Word16 ACMSPEEX::InternalEncode(
    WebRtc_UWord8* ,
    WebRtc_Word16* ) {
  return -1;
}

WebRtc_Word16 ACMSPEEX::DecodeSafe(WebRtc_UWord8* ,
                                   WebRtc_Word16 ,
                                   WebRtc_Word16* ,
                                   WebRtc_Word16* ,
                                   WebRtc_Word8* ) {
  return -1;
}

WebRtc_Word16 ACMSPEEX::EnableDTX() {
  return -1;
}

WebRtc_Word16 ACMSPEEX::DisableDTX() {
  return -1;
}

WebRtc_Word16 ACMSPEEX::InternalInitEncoder(
    WebRtcACMCodecParams* ) {
  return -1;
}

WebRtc_Word16 ACMSPEEX::InternalInitDecoder(
    WebRtcACMCodecParams* ) {
  return -1;
}

WebRtc_Word32 ACMSPEEX::CodecDef(WebRtcNetEQ_CodecDef& ,
                                 const CodecInst& ) {
  return -1;
}

ACMGenericCodec* ACMSPEEX::CreateInstance(void) {
  return NULL;
}

WebRtc_Word16 ACMSPEEX::InternalCreateEncoder() {
  return -1;
}

void ACMSPEEX::DestructEncoderSafe() {
  return;
}

WebRtc_Word16 ACMSPEEX::InternalCreateDecoder() {
  return -1;
}

void ACMSPEEX::DestructDecoderSafe() {
  return;
}

WebRtc_Word16 ACMSPEEX::SetBitRateSafe(const WebRtc_Word32 ) {
  return -1;
}

void ACMSPEEX::InternalDestructEncoderInst(void* ) {
  return;
}

#ifdef UNUSEDSPEEX
WebRtc_Word16 ACMSPEEX::EnableVBR() {
  return -1;
}

WebRtc_Word16 ACMSPEEX::DisableVBR() {
  return -1;
}

WebRtc_Word16 ACMSPEEX::SetComplMode(WebRtc_Word16 mode) {
  return -1;
}
#endif

#else  

ACMSPEEX::ACMSPEEX(WebRtc_Word16 codec_id)
    : encoder_inst_ptr_(NULL),
      decoder_inst_ptr_(NULL) {
  codec_id_ = codec_id;

  
  if (codec_id_ == ACMCodecDB::kSPEEX8) {
    sampling_frequency_ = 8000;
    samples_in_20ms_audio_ = 160;
    encoding_rate_ = 11000;
  } else if (codec_id_ == ACMCodecDB::kSPEEX16) {
    sampling_frequency_ = 16000;
    samples_in_20ms_audio_ = 320;
    encoding_rate_ = 22000;
  } else {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "Wrong codec id for Speex.");

    sampling_frequency_ = -1;
    samples_in_20ms_audio_ = -1;
    encoding_rate_ = -1;
  }

  has_internal_dtx_ = true;
  dtx_enabled_ = false;
  vbr_enabled_ = false;
  compl_mode_ = 3;  

  return;
}

ACMSPEEX::~ACMSPEEX() {
  if (encoder_inst_ptr_ != NULL) {
    WebRtcSpeex_FreeEnc(encoder_inst_ptr_);
    encoder_inst_ptr_ = NULL;
  }
  if (decoder_inst_ptr_ != NULL) {
    WebRtcSpeex_FreeDec(decoder_inst_ptr_);
    decoder_inst_ptr_ = NULL;
  }
  return;
}

WebRtc_Word16 ACMSPEEX::InternalEncode(WebRtc_UWord8* bitstream,
                                       WebRtc_Word16* bitstream_len_byte) {
  WebRtc_Word16 status;
  WebRtc_Word16 num_encoded_samples = 0;
  WebRtc_Word16 n = 0;

  while (num_encoded_samples < frame_len_smpl_) {
    status = WebRtcSpeex_Encode(encoder_inst_ptr_,
                                &in_audio_[in_audio_ix_read_], encoding_rate_);

    
    
    in_audio_ix_read_ += samples_in_20ms_audio_;
    num_encoded_samples += samples_in_20ms_audio_;

    if (status < 0) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                   "Error in Speex encoder");
      return status;
    }

    
    if (has_internal_dtx_ && dtx_enabled_) {
      vad_label_[n++] = status;
      vad_label_[n++] = status;
    }

    if (status == 0) {
      
      
      *bitstream_len_byte = WebRtcSpeex_GetBitstream(encoder_inst_ptr_,
                                                     (WebRtc_Word16*)bitstream);
      return *bitstream_len_byte;
    }
  }

  *bitstream_len_byte = WebRtcSpeex_GetBitstream(encoder_inst_ptr_,
                                                 (WebRtc_Word16*)bitstream);
  return *bitstream_len_byte;
}

WebRtc_Word16 ACMSPEEX::DecodeSafe(WebRtc_UWord8* ,
                                   WebRtc_Word16 ,
                                   WebRtc_Word16* ,
                                   WebRtc_Word16* ,
                                   WebRtc_Word8* ) {
  return 0;
}

WebRtc_Word16 ACMSPEEX::EnableDTX() {
  if (dtx_enabled_) {
    return 0;
  } else if (encoder_exist_) {  
    
    if (WebRtcSpeex_EncoderInit(encoder_inst_ptr_, (vbr_enabled_ ? 1 : 0),
                                compl_mode_, 1) < 0) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                   "Cannot enable DTX for Speex");
      return -1;
    }
    dtx_enabled_ = true;
    return 0;
  } else {
    return -1;
  }

  return 0;
}

WebRtc_Word16 ACMSPEEX::DisableDTX() {
  if (!dtx_enabled_) {
    return 0;
  } else if (encoder_exist_) {  
    
    if (WebRtcSpeex_EncoderInit(encoder_inst_ptr_, (vbr_enabled_ ? 1 : 0),
                                compl_mode_, 0) < 0) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                   "Cannot disable DTX for Speex");
      return -1;
    }
    dtx_enabled_ = false;
    return 0;
  } else {
    
    return 0;
  }

  return 0;
}

WebRtc_Word16 ACMSPEEX::InternalInitEncoder(
    WebRtcACMCodecParams* codec_params) {
  
  if (encoder_inst_ptr_ == NULL) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "Cannot initialize Speex encoder, instance does not exist");
    return -1;
  }

  WebRtc_Word16 status = SetBitRateSafe((codec_params->codecInstant).rate);
  status +=
      (WebRtcSpeex_EncoderInit(encoder_inst_ptr_, vbr_enabled_, compl_mode_,
                               ((codec_params->enable_dtx) ? 1 : 0)) < 0) ?
                                   -1 : 0;

  if (status >= 0) {
    return 0;
  } else {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "Error in initialization of Speex encoder");
    return -1;
  }
}

WebRtc_Word16 ACMSPEEX::InternalInitDecoder(
    WebRtcACMCodecParams* ) {
  WebRtc_Word16 status;

  
  if (decoder_inst_ptr_ == NULL) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "Cannot initialize Speex decoder, instance does not exist");
    return -1;
  }
  status = ((WebRtcSpeex_DecoderInit(decoder_inst_ptr_) < 0) ? -1 : 0);

  if (status >= 0) {
    return 0;
  } else {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "Error in initialization of Speex decoder");
    return -1;
  }
}

WebRtc_Word32 ACMSPEEX::CodecDef(WebRtcNetEQ_CodecDef& codec_def,
                                 const CodecInst& codec_inst) {
  if (!decoder_initialized_) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "Error, Speex decoder is not initialized");
    return -1;
  }

  
  
  
  

  switch (sampling_frequency_) {
    case 8000: {
      SET_CODEC_PAR((codec_def), kDecoderSPEEX_8, codec_inst.pltype,
                    decoder_inst_ptr_, 8000);
      break;
    }
    case 16000: {
      SET_CODEC_PAR((codec_def), kDecoderSPEEX_16, codec_inst.pltype,
                    decoder_inst_ptr_, 16000);
      break;
    }
    default: {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                   "Unsupported sampling frequency for Speex");

      return -1;
    }
  }

  SET_SPEEX_FUNCTIONS((codec_def));
  return 0;
}

ACMGenericCodec* ACMSPEEX::CreateInstance(void) {
  return NULL;
}

WebRtc_Word16 ACMSPEEX::InternalCreateEncoder() {
  return WebRtcSpeex_CreateEnc(&encoder_inst_ptr_, sampling_frequency_);
}

void ACMSPEEX::DestructEncoderSafe() {
  if (encoder_inst_ptr_ != NULL) {
    WebRtcSpeex_FreeEnc(encoder_inst_ptr_);
    encoder_inst_ptr_ = NULL;
  }
  
  encoder_exist_ = false;
  encoder_initialized_ = false;
  encoding_rate_ = 0;
}

WebRtc_Word16 ACMSPEEX::InternalCreateDecoder() {
  return WebRtcSpeex_CreateDec(&decoder_inst_ptr_, sampling_frequency_, 1);
}

void ACMSPEEX::DestructDecoderSafe() {
  if (decoder_inst_ptr_ != NULL) {
    WebRtcSpeex_FreeDec(decoder_inst_ptr_);
    decoder_inst_ptr_ = NULL;
  }
  
  decoder_exist_ = false;
  decoder_initialized_ = false;
}

WebRtc_Word16 ACMSPEEX::SetBitRateSafe(const WebRtc_Word32 rate) {
  
  if (rate == encoding_rate_) {
    return 0;
  } else if (rate > 2000) {
    encoding_rate_ = rate;
    encoder_params_.codecInstant.rate = rate;
  } else {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "Unsupported encoding rate for Speex");

    return -1;
  }

  return 0;
}

void ACMSPEEX::InternalDestructEncoderInst(void* ptr_inst) {
  if (ptr_inst != NULL) {
    WebRtcSpeex_FreeEnc((SPEEX_encinst_t_*) ptr_inst);
  }
  return;
}

#ifdef UNUSEDSPEEX



WebRtc_Word16 ACMSPEEX::EnableVBR() {
  if (vbr_enabled_) {
    return 0;
  } else if (encoder_exist_) {  
    
    if (WebRtcSpeex_EncoderInit(encoder_inst_ptr_, 1, compl_mode_,
                                (dtx_enabled_ ? 1 : 0)) < 0) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                   "Cannot enable VBR mode for Speex");

      return -1;
    }
    vbr_enabled_ = true;
    return 0;
  } else {
    return -1;
  }
}



WebRtc_Word16 ACMSPEEX::DisableVBR() {
  if (!vbr_enabled_) {
    return 0;
  } else if (encoder_exist_) {  
    
    if (WebRtcSpeex_EncoderInit(encoder_inst_ptr_, 0, compl_mode_,
                                (dtx_enabled_ ? 1 : 0)) < 0) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                   "Cannot disable DTX for Speex");

      return -1;
    }
    vbr_enabled_ = false;
    return 0;
  } else {
    
    return 0;
  }
}



WebRtc_Word16 ACMSPEEX::SetComplMode(WebRtc_Word16 mode) {
  
  if (mode == compl_mode_) {
    return 0;
  } else if (encoder_exist_) {  
    
    if (WebRtcSpeex_EncoderInit(encoder_inst_ptr_, 0, mode,
                                (dtx_enabled_ ? 1 : 0)) < 0) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                   "Error in complexity mode for Speex");
      return -1;
    }
    compl_mode_ = mode;
    return 0;
  } else {
    
    return 0;
  }
}

#endif

#endif

}  

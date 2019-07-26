









#include "webrtc/modules/audio_coding/main/source/acm_generic_codec.h"

#include <assert.h>
#include <string.h>

#include "webrtc/common_audio/vad/include/webrtc_vad.h"
#include "webrtc/modules/audio_coding/codecs/cng/include/webrtc_cng.h"
#include "webrtc/modules/audio_coding/main/source/acm_codec_database.h"
#include "webrtc/modules/audio_coding/main/source/acm_common_defs.h"
#include "webrtc/modules/audio_coding/main/source/acm_neteq.h"
#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc {


enum {
  kMaxPLCParamsCNG = WEBRTC_CNG_MAX_LPC_ORDER,
  kNewCNGNumPLCParams = 8
};


enum {
  kCngSidIntervalMsec = 100
};




ACMGenericCodec::ACMGenericCodec()
    : in_audio_ix_write_(0),
      in_audio_ix_read_(0),
      in_timestamp_ix_write_(0),
      in_audio_(NULL),
      in_timestamp_(NULL),
      frame_len_smpl_(-1),  
      num_channels_(1),
      codec_id_(-1),  
      num_missed_samples_(0),
      encoder_exist_(false),
      decoder_exist_(false),
      encoder_initialized_(false),
      decoder_initialized_(false),
      registered_in_neteq_(false),
      has_internal_dtx_(false),
      ptr_vad_inst_(NULL),
      vad_enabled_(false),
      vad_mode_(VADNormal),
      dtx_enabled_(false),
      ptr_dtx_inst_(NULL),
      num_lpc_params_(kNewCNGNumPLCParams),
      sent_cn_previous_(false),
      is_master_(true),
      prev_frame_cng_(0),
      neteq_decode_lock_(NULL),
      codec_wrapper_lock_(*RWLockWrapper::CreateRWLock()),
      last_encoded_timestamp_(0),
      last_timestamp_(0xD87F3F9F),
      is_audio_buff_fresh_(true),
      unique_id_(0) {
  
  for (int i = 0; i < MAX_FRAME_SIZE_10MSEC; i++) {
    vad_label_[i] = 0;
  }
  
  
  memset(&encoder_params_, 0, sizeof(WebRtcACMCodecParams));
  encoder_params_.codec_inst.pltype = -1;
  memset(&decoder_params_, 0, sizeof(WebRtcACMCodecParams));
  decoder_params_.codec_inst.pltype = -1;
}

ACMGenericCodec::~ACMGenericCodec() {
  
  
  if (ptr_vad_inst_ != NULL) {
    WebRtcVad_Free(ptr_vad_inst_);
    ptr_vad_inst_ = NULL;
  }
  if (in_audio_ != NULL) {
    delete[] in_audio_;
    in_audio_ = NULL;
  }
  if (in_timestamp_ != NULL) {
    delete[] in_timestamp_;
    in_timestamp_ = NULL;
  }
  if (ptr_dtx_inst_ != NULL) {
    WebRtcCng_FreeEnc(ptr_dtx_inst_);
    ptr_dtx_inst_ = NULL;
  }
  delete &codec_wrapper_lock_;
}

int32_t ACMGenericCodec::Add10MsData(const uint32_t timestamp,
                                     const int16_t* data,
                                     const uint16_t length_smpl,
                                     const uint8_t audio_channel) {
  WriteLockScoped wl(codec_wrapper_lock_);
  return Add10MsDataSafe(timestamp, data, length_smpl, audio_channel);
}

int32_t ACMGenericCodec::Add10MsDataSafe(const uint32_t timestamp,
                                         const int16_t* data,
                                         const uint16_t length_smpl,
                                         const uint8_t audio_channel) {
  
  
  uint16_t plfreq_hz;
  if (EncoderSampFreq(plfreq_hz) < 0) {
    return -1;
  }

  
  if ((plfreq_hz / 100) != length_smpl) {
    
    return -1;
  }

  if (last_timestamp_ == timestamp) {
    
    if ((in_audio_ix_write_ >= length_smpl * audio_channel) &&
        (in_timestamp_ix_write_ > 0)) {
      in_audio_ix_write_ -= length_smpl * audio_channel;
      in_timestamp_ix_write_--;
      WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceAudioCoding, unique_id_,
                   "Adding 10ms with previous timestamp, overwriting the "
                   "previous 10ms");
    } else {
      WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceAudioCoding, unique_id_,
                   "Adding 10ms with previous timestamp, this will sound bad");
    }
  }

  last_timestamp_ = timestamp;

  
  
  if ((in_audio_ix_write_ + length_smpl * audio_channel) >
      AUDIO_BUFFER_SIZE_W16) {
    
    int16_t missed_samples = in_audio_ix_write_ + length_smpl * audio_channel -
        AUDIO_BUFFER_SIZE_W16;

    
    memmove(in_audio_, in_audio_ + missed_samples,
            (AUDIO_BUFFER_SIZE_W16 - length_smpl * audio_channel) *
            sizeof(int16_t));

    
    memcpy(in_audio_ + (AUDIO_BUFFER_SIZE_W16 - length_smpl * audio_channel),
           data, length_smpl * audio_channel * sizeof(int16_t));

    
    int16_t missed_10ms_blocks =static_cast<int16_t>(
        (missed_samples / audio_channel * 100) / plfreq_hz);

    
    memmove(in_timestamp_, in_timestamp_ + missed_10ms_blocks,
            (in_timestamp_ix_write_ - missed_10ms_blocks) * sizeof(uint32_t));
    in_timestamp_ix_write_ -= missed_10ms_blocks;
    in_timestamp_[in_timestamp_ix_write_] = timestamp;
    in_timestamp_ix_write_++;

    
    in_audio_ix_write_ = AUDIO_BUFFER_SIZE_W16;
    IncreaseNoMissedSamples(missed_samples);
    is_audio_buff_fresh_ = false;
    return -missed_samples;
  }

  
  memcpy(in_audio_ + in_audio_ix_write_, data,
         length_smpl * audio_channel * sizeof(int16_t));
  in_audio_ix_write_ += length_smpl * audio_channel;

  assert(in_timestamp_ix_write_ < TIMESTAMP_BUFFER_SIZE_W32);
  assert(in_timestamp_ix_write_ >= 0);

  in_timestamp_[in_timestamp_ix_write_] = timestamp;
  in_timestamp_ix_write_++;
  is_audio_buff_fresh_ = false;
  return 0;
}

bool ACMGenericCodec::HasFrameToEncode() const {
  ReadLockScoped lockCodec(codec_wrapper_lock_);
  if (in_audio_ix_write_ < frame_len_smpl_ * num_channels_)
    return false;
  return true;
}

int16_t ACMGenericCodec::Encode(uint8_t* bitstream,
                                int16_t* bitstream_len_byte,
                                uint32_t* timestamp,
                                WebRtcACMEncodingType* encoding_type) {
  if (!HasFrameToEncode()) {
    
    *timestamp = 0;
    *bitstream_len_byte = 0;
    
    *encoding_type = kNoEncoding;
    return 0;
  }
  WriteLockScoped lockCodec(codec_wrapper_lock_);
  ReadLockScoped lockNetEq(*neteq_decode_lock_);

  
  
  
  
  
  const int16_t my_basic_coding_block_smpl =
      ACMCodecDB::BasicCodingBlock(codec_id_);
  if (my_basic_coding_block_smpl < 0 || !encoder_initialized_ ||
      !encoder_exist_) {
    
    *timestamp = 0;
    *bitstream_len_byte = 0;
    *encoding_type = kNoEncoding;
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "EncodeSafe: error, basic coding sample block is negative");
    return -1;
  }
  
  in_audio_ix_read_ = 0;
  *timestamp = in_timestamp_[0];

  
  
  int16_t status = 0;
  int16_t dtx_processed_samples = 0;
  status = ProcessFrameVADDTX(bitstream, bitstream_len_byte,
                              &dtx_processed_samples);
  if (status < 0) {
    *timestamp = 0;
    *bitstream_len_byte = 0;
    *encoding_type = kNoEncoding;
  } else {
    if (dtx_processed_samples > 0) {
      
      

      
      
      in_audio_ix_read_ = dtx_processed_samples;
      
      
      uint16_t samp_freq_hz;
      EncoderSampFreq(samp_freq_hz);
      if (samp_freq_hz == 8000) {
        *encoding_type = kPassiveDTXNB;
      } else if (samp_freq_hz == 16000) {
        *encoding_type = kPassiveDTXWB;
      } else if (samp_freq_hz == 32000) {
        *encoding_type = kPassiveDTXSWB;
      } else if (samp_freq_hz == 48000) {
        *encoding_type = kPassiveDTXFB;
      } else {
        status = -1;
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                     "EncodeSafe: Wrong sampling frequency for DTX.");
      }

      
      if ((*bitstream_len_byte == 0) &&
          (sent_cn_previous_ ||
          ((in_audio_ix_write_ - in_audio_ix_read_) <= 0))) {
        
        *bitstream_len_byte = 1;
        *encoding_type = kNoEncoding;
      }
      sent_cn_previous_ = true;
    } else {
      
      

      sent_cn_previous_ = false;
      if (my_basic_coding_block_smpl == 0) {
        
        status = InternalEncode(bitstream, bitstream_len_byte);
        if (status < 0) {
          
          
          WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding,
                       unique_id_, "EncodeSafe: error in internal_encode");
          *bitstream_len_byte = 0;
          *encoding_type = kNoEncoding;
        }
      } else {
        
        
        int16_t tmp_bitstream_len_byte;

        
        *bitstream_len_byte = 0;
        bool done = false;
        while (!done) {
          status = InternalEncode(&bitstream[*bitstream_len_byte],
                                  &tmp_bitstream_len_byte);
          *bitstream_len_byte += tmp_bitstream_len_byte;

          
          if ((status < 0) || (*bitstream_len_byte > MAX_PAYLOAD_SIZE_BYTE)) {
            
            
            
            *bitstream_len_byte = 0;
            *encoding_type = kNoEncoding;
            
            status = -1;
            WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding,
                         unique_id_, "EncodeSafe: error in InternalEncode");
            
            break;
          }

          
          
          
          done = in_audio_ix_read_ >= frame_len_smpl_;
        }
      }
      if (status >= 0) {
        *encoding_type = (vad_label_[0] == 1) ? kActiveNormalEncoded :
            kPassiveNormalEncoded;
        
        if ((*bitstream_len_byte == 0) &&
            ((in_audio_ix_write_ - in_audio_ix_read_) <= 0)) {
          
          *bitstream_len_byte = 1;
          *encoding_type = kNoEncoding;
        }
      }
    }
  }

  
  
  uint16_t samp_freq_hz;
  EncoderSampFreq(samp_freq_hz);
  int16_t num_10ms_blocks = static_cast<int16_t>(
      (in_audio_ix_read_ / num_channels_ * 100) / samp_freq_hz);
  if (in_timestamp_ix_write_ > num_10ms_blocks) {
    memmove(in_timestamp_, in_timestamp_ + num_10ms_blocks,
            (in_timestamp_ix_write_ - num_10ms_blocks) * sizeof(int32_t));
  }
  in_timestamp_ix_write_ -= num_10ms_blocks;

  
  
  if (in_audio_ix_read_ < in_audio_ix_write_) {
    memmove(in_audio_, &in_audio_[in_audio_ix_read_],
            (in_audio_ix_write_ - in_audio_ix_read_) * sizeof(int16_t));
  }
  in_audio_ix_write_ -= in_audio_ix_read_;
  in_audio_ix_read_ = 0;
  last_encoded_timestamp_ = *timestamp;
  return (status < 0) ? (-1) : (*bitstream_len_byte);
}

int16_t ACMGenericCodec::Decode(uint8_t* bitstream,
                                int16_t bitstream_len_byte,
                                int16_t* audio,
                                int16_t* audio_samples,
                                int8_t* speech_type) {
  WriteLockScoped wl(codec_wrapper_lock_);
  return DecodeSafe(bitstream, bitstream_len_byte, audio, audio_samples,
                    speech_type);
}

bool ACMGenericCodec::EncoderInitialized() {
  ReadLockScoped rl(codec_wrapper_lock_);
  return encoder_initialized_;
}

bool ACMGenericCodec::DecoderInitialized() {
  ReadLockScoped rl(codec_wrapper_lock_);
  return decoder_initialized_;
}

int32_t ACMGenericCodec::RegisterInNetEq(ACMNetEQ* neteq,
                                         const CodecInst& codec_inst) {
  WebRtcNetEQ_CodecDef codec_def;
  WriteLockScoped wl(codec_wrapper_lock_);

  if (CodecDef(codec_def, codec_inst) < 0) {
    
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "RegisterInNetEq: error, failed to register");
    registered_in_neteq_ = false;
    return -1;
  } else {
    if (neteq->AddCodec(&codec_def, is_master_) < 0) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                   "RegisterInNetEq: error, failed to add codec");
      registered_in_neteq_ = false;
      return -1;
    }
    
    registered_in_neteq_ = true;
    return 0;
  }
}

int16_t ACMGenericCodec::EncoderParams(WebRtcACMCodecParams* enc_params) {
  ReadLockScoped rl(codec_wrapper_lock_);
  return EncoderParamsSafe(enc_params);
}

int16_t ACMGenericCodec::EncoderParamsSafe(WebRtcACMCodecParams* enc_params) {
  
  if (encoder_initialized_) {
    int32_t current_rate;
    memcpy(enc_params, &encoder_params_, sizeof(WebRtcACMCodecParams));
    current_rate = enc_params->codec_inst.rate;
    CurrentRate(current_rate);
    enc_params->codec_inst.rate = current_rate;
    return 0;
  } else {
    enc_params->codec_inst.plname[0] = '\0';
    enc_params->codec_inst.pltype = -1;
    enc_params->codec_inst.pacsize = 0;
    enc_params->codec_inst.rate = 0;
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "EncoderParamsSafe: error, encoder not initialized");
    return -1;
  }
}

bool ACMGenericCodec::DecoderParams(WebRtcACMCodecParams* dec_params,
                                    const uint8_t payload_type) {
  ReadLockScoped rl(codec_wrapper_lock_);
  return DecoderParamsSafe(dec_params, payload_type);
}

bool ACMGenericCodec::DecoderParamsSafe(WebRtcACMCodecParams* dec_params,
                                        const uint8_t payload_type) {
  
  if (decoder_initialized_) {
    if (payload_type == decoder_params_.codec_inst.pltype) {
      memcpy(dec_params, &decoder_params_, sizeof(WebRtcACMCodecParams));
      return true;
    }
  }

  dec_params->codec_inst.plname[0] = '\0';
  dec_params->codec_inst.pltype = -1;
  dec_params->codec_inst.pacsize = 0;
  dec_params->codec_inst.rate = 0;
  return false;
}

int16_t ACMGenericCodec::ResetEncoder() {
  WriteLockScoped lockCodec(codec_wrapper_lock_);
  ReadLockScoped lockNetEq(*neteq_decode_lock_);
  return ResetEncoderSafe();
}

int16_t ACMGenericCodec::ResetEncoderSafe() {
  if (!encoder_exist_ || !encoder_initialized_) {
    
    return 0;
  }

  in_audio_ix_write_ = 0;
  in_audio_ix_read_ = 0;
  in_timestamp_ix_write_ = 0;
  num_missed_samples_ = 0;
  is_audio_buff_fresh_ = true;
  memset(in_audio_, 0, AUDIO_BUFFER_SIZE_W16 * sizeof(int16_t));
  memset(in_timestamp_, 0, TIMESTAMP_BUFFER_SIZE_W32 * sizeof(int32_t));

  
  bool enable_vad = vad_enabled_;
  bool enable_dtx = dtx_enabled_;
  ACMVADMode mode = vad_mode_;

  
  if (InternalResetEncoder() < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "ResetEncoderSafe: error in reset encoder");
    return -1;
  }

  
  DisableDTX();
  DisableVAD();

  
  return SetVADSafe(enable_dtx, enable_vad, mode);
}

int16_t ACMGenericCodec::InternalResetEncoder() {
  
  return InternalInitEncoder(&encoder_params_);
}

int16_t ACMGenericCodec::InitEncoder(WebRtcACMCodecParams* codec_params,
                                     bool force_initialization) {
  WriteLockScoped lockCodec(codec_wrapper_lock_);
  ReadLockScoped lockNetEq(*neteq_decode_lock_);
  return InitEncoderSafe(codec_params, force_initialization);
}

int16_t ACMGenericCodec::InitEncoderSafe(WebRtcACMCodecParams* codec_params,
                                         bool force_initialization) {
  
  int mirrorID;
  int codec_number = ACMCodecDB::CodecNumber(&(codec_params->codec_inst),
                                             &mirrorID);
  if (codec_number < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "InitEncoderSafe: error, codec number negative");
    return -1;
  }
  
  if ((codec_id_ >= 0) && (codec_id_ != codec_number) &&
      (codec_id_ != mirrorID)) {
    
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "InitEncoderSafe: current codec is not the same as the one "
                 "given by codec_params");
    return -1;
  }

  if (!CanChangeEncodingParam(codec_params->codec_inst)) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "InitEncoderSafe: cannot change encoding parameters");
    return -1;
  }

  if (encoder_initialized_ && !force_initialization) {
    
    
    return 0;
  }
  int16_t status;
  if (!encoder_exist_) {
    
    encoder_initialized_ = false;
    status = CreateEncoder();
    if (status < 0) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                   "InitEncoderSafe: cannot create encoder");
      return -1;
    } else {
      encoder_exist_ = true;
    }
  }
  frame_len_smpl_ = (codec_params->codec_inst).pacsize;
  num_channels_ = codec_params->codec_inst.channels;
  status = InternalInitEncoder(codec_params);
  if (status < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "InitEncoderSafe: error in init encoder");
    encoder_initialized_ = false;
    return -1;
  } else {
    
    memcpy(&encoder_params_, codec_params, sizeof(WebRtcACMCodecParams));
    encoder_initialized_ = true;
    if (in_audio_ == NULL) {
      in_audio_ = new int16_t[AUDIO_BUFFER_SIZE_W16];
      if (in_audio_ == NULL) {
        return -1;
      }
      memset(in_audio_, 0, AUDIO_BUFFER_SIZE_W16 * sizeof(int16_t));
    }
    if (in_timestamp_ == NULL) {
      in_timestamp_ = new uint32_t[TIMESTAMP_BUFFER_SIZE_W32];
      if (in_timestamp_ == NULL) {
        return -1;
      }
      memset(in_timestamp_, 0, sizeof(uint32_t) * TIMESTAMP_BUFFER_SIZE_W32);
    }
    is_audio_buff_fresh_ = true;
  }
  status = SetVADSafe(codec_params->enable_dtx, codec_params->enable_vad,
                      codec_params->vad_mode);

  return status;
}



bool ACMGenericCodec::CanChangeEncodingParam(CodecInst& ) {
  return true;
}

int16_t ACMGenericCodec::InitDecoder(WebRtcACMCodecParams* codec_params,
                                     bool force_initialization) {
  WriteLockScoped lockCodc(codec_wrapper_lock_);
  WriteLockScoped lockNetEq(*neteq_decode_lock_);
  return InitDecoderSafe(codec_params, force_initialization);
}

int16_t ACMGenericCodec::InitDecoderSafe(WebRtcACMCodecParams* codec_params,
                                         bool force_initialization) {
  int mirror_id;
  
  int codec_number = ACMCodecDB::ReceiverCodecNumber(&codec_params->codec_inst,
                                                     &mirror_id);
  if (codec_number < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "InitDecoderSafe: error, invalid codec number");
    return -1;
  }
  
  if ((codec_id_ >= 0) && (codec_id_ != codec_number) &&
      (codec_id_ != mirror_id)) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "InitDecoderSafe: current codec is not the same as the one "
                 "given by codec_params");
    
    return -1;
  }

  if (decoder_initialized_ && !force_initialization) {
    
    
    return 0;
  }

  int16_t status;
  if (!decoder_exist_) {
    
    decoder_initialized_ = false;
    status = CreateDecoder();
    if (status < 0) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                   "InitDecoderSafe: cannot create decoder");
      return -1;
    } else {
      decoder_exist_ = true;
    }
  }

  status = InternalInitDecoder(codec_params);
  if (status < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "InitDecoderSafe: cannot init decoder");
    decoder_initialized_ = false;
    return -1;
  } else {
    
    SaveDecoderParamSafe(codec_params);
    decoder_initialized_ = true;
  }
  return 0;
}

int16_t ACMGenericCodec::ResetDecoder(int16_t payload_type) {
  WriteLockScoped lockCodec(codec_wrapper_lock_);
  WriteLockScoped lockNetEq(*neteq_decode_lock_);
  return ResetDecoderSafe(payload_type);
}

int16_t ACMGenericCodec::ResetDecoderSafe(int16_t payload_type) {
  WebRtcACMCodecParams decoder_params;
  if (!decoder_exist_ || !decoder_initialized_) {
    return 0;
  }
  
  
  
  DecoderParamsSafe(&decoder_params, static_cast<uint8_t>(payload_type));
  return InternalInitDecoder(&decoder_params);
}

void ACMGenericCodec::ResetNoMissedSamples() {
  WriteLockScoped cs(codec_wrapper_lock_);
  num_missed_samples_ = 0;
}

void ACMGenericCodec::IncreaseNoMissedSamples(const int16_t num_samples) {
  num_missed_samples_ += num_samples;
}


uint32_t ACMGenericCodec::NoMissedSamples() const {
  ReadLockScoped cs(codec_wrapper_lock_);
  return num_missed_samples_;
}

void ACMGenericCodec::DestructEncoder() {
  WriteLockScoped wl(codec_wrapper_lock_);

  
  if (ptr_vad_inst_ != NULL) {
    WebRtcVad_Free(ptr_vad_inst_);
    ptr_vad_inst_ = NULL;
  }
  vad_enabled_ = false;
  vad_mode_ = VADNormal;

  
  dtx_enabled_ = false;
  if (ptr_dtx_inst_ != NULL) {
    WebRtcCng_FreeEnc(ptr_dtx_inst_);
    ptr_dtx_inst_ = NULL;
  }
  num_lpc_params_ = kNewCNGNumPLCParams;

  DestructEncoderSafe();
}

void ACMGenericCodec::DestructDecoder() {
  WriteLockScoped wl(codec_wrapper_lock_);
  decoder_params_.codec_inst.pltype = -1;
  DestructDecoderSafe();
}

int16_t ACMGenericCodec::SetBitRate(const int32_t bitrate_bps) {
  WriteLockScoped wl(codec_wrapper_lock_);
  return SetBitRateSafe(bitrate_bps);
}

int16_t ACMGenericCodec::SetBitRateSafe(const int32_t bitrate_bps) {
  
  
  CodecInst codec_params;
  if (ACMCodecDB::Codec(codec_id_, &codec_params) < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "SetBitRateSafe: error in ACMCodecDB::Codec");
    return -1;
  }
  if (codec_params.rate != bitrate_bps) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "SetBitRateSafe: rate value is not acceptable");
    return -1;
  } else {
    return 0;
  }
}


int32_t ACMGenericCodec::GetEstimatedBandwidth() {
  WriteLockScoped wl(codec_wrapper_lock_);
  return GetEstimatedBandwidthSafe();
}

int32_t ACMGenericCodec::GetEstimatedBandwidthSafe() {
  
  return -1;
}

int32_t ACMGenericCodec::SetEstimatedBandwidth(int32_t estimated_bandwidth) {
  WriteLockScoped wl(codec_wrapper_lock_);
  return SetEstimatedBandwidthSafe(estimated_bandwidth);
}

int32_t ACMGenericCodec::SetEstimatedBandwidthSafe(
    int32_t ) {
  
  return -1;
}


int32_t ACMGenericCodec::GetRedPayload(uint8_t* red_payload,
                                       int16_t* payload_bytes) {
  WriteLockScoped wl(codec_wrapper_lock_);
  return GetRedPayloadSafe(red_payload, payload_bytes);
}

int32_t ACMGenericCodec::GetRedPayloadSafe(uint8_t* ,
                                           int16_t* ) {
  return -1;  
}

int16_t ACMGenericCodec::CreateEncoder() {
  int16_t status = 0;
  if (!encoder_exist_) {
    status = InternalCreateEncoder();
    
    encoder_initialized_ = false;
  }
  if (status < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "CreateEncoder: error in internal create encoder");
    encoder_exist_ = false;
  } else {
    encoder_exist_ = true;
  }
  return status;
}

int16_t ACMGenericCodec::CreateDecoder() {
  int16_t status = 0;
  if (!decoder_exist_) {
    status = InternalCreateDecoder();
    
    decoder_initialized_ = false;
  }
  if (status < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "CreateDecoder: error in internal create decoder");
    decoder_exist_ = false;
  } else {
    decoder_exist_ = true;
  }
  return status;
}

void ACMGenericCodec::DestructEncoderInst(void* ptr_inst) {
  if (ptr_inst != NULL) {
    WriteLockScoped lockCodec(codec_wrapper_lock_);
    ReadLockScoped lockNetEq(*neteq_decode_lock_);
    InternalDestructEncoderInst(ptr_inst);
  }
}


int16_t ACMGenericCodec::AudioBuffer(WebRtcACMAudioBuff& audio_buff) {
  ReadLockScoped cs(codec_wrapper_lock_);
  memcpy(audio_buff.in_audio, in_audio_,
         AUDIO_BUFFER_SIZE_W16 * sizeof(int16_t));
  audio_buff.in_audio_ix_read = in_audio_ix_read_;
  audio_buff.in_audio_ix_write = in_audio_ix_write_;
  memcpy(audio_buff.in_timestamp, in_timestamp_,
         TIMESTAMP_BUFFER_SIZE_W32 * sizeof(uint32_t));
  audio_buff.in_timestamp_ix_write = in_timestamp_ix_write_;
  audio_buff.last_timestamp = last_timestamp_;
  return 0;
}


int16_t ACMGenericCodec::SetAudioBuffer(WebRtcACMAudioBuff& audio_buff) {
  WriteLockScoped cs(codec_wrapper_lock_);
  memcpy(in_audio_, audio_buff.in_audio,
         AUDIO_BUFFER_SIZE_W16 * sizeof(int16_t));
  in_audio_ix_read_ = audio_buff.in_audio_ix_read;
  in_audio_ix_write_ = audio_buff.in_audio_ix_write;
  memcpy(in_timestamp_, audio_buff.in_timestamp,
         TIMESTAMP_BUFFER_SIZE_W32 * sizeof(uint32_t));
  in_timestamp_ix_write_ = audio_buff.in_timestamp_ix_write;
  last_timestamp_ = audio_buff.last_timestamp;
  is_audio_buff_fresh_ = false;
  return 0;
}

uint32_t ACMGenericCodec::LastEncodedTimestamp() const {
  ReadLockScoped cs(codec_wrapper_lock_);
  return last_encoded_timestamp_;
}

uint32_t ACMGenericCodec::EarliestTimestamp() const {
  ReadLockScoped cs(codec_wrapper_lock_);
  return in_timestamp_[0];
}

int16_t ACMGenericCodec::SetVAD(const bool enable_dtx,
                                const bool enable_vad,
                                const ACMVADMode mode) {
  WriteLockScoped cs(codec_wrapper_lock_);
  return SetVADSafe(enable_dtx, enable_vad, mode);
}

int16_t ACMGenericCodec::SetVADSafe(const bool enable_dtx,
                                    const bool enable_vad,
                                    const ACMVADMode mode) {
  if (enable_dtx) {
    
    if (!STR_CASE_CMP(encoder_params_.codec_inst.plname, "G729")
        && !has_internal_dtx_) {
      if (ACMGenericCodec::EnableDTX() < 0) {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                     "SetVADSafe: error in enable DTX");
        return -1;
      }
    } else {
      if (EnableDTX() < 0) {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                     "SetVADSafe: error in enable DTX");
        return -1;
      }
    }

    if (has_internal_dtx_) {
      
      
      
      vad_mode_ = mode;
      return (enable_vad) ? EnableVAD(mode) : DisableVAD();
    } else {
      
      
      if (EnableVAD(mode) < 0) {
        
        if (!vad_enabled_) {
          DisableDTX();
        }
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                     "SetVADSafe: error in enable VAD");
        return -1;
      }

      
      
      if (enable_vad == false) {
        return 1;
      } else {
        return 0;
      }
    }
  } else {
    
    if (!STR_CASE_CMP(encoder_params_.codec_inst.plname, "G729")
        && !has_internal_dtx_) {
      ACMGenericCodec::DisableDTX();
    } else {
      DisableDTX();
    }
    return (enable_vad) ? EnableVAD(mode) : DisableVAD();
  }
}

int16_t ACMGenericCodec::EnableDTX() {
  if (has_internal_dtx_) {
    
    
    return -1;
  }
  if (!dtx_enabled_) {
    if (WebRtcCng_CreateEnc(&ptr_dtx_inst_) < 0) {
      ptr_dtx_inst_ = NULL;
      return -1;
    }
    uint16_t freq_hz;
    EncoderSampFreq(freq_hz);
    if (WebRtcCng_InitEnc(ptr_dtx_inst_, freq_hz, kCngSidIntervalMsec,
                          num_lpc_params_) < 0) {
      
      WebRtcCng_FreeEnc(ptr_dtx_inst_);
      ptr_dtx_inst_ = NULL;
      return -1;
    }
    dtx_enabled_ = true;
  }
  return 0;
}

int16_t ACMGenericCodec::DisableDTX() {
  if (has_internal_dtx_) {
    
    
    return -1;
  }
  if (ptr_dtx_inst_ != NULL) {
    WebRtcCng_FreeEnc(ptr_dtx_inst_);
    ptr_dtx_inst_ = NULL;
  }
  dtx_enabled_ = false;
  return 0;
}

int16_t ACMGenericCodec::EnableVAD(ACMVADMode mode) {
  if ((mode < VADNormal) || (mode > VADVeryAggr)) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "EnableVAD: error in VAD mode range");
    return -1;
  }

  if (!vad_enabled_) {
    if (WebRtcVad_Create(&ptr_vad_inst_) < 0) {
      ptr_vad_inst_ = NULL;
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                   "EnableVAD: error in create VAD");
      return -1;
    }
    if (WebRtcVad_Init(ptr_vad_inst_) < 0) {
      WebRtcVad_Free(ptr_vad_inst_);
      ptr_vad_inst_ = NULL;
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                   "EnableVAD: error in init VAD");
      return -1;
    }
  }

  
  if (WebRtcVad_set_mode(ptr_vad_inst_, mode) < 0) {
    
    
    
    if (!vad_enabled_) {
      
      
      WebRtcVad_Free(ptr_vad_inst_);
      ptr_vad_inst_ = NULL;
    }
    WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceAudioCoding, unique_id_,
                 "EnableVAD: failed to set the VAD mode");
    return -1;
  }
  vad_mode_ = mode;
  vad_enabled_ = true;
  return 0;
}

int16_t ACMGenericCodec::DisableVAD() {
  if (ptr_vad_inst_ != NULL) {
    WebRtcVad_Free(ptr_vad_inst_);
    ptr_vad_inst_ = NULL;
  }
  vad_enabled_ = false;
  return 0;
}

int32_t ACMGenericCodec::ReplaceInternalDTX(const bool replace_internal_dtx) {
  WriteLockScoped cs(codec_wrapper_lock_);
  return ReplaceInternalDTXSafe(replace_internal_dtx);
}

int32_t ACMGenericCodec::ReplaceInternalDTXSafe(
    const bool ) {
  return -1;
}

int32_t ACMGenericCodec::IsInternalDTXReplaced(bool* internal_dtx_replaced) {
  WriteLockScoped cs(codec_wrapper_lock_);
  return IsInternalDTXReplacedSafe(internal_dtx_replaced);
}

int32_t ACMGenericCodec::IsInternalDTXReplacedSafe(
    bool* internal_dtx_replaced) {
  *internal_dtx_replaced = false;
  return 0;
}

int16_t ACMGenericCodec::ProcessFrameVADDTX(uint8_t* bitstream,
                                            int16_t* bitstream_len_byte,
                                            int16_t* samples_processed) {
  if (!vad_enabled_) {
    
    for (int n = 0; n < MAX_FRAME_SIZE_10MSEC; n++) {
      vad_label_[n] = 1;
    }
    *samples_processed = 0;
    return 0;
  }

  uint16_t freq_hz;
  EncoderSampFreq(freq_hz);

  
  int16_t samples_in_10ms = static_cast<int16_t>(freq_hz / 100);
  int32_t frame_len_ms = static_cast<int32_t>(frame_len_smpl_) * 1000 / freq_hz;
  int16_t status;

  
  int16_t audio[1440];

  
  
  int num_samples_to_process[2];
  if (frame_len_ms == 40) {
    
    num_samples_to_process[0] = num_samples_to_process[1] = 2 * samples_in_10ms;
  } else {
    
    
    num_samples_to_process[0] =
        (frame_len_ms > 30) ? 3 * samples_in_10ms : frame_len_smpl_;
    num_samples_to_process[1] = frame_len_smpl_ - num_samples_to_process[0];
  }

  int offset = 0;
  int loops = (num_samples_to_process[1] > 0) ? 2 : 1;
  for (int i = 0; i < loops; i++) {
    
    
    if (num_channels_ == 2) {
      for (int j = 0; j < num_samples_to_process[i]; j++) {
        audio[j] = (in_audio_[(offset + j) * 2] +
            in_audio_[(offset + j) * 2 + 1]) / 2;
      }
      offset = num_samples_to_process[0];
    } else {
      
      memcpy(audio, in_audio_, sizeof(int16_t) * num_samples_to_process[i]);
    }

    
    status = static_cast<int16_t>(WebRtcVad_Process(ptr_vad_inst_,
                                                    static_cast<int>(freq_hz),
                                                    audio,
                                                    num_samples_to_process[i]));
    vad_label_[i] = status;

    if (status < 0) {
      
      *samples_processed += num_samples_to_process[i];
      return -1;
    }

    
    
    
    
    *samples_processed = 0;
    if ((status == 0) && (i == 0) && dtx_enabled_ && !has_internal_dtx_) {
      int16_t bitstream_len;
      int num_10ms_frames = num_samples_to_process[i] / samples_in_10ms;
      *bitstream_len_byte = 0;
      for (int n = 0; n < num_10ms_frames; n++) {
        
        
        status = WebRtcCng_Encode(ptr_dtx_inst_, &audio[n * samples_in_10ms],
                                  samples_in_10ms, bitstream, &bitstream_len,
                                  !prev_frame_cng_);
        if (status < 0) {
          return -1;
        }

        
        prev_frame_cng_ = 1;

        *samples_processed += samples_in_10ms * num_channels_;

        
        *bitstream_len_byte += bitstream_len;
      }

      
      if (*samples_processed != num_samples_to_process[i] * num_channels_) {
        
        *samples_processed = 0;
      }
    } else {
      
      prev_frame_cng_ = 0;
    }

    if (*samples_processed > 0) {
      
      
      break;
    }
  }

  return status;
}

int16_t ACMGenericCodec::SamplesLeftToEncode() {
  ReadLockScoped rl(codec_wrapper_lock_);
  return (frame_len_smpl_ <= in_audio_ix_write_) ? 0 :
      (frame_len_smpl_ - in_audio_ix_write_);
}

void ACMGenericCodec::SetUniqueID(const uint32_t id) {
  unique_id_ = id;
}

bool ACMGenericCodec::IsAudioBufferFresh() const {
  ReadLockScoped rl(codec_wrapper_lock_);
  return is_audio_buff_fresh_;
}


int16_t ACMGenericCodec::EncoderSampFreq(uint16_t& samp_freq_hz) {
  int32_t f;
  f = ACMCodecDB::CodecFreq(codec_id_);
  if (f < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "EncoderSampFreq: codec frequency is negative");
    return -1;
  } else {
    samp_freq_hz = static_cast<uint16_t>(f);
    return 0;
  }
}

int32_t ACMGenericCodec::ConfigISACBandwidthEstimator(
    const uint8_t ,
    const uint16_t ,
    const bool ) {
  WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceAudioCoding, unique_id_,
               "The send-codec is not iSAC, failed to config iSAC bandwidth "
               "estimator.");
  return -1;
}

int32_t ACMGenericCodec::SetISACMaxRate(
    const uint32_t ) {
  WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceAudioCoding, unique_id_,
               "The send-codec is not iSAC, failed to set iSAC max rate.");
  return -1;
}

int32_t ACMGenericCodec::SetISACMaxPayloadSize(
    const uint16_t ) {
  WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceAudioCoding, unique_id_,
               "The send-codec is not iSAC, failed to set iSAC max "
               "payload-size.");
  return -1;
}

void ACMGenericCodec::SaveDecoderParam(
    const WebRtcACMCodecParams* codec_params) {
  WriteLockScoped wl(codec_wrapper_lock_);
  SaveDecoderParamSafe(codec_params);
}

void ACMGenericCodec::SaveDecoderParamSafe(
    const WebRtcACMCodecParams* codec_params) {
  memcpy(&decoder_params_, codec_params, sizeof(WebRtcACMCodecParams));
}

int16_t ACMGenericCodec::UpdateEncoderSampFreq(
    uint16_t ) {
  WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
               "It is asked for a change in smapling frequency while the "
               "current  send-codec supports only one sampling rate.");
  return -1;
}

void ACMGenericCodec::SetIsMaster(bool is_master) {
  WriteLockScoped wl(codec_wrapper_lock_);
  is_master_ = is_master;
}

int16_t ACMGenericCodec::REDPayloadISAC(const int32_t ,
                                        const int16_t ,
                                        uint8_t* ,
                                        int16_t* ) {
  WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
               "Error: REDPayloadISAC is an iSAC specific function");
  return -1;
}

}  

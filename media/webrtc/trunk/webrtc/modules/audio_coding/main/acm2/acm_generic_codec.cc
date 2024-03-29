









#include "webrtc/modules/audio_coding/main/acm2/acm_generic_codec.h"

#include <assert.h>
#include <string.h>

#include "webrtc/common_audio/vad/include/webrtc_vad.h"
#include "webrtc/modules/audio_coding/codecs/cng/include/webrtc_cng.h"
#include "webrtc/modules/audio_coding/main/acm2/acm_codec_database.h"
#include "webrtc/modules/audio_coding/main/acm2/acm_common_defs.h"
#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc {

namespace acm2 {


enum {
  kMaxPLCParamsCNG = WEBRTC_CNG_MAX_LPC_ORDER,
  kNewCNGNumLPCParams = 8
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
      encoder_initialized_(false),
      registered_in_neteq_(false),
      has_internal_dtx_(false),
      ptr_vad_inst_(NULL),
      vad_enabled_(false),
      vad_mode_(VADNormal),
      dtx_enabled_(false),
      ptr_dtx_inst_(NULL),
      num_lpc_params_(kNewCNGNumLPCParams),
      sent_cn_previous_(false),
      prev_frame_cng_(0),
      has_internal_fec_(false),
      codec_wrapper_lock_(*RWLockWrapper::CreateRWLock()),
      last_timestamp_(0xD87F3F9F),
      unique_id_(0) {
  
  for (int i = 0; i < MAX_FRAME_SIZE_10MSEC; i++) {
    vad_label_[i] = 0;
  }
  
  
  memset(&encoder_params_, 0, sizeof(WebRtcACMCodecParams));
  encoder_params_.codec_inst.pltype = -1;
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
  if (EncoderSampFreq(&plfreq_hz) < 0) {
    return -1;
  }

  
  if ((plfreq_hz / 100) != length_smpl) {
    
    return -1;
  }

  if (last_timestamp_ == timestamp) {
    
    if ((in_audio_ix_write_ >= length_smpl * audio_channel) &&
        (in_timestamp_ix_write_ > 0)) {
      in_audio_ix_write_ -= length_smpl * audio_channel;
      assert(in_timestamp_ix_write_ >= 0);

      in_timestamp_ix_write_--;
      assert(in_audio_ix_write_ >= 0);
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
    assert(in_timestamp_ix_write_ >= 0);

    in_timestamp_[in_timestamp_ix_write_] = timestamp;
    in_timestamp_ix_write_++;
    assert(in_timestamp_ix_write_ < TIMESTAMP_BUFFER_SIZE_W32);

    
    in_audio_ix_write_ = AUDIO_BUFFER_SIZE_W16;
    IncreaseNoMissedSamples(missed_samples);
    return -missed_samples;
  }

  
  memcpy(in_audio_ + in_audio_ix_write_, data,
         length_smpl * audio_channel * sizeof(int16_t));
  in_audio_ix_write_ += length_smpl * audio_channel;
  assert(in_timestamp_ix_write_ < TIMESTAMP_BUFFER_SIZE_W32);

  in_timestamp_[in_timestamp_ix_write_] = timestamp;
  in_timestamp_ix_write_++;
  assert(in_timestamp_ix_write_ < TIMESTAMP_BUFFER_SIZE_W32);
  return 0;
}

bool ACMGenericCodec::HasFrameToEncode() const {
  ReadLockScoped lockCodec(codec_wrapper_lock_);
  if (in_audio_ix_write_ < frame_len_smpl_ * num_channels_)
    return false;
  return true;
}

int ACMGenericCodec::SetFEC(bool enable_fec) {
  if (!HasInternalFEC() && enable_fec)
    return -1;
  return 0;
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
      EncoderSampFreq(&samp_freq_hz);
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
          done = in_audio_ix_read_ >= frame_len_smpl_ * num_channels_;
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
  EncoderSampFreq(&samp_freq_hz);
  int16_t num_10ms_blocks = static_cast<int16_t>(
      (in_audio_ix_read_ / num_channels_ * 100) / samp_freq_hz);
  if (in_timestamp_ix_write_ > num_10ms_blocks) {
    memmove(in_timestamp_, in_timestamp_ + num_10ms_blocks,
            (in_timestamp_ix_write_ - num_10ms_blocks) * sizeof(int32_t));
  }
  in_timestamp_ix_write_ -= num_10ms_blocks;
  assert(in_timestamp_ix_write_ >= 0);

  
  
  if (in_audio_ix_read_ < in_audio_ix_write_) {
    memmove(in_audio_, &in_audio_[in_audio_ix_read_],
            (in_audio_ix_write_ - in_audio_ix_read_) * sizeof(int16_t));
  }
  in_audio_ix_write_ -= in_audio_ix_read_;
  in_audio_ix_read_ = 0;
  return (status < 0) ? (-1) : (*bitstream_len_byte);
}

bool ACMGenericCodec::EncoderInitialized() {
  ReadLockScoped rl(codec_wrapper_lock_);
  return encoder_initialized_;
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
    CurrentRate(&current_rate);
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

int16_t ACMGenericCodec::ResetEncoder() {
  WriteLockScoped lockCodec(codec_wrapper_lock_);
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

  
  int status = SetVADSafe(&enable_dtx, &enable_vad, &mode);
  dtx_enabled_ = enable_dtx;
  vad_enabled_ = enable_vad;
  vad_mode_ = mode;
  return status;
}

int16_t ACMGenericCodec::InternalResetEncoder() {
  
  return InternalInitEncoder(&encoder_params_);
}

int16_t ACMGenericCodec::InitEncoder(WebRtcACMCodecParams* codec_params,
                                     bool force_initialization) {
  WriteLockScoped lockCodec(codec_wrapper_lock_);
  return InitEncoderSafe(codec_params, force_initialization);
}

int16_t ACMGenericCodec::InitEncoderSafe(WebRtcACMCodecParams* codec_params,
                                         bool force_initialization) {
  
  int mirrorID;
  int codec_number = ACMCodecDB::CodecNumber(codec_params->codec_inst,
                                             &mirrorID);
  assert(codec_number >= 0);

  
  if ((codec_id_ >= 0) && (codec_id_ != codec_number) &&
      (codec_id_ != mirrorID)) {
    
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "InitEncoderSafe: current codec is not the same as the one "
                 "given by codec_params");
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
  frame_len_smpl_ = codec_params->codec_inst.pacsize;
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
    }
    if (in_timestamp_ == NULL) {
      in_timestamp_ = new uint32_t[TIMESTAMP_BUFFER_SIZE_W32];
    }
  }

  
  memset(in_audio_, 0, sizeof(*in_audio_) * AUDIO_BUFFER_SIZE_W16);
  memset(in_timestamp_, 0, sizeof(*in_timestamp_) * TIMESTAMP_BUFFER_SIZE_W32);
  in_audio_ix_write_ = 0;
  in_audio_ix_read_ = 0;
  in_timestamp_ix_write_ = 0;

  return SetVADSafe(&codec_params->enable_dtx, &codec_params->enable_vad,
                    &codec_params->vad_mode);
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
  num_lpc_params_ = kNewCNGNumLPCParams;

  DestructEncoderSafe();
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

uint32_t ACMGenericCodec::EarliestTimestamp() const {
  ReadLockScoped cs(codec_wrapper_lock_);
  return in_timestamp_[0];
}

int16_t ACMGenericCodec::SetVAD(bool* enable_dtx,
                                bool* enable_vad,
                                ACMVADMode* mode) {
  WriteLockScoped cs(codec_wrapper_lock_);
  return SetVADSafe(enable_dtx, enable_vad, mode);
}

int16_t ACMGenericCodec::SetVADSafe(bool* enable_dtx,
                                    bool* enable_vad,
                                    ACMVADMode* mode) {
  if (!STR_CASE_CMP(encoder_params_.codec_inst.plname, "OPUS") ||
      encoder_params_.codec_inst.channels == 2 ) {
    
    
    DisableDTX();
    DisableVAD();
    *enable_dtx = false;
    *enable_vad = false;
    return 0;
  }

  if (*enable_dtx) {
    
    if (!STR_CASE_CMP(encoder_params_.codec_inst.plname, "G729")
        && !has_internal_dtx_) {
      if (ACMGenericCodec::EnableDTX() < 0) {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                     "SetVADSafe: error in enable DTX");
        *enable_dtx = false;
        *enable_vad = vad_enabled_;
        return -1;
      }
    } else {
      if (EnableDTX() < 0) {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                     "SetVADSafe: error in enable DTX");
        *enable_dtx = false;
        *enable_vad = vad_enabled_;
        return -1;
      }
    }

    
    
    
    
    if (!has_internal_dtx_) {
      
      *enable_vad = true;
    }
  } else {
    
    if (!STR_CASE_CMP(encoder_params_.codec_inst.plname, "G729")
        && !has_internal_dtx_) {
      ACMGenericCodec::DisableDTX();
      *enable_dtx = false;
    } else {
      DisableDTX();
      *enable_dtx = false;
    }
  }

  int16_t status = (*enable_vad) ? EnableVAD(*mode) : DisableVAD();
  if (status < 0) {
    
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
    "SetVADSafe: error in enable VAD");
    DisableDTX();
    *enable_dtx = false;
    *enable_vad = false;
  }
  return status;
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
    EncoderSampFreq(&freq_hz);
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
  EncoderSampFreq(&freq_hz);

  
  int16_t samples_in_10ms = static_cast<int16_t>(freq_hz / 100);
  int32_t frame_len_ms = static_cast<int32_t>(frame_len_smpl_) * 1000 / freq_hz;
  int16_t status = -1;

  
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


int16_t ACMGenericCodec::EncoderSampFreq(uint16_t* samp_freq_hz) {
  int32_t f;
  f = ACMCodecDB::CodecFreq(codec_id_);
  if (f < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "EncoderSampFreq: codec frequency is negative");
    return -1;
  } else {
    *samp_freq_hz = static_cast<uint16_t>(f);
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

int16_t ACMGenericCodec::UpdateEncoderSampFreq(
    uint16_t ) {
  WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
               "It is asked for a change in smapling frequency while the "
               "current  send-codec supports only one sampling rate.");
  return -1;
}

int16_t ACMGenericCodec::REDPayloadISAC(const int32_t ,
                                        const int16_t ,
                                        uint8_t* ,
                                        int16_t* ) {
  WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
               "Error: REDPayloadISAC is an iSAC specific function");
  return -1;
}

int ACMGenericCodec::SetOpusMaxPlaybackRate(int ) {
  WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceAudioCoding, unique_id_,
      "The send-codec is not Opus, failed to set maximum playback rate.");
  return -1;
}

}  

}  

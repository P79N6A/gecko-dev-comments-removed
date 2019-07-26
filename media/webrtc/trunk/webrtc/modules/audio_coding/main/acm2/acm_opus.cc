









#include "webrtc/modules/audio_coding/main/acm2/acm_opus.h"

#ifdef WEBRTC_CODEC_OPUS
#include "webrtc/modules/audio_coding/codecs/opus/interface/opus_interface.h"
#include "webrtc/modules/audio_coding/main/acm2/acm_codec_database.h"
#include "webrtc/modules/audio_coding/main/acm2/acm_common_defs.h"
#include "webrtc/system_wrappers/interface/trace.h"
#endif

namespace webrtc {

namespace acm2 {

#ifndef WEBRTC_CODEC_OPUS

ACMOpus::ACMOpus(int16_t )
    : encoder_inst_ptr_(NULL),
      sample_freq_(0),
      bitrate_(0),
      channels_(1) {
  return;
}

ACMOpus::~ACMOpus() {
  return;
}

int16_t ACMOpus::InternalEncode(uint8_t* ,
                                int16_t* ) {
  return -1;
}

int16_t ACMOpus::InternalInitEncoder(WebRtcACMCodecParams* ) {
  return -1;
}

ACMGenericCodec* ACMOpus::CreateInstance(void) {
  return NULL;
}

int16_t ACMOpus::InternalCreateEncoder() {
  return -1;
}

void ACMOpus::DestructEncoderSafe() {
  return;
}

void ACMOpus::InternalDestructEncoderInst(void* ) {
  return;
}

int16_t ACMOpus::SetBitRateSafe(const int32_t ) {
  return -1;
}

#else  

ACMOpus::ACMOpus(int16_t codec_id)
    : encoder_inst_ptr_(NULL),
      sample_freq_(32000),  
      bitrate_(20000),  
      channels_(1) {  
  codec_id_ = codec_id;
  
  has_internal_dtx_ = false;

  if (codec_id_ != ACMCodecDB::kOpus) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "Wrong codec id for Opus.");
    sample_freq_ = -1;
    bitrate_ = -1;
  }
  return;
}

ACMOpus::~ACMOpus() {
  if (encoder_inst_ptr_ != NULL) {
    WebRtcOpus_EncoderFree(encoder_inst_ptr_);
    encoder_inst_ptr_ = NULL;
  }
}

int16_t ACMOpus::InternalEncode(uint8_t* bitstream,
                                int16_t* bitstream_len_byte) {
  
  *bitstream_len_byte = WebRtcOpus_Encode(encoder_inst_ptr_,
                                          &in_audio_[in_audio_ix_read_],
                                          frame_len_smpl_,
                                          MAX_PAYLOAD_SIZE_BYTE, bitstream);
  
  if (*bitstream_len_byte < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "InternalEncode: Encode error for Opus");
    *bitstream_len_byte = 0;
    return -1;
  }

  
  
  in_audio_ix_read_ += frame_len_smpl_ * channels_;

  return *bitstream_len_byte;
}

int16_t ACMOpus::InternalInitEncoder(WebRtcACMCodecParams* codec_params) {
  int16_t ret;
  if (encoder_inst_ptr_ != NULL) {
    WebRtcOpus_EncoderFree(encoder_inst_ptr_);
    encoder_inst_ptr_ = NULL;
  }
  ret = WebRtcOpus_EncoderCreate(&encoder_inst_ptr_,
                                 codec_params->codec_inst.channels);
  
  channels_ = codec_params->codec_inst.channels;

  if (ret < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "Encoder creation failed for Opus");
    return ret;
  }
  ret = WebRtcOpus_SetBitRate(encoder_inst_ptr_,
                              codec_params->codec_inst.rate);
  if (ret < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "Setting initial bitrate failed for Opus");
    return ret;
  }

  
  bitrate_ = codec_params->codec_inst.rate;

  return 0;
}

ACMGenericCodec* ACMOpus::CreateInstance(void) {
  return NULL;
}

int16_t ACMOpus::InternalCreateEncoder() {
  
  return 0;
}

void ACMOpus::DestructEncoderSafe() {
  if (encoder_inst_ptr_) {
    WebRtcOpus_EncoderFree(encoder_inst_ptr_);
    encoder_inst_ptr_ = NULL;
  }
}

void ACMOpus::InternalDestructEncoderInst(void* ptr_inst) {
  if (ptr_inst != NULL) {
    WebRtcOpus_EncoderFree(static_cast<OpusEncInst*>(ptr_inst));
  }
  return;
}

int16_t ACMOpus::SetBitRateSafe(const int32_t rate) {
  if (rate < 6000 || rate > 510000) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "SetBitRateSafe: Invalid rate Opus");
    return -1;
  }

  bitrate_ = rate;

  
  if (WebRtcOpus_SetBitRate(encoder_inst_ptr_, bitrate_) >= 0) {
    encoder_params_.codec_inst.rate = bitrate_;
    return 0;
  }

  return -1;
}

#endif  

}  

}  

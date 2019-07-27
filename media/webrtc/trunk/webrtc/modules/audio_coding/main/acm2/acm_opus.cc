









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
      channels_(1),
      packet_loss_rate_(0) {
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

int16_t ACMOpus::SetBitRateSafe(const int32_t ) {
  return -1;
}

#else  

ACMOpus::ACMOpus(int16_t codec_id)
    : encoder_inst_ptr_(NULL),
      sample_freq_(32000),  
      bitrate_(20000),  
      channels_(1),  
      packet_loss_rate_(0) {  
  codec_id_ = codec_id;
  
  has_internal_dtx_ = false;

  has_internal_fec_ = true;

  if (codec_id_ != ACMCodecDB::kOpus) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                 "Wrong codec id for Opus.");
    sample_freq_ = 0xFFFF;
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

  
  
#if defined(WEBRTC_ANDROID) || defined(WEBRTC_IOS) || defined(WEBRTC_ARCH_ARM)
  
  
  const int kOpusComplexity5 = 5;
  WebRtcOpus_SetComplexity(encoder_inst_ptr_, kOpusComplexity5);
  if (ret < 0) {
     WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                  "Setting complexity failed for Opus");
     return ret;
   }
#endif

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

int ACMOpus::SetFEC(bool enable_fec) {
  
  if (enable_fec) {
    if (WebRtcOpus_EnableFec(encoder_inst_ptr_) == 0)
      return 0;
  } else {
    if (WebRtcOpus_DisableFec(encoder_inst_ptr_) == 0)
      return 0;
  }
  return -1;
}

int ACMOpus::SetPacketLossRate(int loss_rate) {
  
  
  
  
  
  
  const int kPacketLossRate20 = 20;
  const int kPacketLossRate10 = 10;
  const int kPacketLossRate5 = 5;
  const int kPacketLossRate1 = 1;
  const int kLossRate20Margin = 2;
  const int kLossRate10Margin = 1;
  const int kLossRate5Margin = 1;
  int opt_loss_rate;
  if (loss_rate >= kPacketLossRate20 + kLossRate20Margin *
      (kPacketLossRate20 - packet_loss_rate_ > 0 ? 1 : -1)) {
    opt_loss_rate = kPacketLossRate20;
  } else if (loss_rate >= kPacketLossRate10 + kLossRate10Margin *
      (kPacketLossRate10 - packet_loss_rate_ > 0 ? 1 : -1)) {
    opt_loss_rate = kPacketLossRate10;
  } else if (loss_rate >= kPacketLossRate5 + kLossRate5Margin *
      (kPacketLossRate5 - packet_loss_rate_ > 0 ? 1 : -1)) {
    opt_loss_rate = kPacketLossRate5;
  } else if (loss_rate >= kPacketLossRate1) {
    opt_loss_rate = kPacketLossRate1;
  } else {
    opt_loss_rate = 0;
  }

  if (packet_loss_rate_ == opt_loss_rate) {
    return 0;
  }

  
  if (WebRtcOpus_SetPacketLossRate(encoder_inst_ptr_, opt_loss_rate) == 0) {
    packet_loss_rate_ = opt_loss_rate;
    return 0;
  }

  return -1;
}

int ACMOpus::SetOpusMaxPlaybackRate(int frequency_hz) {
  
  return WebRtcOpus_SetMaxPlaybackRate(encoder_inst_ptr_, frequency_hz);
}

#endif  

}  

}  

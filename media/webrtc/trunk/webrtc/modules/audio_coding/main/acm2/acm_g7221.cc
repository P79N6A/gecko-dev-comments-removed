









#include "webrtc/modules/audio_coding/main/acm2/acm_g7221.h"

#ifdef WEBRTC_CODEC_G722_1


#include "webrtc/modules/audio_coding/main/codecs/g7221/interface/g7221_interface.h"
#include "webrtc/modules/audio_coding/main/acm2/acm_codec_database.h"
#include "webrtc/modules/audio_coding/main/acm2/acm_common_defs.h"
#include "webrtc/system_wrappers/interface/trace.h"



























































#endif

namespace webrtc {

#ifndef WEBRTC_CODEC_G722_1

ACMG722_1::ACMG722_1(int16_t )
    : operational_rate_(-1),
      encoder_inst_ptr_(NULL),
      encoder_inst_ptr_right_(NULL),
      encoder_inst16_ptr_(NULL),
      encoder_inst16_ptr_right_(NULL),
      encoder_inst24_ptr_(NULL),
      encoder_inst24_ptr_right_(NULL),
      encoder_inst32_ptr_(NULL),
      encoder_inst32_ptr_right_(NULL) {
  return;
}

ACMG722_1::~ACMG722_1() { return; }

int16_t ACMG722_1::InternalEncode(uint8_t* ,
                                  int16_t* ) {
  return -1;
}

int16_t ACMG722_1::InternalInitEncoder(
    WebRtcACMCodecParams* ) {
  return -1;
}

ACMGenericCodec* ACMG722_1::CreateInstance(void) { return NULL; }

int16_t ACMG722_1::InternalCreateEncoder() { return -1; }

void ACMG722_1::DestructEncoderSafe() { return; }

void ACMG722_1::InternalDestructEncoderInst(void* ) { return; }

#else  
ACMG722_1::ACMG722_1(int16_t codec_id)
    : encoder_inst_ptr_(NULL),
      encoder_inst_ptr_right_(NULL),
      encoder_inst16_ptr_(NULL),
      encoder_inst16_ptr_right_(NULL),
      encoder_inst24_ptr_(NULL),
      encoder_inst24_ptr_right_(NULL),
      encoder_inst32_ptr_(NULL),
      encoder_inst32_ptr_right_(NULL) {
  codec_id_ = codec_id;
  if (codec_id_ == ACMCodecDB::kG722_1_16) {
    operational_rate_ = 16000;
  } else if (codec_id_ == ACMCodecDB::kG722_1_24) {
    operational_rate_ = 24000;
  } else if (codec_id_ == ACMCodecDB::kG722_1_32) {
    operational_rate_ = 32000;
  } else {
    operational_rate_ = -1;
  }
  return;
}

ACMG722_1::~ACMG722_1() {
  if (encoder_inst_ptr_ != NULL) {
    delete encoder_inst_ptr_;
    encoder_inst_ptr_ = NULL;
  }
  if (encoder_inst_ptr_right_ != NULL) {
    delete encoder_inst_ptr_right_;
    encoder_inst_ptr_right_ = NULL;
  }

  switch (operational_rate_) {
    case 16000: {
      encoder_inst16_ptr_ = NULL;
      encoder_inst16_ptr_right_ = NULL;
      break;
    }
    case 24000: {
      encoder_inst24_ptr_ = NULL;
      encoder_inst24_ptr_right_ = NULL;
      break;
    }
    case 32000: {
      encoder_inst32_ptr_ = NULL;
      encoder_inst32_ptr_right_ = NULL;
      break;
    }
    default: {
      break;
    }
  }
  return;
}

int16_t ACMG722_1::InternalEncode(uint8_t* bitstream,
                                  int16_t* bitstream_len_byte) {
  int16_t left_channel[320];
  int16_t right_channel[320];
  int16_t len_in_bytes;
  int16_t out_bits[160];

  
  if (num_channels_ == 2) {
    for (int i = 0, j = 0; i < frame_len_smpl_ * 2; i += 2, j++) {
      left_channel[j] = in_audio_[in_audio_ix_read_ + i];
      right_channel[j] = in_audio_[in_audio_ix_read_ + i + 1];
    }
  } else {
    memcpy(left_channel, &in_audio_[in_audio_ix_read_], 320);
  }

  switch (operational_rate_) {
    case 16000: {
      len_in_bytes = WebRtcG7221_Encode16(encoder_inst16_ptr_, left_channel,
                                               320, &out_bits[0]);
      if (num_channels_ == 2) {
        len_in_bytes += WebRtcG7221_Encode16(encoder_inst16_ptr_right_,
                                             right_channel, 320,
                                             &out_bits[len_in_bytes / 2]);
      }
      break;
    }
    case 24000: {
      len_in_bytes = WebRtcG7221_Encode24(encoder_inst24_ptr_, left_channel,
                                          320, &out_bits[0]);
      if (num_channels_ == 2) {
        len_in_bytes += WebRtcG7221_Encode24(encoder_inst24_ptr_right_,
                                             right_channel, 320,
                                             &out_bits[len_in_bytes / 2]);
      }
      break;
    }
    case 32000: {
      len_in_bytes = WebRtcG7221_Encode32(encoder_inst32_ptr_, left_channel,
                                          320, &out_bits[0]);
      if (num_channels_ == 2) {
        len_in_bytes += WebRtcG7221_Encode32(encoder_inst32_ptr_right_,
                                             right_channel, 320,
                                             &out_bits[len_in_bytes / 2]);
      }
      break;
    }
    default: {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                   "InternalInitEncode: Wrong rate for G722_1.");
      return -1;
    }
  }
  memcpy(bitstream, out_bits, len_in_bytes);
  *bitstream_len_byte = len_in_bytes;

  
  
  in_audio_ix_read_ += 320 * num_channels_;
  return *bitstream_len_byte;
}

int16_t ACMG722_1::InternalInitEncoder(WebRtcACMCodecParams* codec_params) {
  int16_t ret;

  switch (operational_rate_) {
    case 16000: {
      ret = WebRtcG7221_EncoderInit16(encoder_inst16_ptr_right_);
      if (ret < 0) {
        return ret;
      }
      return WebRtcG7221_EncoderInit16(encoder_inst16_ptr_);
    }
    case 24000: {
      ret = WebRtcG7221_EncoderInit24(encoder_inst24_ptr_right_);
      if (ret < 0) {
        return ret;
      }
      return WebRtcG7221_EncoderInit24(encoder_inst24_ptr_);
    }
    case 32000: {
      ret = WebRtcG7221_EncoderInit32(encoder_inst32_ptr_right_);
      if (ret < 0) {
        return ret;
      }
      return WebRtcG7221_EncoderInit32(encoder_inst32_ptr_);
    }
    default: {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding,
                   unique_id_, "InternalInitEncoder: Wrong rate for G722_1.");
      return -1;
    }
  }
}

ACMGenericCodec* ACMG722_1::CreateInstance(void) { return NULL; }

int16_t ACMG722_1::InternalCreateEncoder() {
  if ((encoder_inst_ptr_ == NULL) || (encoder_inst_ptr_right_ == NULL)) {
    return -1;
  }
  switch (operational_rate_) {
    case 16000: {
      WebRtcG7221_CreateEnc16(&encoder_inst16_ptr_);
      WebRtcG7221_CreateEnc16(&encoder_inst16_ptr_right_);
      break;
    }
    case 24000: {
      WebRtcG7221_CreateEnc24(&encoder_inst24_ptr_);
      WebRtcG7221_CreateEnc24(&encoder_inst24_ptr_right_);
      break;
    }
    case 32000: {
      WebRtcG7221_CreateEnc32(&encoder_inst32_ptr_);
      WebRtcG7221_CreateEnc32(&encoder_inst32_ptr_right_);
      break;
    }
    default: {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, unique_id_,
                   "InternalCreateEncoder: Wrong rate for G722_1.");
      return -1;
    }
  }
  return 0;
}

void ACMG722_1::DestructEncoderSafe() {
  encoder_exist_ = false;
  encoder_initialized_ = false;
  if (encoder_inst_ptr_ != NULL) {
    delete encoder_inst_ptr_;
    encoder_inst_ptr_ = NULL;
  }
  if (encoder_inst_ptr_right_ != NULL) {
    delete encoder_inst_ptr_right_;
    encoder_inst_ptr_right_ = NULL;
  }
  encoder_inst16_ptr_ = NULL;
  encoder_inst24_ptr_ = NULL;
  encoder_inst32_ptr_ = NULL;
}

void ACMG722_1::InternalDestructEncoderInst(void* ptr_inst) {
  if (ptr_inst != NULL) {
    delete ptr_inst;
  }
  return;
}

#endif

}  











#include "webrtc/modules/audio_coding/main/acm2/acm_pcm16b.h"

#ifdef WEBRTC_CODEC_PCM16
#include "webrtc/modules/audio_coding/codecs/pcm16b/include/pcm16b.h"
#include "webrtc/modules/audio_coding/main/acm2/acm_codec_database.h"
#include "webrtc/modules/audio_coding/main/acm2/acm_common_defs.h"
#include "webrtc/system_wrappers/interface/trace.h"
#endif

namespace webrtc {

namespace acm2 {

#ifndef WEBRTC_CODEC_PCM16

ACMPCM16B::ACMPCM16B(int16_t ) { return; }

ACMPCM16B::~ACMPCM16B() { return; }

int16_t ACMPCM16B::InternalEncode(uint8_t* ,
                                  int16_t* ) {
  return -1;
}

int16_t ACMPCM16B::InternalInitEncoder(
    WebRtcACMCodecParams* ) {
  return -1;
}

ACMGenericCodec* ACMPCM16B::CreateInstance(void) { return NULL; }

int16_t ACMPCM16B::InternalCreateEncoder() { return -1; }

void ACMPCM16B::InternalDestructEncoderInst(void* ) { return; }

void ACMPCM16B::DestructEncoderSafe() { return; }

#else  
ACMPCM16B::ACMPCM16B(int16_t codec_id) {
  codec_id_ = codec_id;
  sampling_freq_hz_ = ACMCodecDB::CodecFreq(codec_id_);
}

ACMPCM16B::~ACMPCM16B() { return; }

int16_t ACMPCM16B::InternalEncode(uint8_t* bitstream,
                                  int16_t* bitstream_len_byte) {
  *bitstream_len_byte = WebRtcPcm16b_Encode(&in_audio_[in_audio_ix_read_],
                                            frame_len_smpl_ * num_channels_,
                                            bitstream);
  
  
  in_audio_ix_read_ += frame_len_smpl_ * num_channels_;
  return *bitstream_len_byte;
}

int16_t ACMPCM16B::InternalInitEncoder(
    WebRtcACMCodecParams* ) {
  
  return 0;
}

ACMGenericCodec* ACMPCM16B::CreateInstance(void) { return NULL; }

int16_t ACMPCM16B::InternalCreateEncoder() {
  
  return 0;
}

void ACMPCM16B::InternalDestructEncoderInst(void* ) {
  
  return;
}

void ACMPCM16B::DestructEncoderSafe() {
  
  encoder_exist_ = false;
  encoder_initialized_ = false;
  return;
}

#endif

}  

}  

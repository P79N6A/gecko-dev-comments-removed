









#include "webrtc/modules/audio_coding/main/source/acm_pcm16b.h"

#include "webrtc/modules/audio_coding/main/source/acm_codec_database.h"
#include "webrtc/modules/audio_coding/main/acm2/acm_common_defs.h"
#include "webrtc/modules/audio_coding/main/source/acm_neteq.h"
#include "webrtc/modules/audio_coding/neteq/interface/webrtc_neteq.h"
#include "webrtc/modules/audio_coding/neteq/interface/webrtc_neteq_help_macros.h"
#include "webrtc/system_wrappers/interface/trace.h"

#ifdef WEBRTC_CODEC_PCM16
#include "webrtc/modules/audio_coding/codecs/pcm16b/include/pcm16b.h"
#endif

namespace webrtc {

namespace acm1 {

#ifndef WEBRTC_CODEC_PCM16

ACMPCM16B::ACMPCM16B(int16_t ) {
  return;
}

ACMPCM16B::~ACMPCM16B() {
  return;
}

int16_t ACMPCM16B::InternalEncode(
    uint8_t* ,
    int16_t* ) {
  return -1;
}

int16_t ACMPCM16B::DecodeSafe(uint8_t* ,
                              int16_t ,
                              int16_t* ,
                              int16_t* ,
                              int8_t* ) {
  return -1;
}

int16_t ACMPCM16B::InternalInitEncoder(
    WebRtcACMCodecParams* ) {
  return -1;
}

int16_t ACMPCM16B::InternalInitDecoder(
    WebRtcACMCodecParams* ) {
  return -1;
}

int32_t ACMPCM16B::CodecDef(WebRtcNetEQ_CodecDef& ,
                            const CodecInst& ) {
  return -1;
}

ACMGenericCodec* ACMPCM16B::CreateInstance(void) {
  return NULL;
}

int16_t ACMPCM16B::InternalCreateEncoder() {
  return -1;
}

int16_t ACMPCM16B::InternalCreateDecoder() {
  return -1;
}

void ACMPCM16B::InternalDestructEncoderInst(void* ) {
  return;
}

void ACMPCM16B::DestructEncoderSafe() {
  return;
}

void ACMPCM16B::DestructDecoderSafe() {
  return;
}

void ACMPCM16B::SplitStereoPacket(uint8_t* ,
                                  int32_t* ) {
}

#else     
ACMPCM16B::ACMPCM16B(int16_t codec_id) {
  codec_id_ = codec_id;
  sampling_freq_hz_ = ACMCodecDB::CodecFreq(codec_id_);
}

ACMPCM16B::~ACMPCM16B() {
  return;
}

int16_t ACMPCM16B::InternalEncode(uint8_t* bitstream,
                                  int16_t* bitstream_len_byte) {
  *bitstream_len_byte = WebRtcPcm16b_Encode(&in_audio_[in_audio_ix_read_],
                                            frame_len_smpl_ * num_channels_,
                                            bitstream);
  
  
  in_audio_ix_read_ += frame_len_smpl_ * num_channels_;
  return *bitstream_len_byte;
}

int16_t ACMPCM16B::DecodeSafe(uint8_t* ,
                              int16_t ,
                              int16_t* ,
                              int16_t* ,
                              int8_t* ) {
  return 0;
}

int16_t ACMPCM16B::InternalInitEncoder(
    WebRtcACMCodecParams* ) {
  
  return 0;
}

int16_t ACMPCM16B::InternalInitDecoder(
    WebRtcACMCodecParams* ) {
  
  return 0;
}

int32_t ACMPCM16B::CodecDef(WebRtcNetEQ_CodecDef& codec_def,
                            const CodecInst& codec_inst) {
  
  
  if (codec_inst.channels == 1) {
    switch (sampling_freq_hz_) {
      case 8000: {
        SET_CODEC_PAR(codec_def, kDecoderPCM16B, codec_inst.pltype, NULL, 8000);
        SET_PCM16B_FUNCTIONS(codec_def);
        break;
      }
      case 16000: {
        SET_CODEC_PAR(codec_def, kDecoderPCM16Bwb, codec_inst.pltype, NULL,
                      16000);
        SET_PCM16B_WB_FUNCTIONS(codec_def);
        break;
      }
      case 32000: {
        SET_CODEC_PAR(codec_def, kDecoderPCM16Bswb32kHz, codec_inst.pltype,
                      NULL, 32000);
        SET_PCM16B_SWB32_FUNCTIONS(codec_def);
        break;
      }
      default: {
        return -1;
      }
    }
  } else {
    switch (sampling_freq_hz_) {
      case 8000: {
        SET_CODEC_PAR(codec_def, kDecoderPCM16B_2ch, codec_inst.pltype, NULL,
                      8000);
        SET_PCM16B_FUNCTIONS(codec_def);
        break;
      }
      case 16000: {
        SET_CODEC_PAR(codec_def, kDecoderPCM16Bwb_2ch, codec_inst.pltype,
                      NULL, 16000);
        SET_PCM16B_WB_FUNCTIONS(codec_def);
        break;
      }
      case 32000: {
        SET_CODEC_PAR(codec_def, kDecoderPCM16Bswb32kHz_2ch, codec_inst.pltype,
                      NULL, 32000);
        SET_PCM16B_SWB32_FUNCTIONS(codec_def);
        break;
      }
      default: {
        return -1;
      }
    }
  }
  return 0;
}

ACMGenericCodec* ACMPCM16B::CreateInstance(void) {
  return NULL;
}

int16_t ACMPCM16B::InternalCreateEncoder() {
  
  return 0;
}

int16_t ACMPCM16B::InternalCreateDecoder() {
  
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

void ACMPCM16B::DestructDecoderSafe() {
  
  decoder_exist_ = false;
  decoder_initialized_ = false;
  return;
}



void ACMPCM16B::SplitStereoPacket(uint8_t* payload, int32_t* payload_length) {
  uint8_t right_byte_msb;
  uint8_t right_byte_lsb;

  
  assert(payload != NULL);
  assert(*payload_length > 0);

  
  
  
  

  for (int i = 0; i < *payload_length / 2; i += 2) {
    right_byte_msb = payload[i + 2];
    right_byte_lsb = payload[i + 3];
    memmove(&payload[i + 2], &payload[i + 4], *payload_length - i - 4);
    payload[*payload_length - 2] = right_byte_msb;
    payload[*payload_length - 1] = right_byte_lsb;
  }
}
#endif

}  

}  











#include "webrtc/modules/audio_coding/main/source/acm_pcma.h"

#include "webrtc/modules/audio_coding/codecs/g711/include/g711_interface.h"
#include "webrtc/modules/audio_coding/main/acm2/acm_common_defs.h"
#include "webrtc/modules/audio_coding/main/source/acm_neteq.h"
#include "webrtc/modules/audio_coding/neteq/interface/webrtc_neteq.h"
#include "webrtc/modules/audio_coding/neteq/interface/webrtc_neteq_help_macros.h"
#include "webrtc/system_wrappers/interface/trace.h"



namespace webrtc {

namespace acm1 {

ACMPCMA::ACMPCMA(int16_t codec_id) {
  codec_id_ = codec_id;
}

ACMPCMA::~ACMPCMA() {
  return;
}

int16_t ACMPCMA::InternalEncode(uint8_t* bitstream,
                                int16_t* bitstream_len_byte) {
  *bitstream_len_byte = WebRtcG711_EncodeA(NULL, &in_audio_[in_audio_ix_read_],
                                           frame_len_smpl_ * num_channels_,
                                           (int16_t*) bitstream);
  
  
  in_audio_ix_read_ += frame_len_smpl_ * num_channels_;
  return *bitstream_len_byte;
}

int16_t ACMPCMA::DecodeSafe(uint8_t* ,
                            int16_t ,
                            int16_t* ,
                            int16_t* ,
                            int8_t* ) {
  return 0;
}

int16_t ACMPCMA::InternalInitEncoder(
    WebRtcACMCodecParams* ) {
  
  return 0;
}

int16_t ACMPCMA::InternalInitDecoder(
    WebRtcACMCodecParams* ) {
  
  return 0;
}

int32_t ACMPCMA::CodecDef(WebRtcNetEQ_CodecDef& codec_def,
                          const CodecInst& codec_inst) {
  
  
  
  if (codec_inst.channels == 1) {
    
    SET_CODEC_PAR(codec_def, kDecoderPCMa, codec_inst.pltype, NULL, 8000);
  } else {
    
    SET_CODEC_PAR(codec_def, kDecoderPCMa_2ch, codec_inst.pltype, NULL, 8000);
  }
  SET_PCMA_FUNCTIONS(codec_def);
  return 0;
}

ACMGenericCodec* ACMPCMA::CreateInstance(void) {
  return NULL;
}

int16_t ACMPCMA::InternalCreateEncoder() {
  
  return 0;
}

int16_t ACMPCMA::InternalCreateDecoder() {
  
  return 0;
}

void ACMPCMA::InternalDestructEncoderInst(void* ) {
  
  return;
}

void ACMPCMA::DestructEncoderSafe() {
  
  return;
}

void ACMPCMA::DestructDecoderSafe() {
  
  decoder_initialized_ = false;
  decoder_exist_ = false;
  return;
}



void ACMPCMA::SplitStereoPacket(uint8_t* payload, int32_t* payload_length) {
  uint8_t right_byte;

  
  assert(payload != NULL);
  assert(*payload_length > 0);

  
  
  
  
  for (int i = 0; i < *payload_length / 2; i++) {
    right_byte = payload[i + 1];
    memmove(&payload[i + 1], &payload[i + 2], *payload_length - i - 2);
    payload[*payload_length - 1] = right_byte;
  }
}

}  

}  

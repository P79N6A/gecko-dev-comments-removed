









#include "webrtc/modules/audio_coding/main/source/acm_pcma.h"

#include "webrtc/modules/audio_coding/codecs/g711/include/g711_interface.h"
#include "webrtc/modules/audio_coding/main/source/acm_common_defs.h"
#include "webrtc/modules/audio_coding/main/source/acm_neteq.h"
#include "webrtc/modules/audio_coding/neteq/interface/webrtc_neteq.h"
#include "webrtc/modules/audio_coding/neteq/interface/webrtc_neteq_help_macros.h"
#include "webrtc/system_wrappers/interface/trace.h"



namespace webrtc {

ACMPCMA::ACMPCMA(WebRtc_Word16 codec_id) {
  codec_id_ = codec_id;
}

ACMPCMA::~ACMPCMA() {
  return;
}

WebRtc_Word16 ACMPCMA::InternalEncode(WebRtc_UWord8* bitstream,
                                      WebRtc_Word16* bitstream_len_byte) {
  *bitstream_len_byte = WebRtcG711_EncodeA(NULL, &in_audio_[in_audio_ix_read_],
                                           frame_len_smpl_ * num_channels_,
                                           (WebRtc_Word16*) bitstream);
  
  
  in_audio_ix_read_ += frame_len_smpl_ * num_channels_;
  return *bitstream_len_byte;
}

WebRtc_Word16 ACMPCMA::DecodeSafe(WebRtc_UWord8* ,
                                  WebRtc_Word16 ,
                                  WebRtc_Word16* ,
                                  WebRtc_Word16* ,
                                  WebRtc_Word8* ) {
  return 0;
}

WebRtc_Word16 ACMPCMA::InternalInitEncoder(
    WebRtcACMCodecParams* ) {
  
  return 0;
}

WebRtc_Word16 ACMPCMA::InternalInitDecoder(
    WebRtcACMCodecParams* ) {
  
  return 0;
}

WebRtc_Word32 ACMPCMA::CodecDef(WebRtcNetEQ_CodecDef& codec_def,
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

WebRtc_Word16 ACMPCMA::InternalCreateEncoder() {
  
  return 0;
}

WebRtc_Word16 ACMPCMA::InternalCreateDecoder() {
  
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

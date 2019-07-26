









#include "webrtc/modules/audio_coding/main/source/acm_pcmu.h"

#include "webrtc/modules/audio_coding/codecs/g711/include/g711_interface.h"
#include "webrtc/modules/audio_coding/main/source/acm_common_defs.h"
#include "webrtc/modules/audio_coding/main/source/acm_neteq.h"
#include "webrtc/modules/audio_coding/neteq/interface/webrtc_neteq.h"
#include "webrtc/modules/audio_coding/neteq/interface/webrtc_neteq_help_macros.h"
#include "webrtc/system_wrappers/interface/trace.h"



namespace webrtc {

ACMPCMU::ACMPCMU(WebRtc_Word16 codec_id) {
  codec_id_ = codec_id;
}

ACMPCMU::~ACMPCMU() {
  return;
}

WebRtc_Word16 ACMPCMU::InternalEncode(WebRtc_UWord8* bitstream,
                                      WebRtc_Word16* bitstream_len_byte) {
  *bitstream_len_byte = WebRtcG711_EncodeU(NULL, &in_audio_[in_audio_ix_read_],
                                           frame_len_smpl_ * num_channels_,
                                           (WebRtc_Word16*)bitstream);
  
  
  in_audio_ix_read_ += frame_len_smpl_ * num_channels_;
  return *bitstream_len_byte;
}

WebRtc_Word16 ACMPCMU::DecodeSafe(WebRtc_UWord8* ,
                                  WebRtc_Word16 ,
                                  WebRtc_Word16* ,
                                  WebRtc_Word16* ,
                                  WebRtc_Word8* ) {
  return 0;
}

WebRtc_Word16 ACMPCMU::InternalInitEncoder(
    WebRtcACMCodecParams* ) {
  
  return 0;
}

WebRtc_Word16 ACMPCMU::InternalInitDecoder(
    WebRtcACMCodecParams* ) {
  
  return 0;
}

WebRtc_Word32 ACMPCMU::CodecDef(WebRtcNetEQ_CodecDef& codec_def,
                                const CodecInst& codec_inst) {
  
  
  
  if (codec_inst.channels == 1) {
    
    SET_CODEC_PAR(codec_def, kDecoderPCMu, codec_inst.pltype, NULL, 8000);
  } else {
    
    SET_CODEC_PAR(codec_def, kDecoderPCMu_2ch, codec_inst.pltype, NULL, 8000);
  }
  SET_PCMU_FUNCTIONS(codec_def);
  return 0;
}

ACMGenericCodec* ACMPCMU::CreateInstance(void) {
  return NULL;
}

WebRtc_Word16 ACMPCMU::InternalCreateEncoder() {
  
  return 0;
}

WebRtc_Word16 ACMPCMU::InternalCreateDecoder() {
  
  return 0;
}

void ACMPCMU::InternalDestructEncoderInst(void* ) {
  
  return;
}

void ACMPCMU::DestructEncoderSafe() {
  
  encoder_exist_ = false;
  encoder_initialized_ = false;
  return;
}

void ACMPCMU::DestructDecoderSafe() {
  
  decoder_initialized_ = false;
  decoder_exist_ = false;
  return;
}



void ACMPCMU::SplitStereoPacket(uint8_t* payload, int32_t* payload_length) {
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

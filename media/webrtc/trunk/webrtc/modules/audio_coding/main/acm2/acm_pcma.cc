









#include "webrtc/modules/audio_coding/main/acm2/acm_pcma.h"

#include "webrtc/modules/audio_coding/codecs/g711/include/g711_interface.h"
#include "webrtc/modules/audio_coding/main/acm2/acm_common_defs.h"
#include "webrtc/system_wrappers/interface/trace.h"



namespace webrtc {

namespace acm2 {

ACMPCMA::ACMPCMA(int16_t codec_id) { codec_id_ = codec_id; }

ACMPCMA::~ACMPCMA() { return; }

int16_t ACMPCMA::InternalEncode(uint8_t* bitstream,
                                int16_t* bitstream_len_byte) {
  *bitstream_len_byte = WebRtcG711_EncodeA(
      NULL, &in_audio_[in_audio_ix_read_], frame_len_smpl_ * num_channels_,
      reinterpret_cast<int16_t*>(bitstream));
  
  
  in_audio_ix_read_ += frame_len_smpl_ * num_channels_;
  return *bitstream_len_byte;
}

int16_t ACMPCMA::InternalInitEncoder(WebRtcACMCodecParams* ) {
  
  return 0;
}

ACMGenericCodec* ACMPCMA::CreateInstance(void) { return NULL; }

int16_t ACMPCMA::InternalCreateEncoder() {
  
  return 0;
}

void ACMPCMA::InternalDestructEncoderInst(void* ) {
  
  return;
}

void ACMPCMA::DestructEncoderSafe() {
  
  return;
}

}  

}  











#include "webrtc/modules/audio_coding/main/acm2/acm_pcmu.h"

#include "webrtc/modules/audio_coding/codecs/g711/include/g711_interface.h"
#include "webrtc/modules/audio_coding/main/acm2/acm_common_defs.h"
#include "webrtc/system_wrappers/interface/trace.h"



namespace webrtc {

namespace acm2 {

ACMPCMU::ACMPCMU(int16_t codec_id) { codec_id_ = codec_id; }

ACMPCMU::~ACMPCMU() {}

int16_t ACMPCMU::InternalEncode(uint8_t* bitstream,
                                int16_t* bitstream_len_byte) {
  *bitstream_len_byte = WebRtcG711_EncodeU(
      NULL, &in_audio_[in_audio_ix_read_], frame_len_smpl_ * num_channels_,
      reinterpret_cast<int16_t*>(bitstream));

  
  
  in_audio_ix_read_ += frame_len_smpl_ * num_channels_;
  return *bitstream_len_byte;
}

int16_t ACMPCMU::InternalInitEncoder(WebRtcACMCodecParams* ) {
  
  return 0;
}

ACMGenericCodec* ACMPCMU::CreateInstance(void) { return NULL; }

int16_t ACMPCMU::InternalCreateEncoder() {
  
  return 0;
}

void ACMPCMU::InternalDestructEncoderInst(void* ) {
  
}

void ACMPCMU::DestructEncoderSafe() {
  
  encoder_exist_ = false;
  encoder_initialized_ = false;
}

}  

}  

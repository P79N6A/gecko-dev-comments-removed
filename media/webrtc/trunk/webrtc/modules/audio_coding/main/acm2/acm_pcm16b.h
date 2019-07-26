









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_ACM_PCM16B_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_ACM_PCM16B_H_

#include "webrtc/modules/audio_coding/main/acm2/acm_generic_codec.h"

namespace webrtc {

namespace acm2 {

class ACMPCM16B : public ACMGenericCodec {
 public:
  explicit ACMPCM16B(int16_t codec_id);
  ~ACMPCM16B();

  
  ACMGenericCodec* CreateInstance(void);

  int16_t InternalEncode(uint8_t* bitstream, int16_t* bitstream_len_byte);

  int16_t InternalInitEncoder(WebRtcACMCodecParams* codec_params);

 protected:
  void DestructEncoderSafe();

  int16_t InternalCreateEncoder();

  void InternalDestructEncoderInst(void* ptr_inst);

  int32_t sampling_freq_hz_;
};

}  

}  

#endif  

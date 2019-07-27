









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_ACM_PCMU_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_ACM_PCMU_H_

#include "webrtc/modules/audio_coding/main/acm2/acm_generic_codec.h"

namespace webrtc {

namespace acm2 {

class ACMPCMU : public ACMGenericCodec {
 public:
  explicit ACMPCMU(int16_t codec_id);
  ~ACMPCMU();

  
  ACMGenericCodec* CreateInstance(void);

  int16_t InternalEncode(uint8_t* bitstream,
                         int16_t* bitstream_len_byte) OVERRIDE
      EXCLUSIVE_LOCKS_REQUIRED(codec_wrapper_lock_);

  int16_t InternalInitEncoder(WebRtcACMCodecParams* codec_params);

 protected:
  void DestructEncoderSafe() OVERRIDE
      EXCLUSIVE_LOCKS_REQUIRED(codec_wrapper_lock_);

  int16_t InternalCreateEncoder();
};

}  

}  

#endif  

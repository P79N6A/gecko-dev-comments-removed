









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_ACM_DTMF_PLAYOUT_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_ACM_DTMF_PLAYOUT_H_

#include "webrtc/modules/audio_coding/main/acm2/acm_generic_codec.h"

namespace webrtc {

namespace acm2 {

class ACMDTMFPlayout : public ACMGenericCodec {
 public:
  explicit ACMDTMFPlayout(int16_t codec_id);
  ~ACMDTMFPlayout();

  
  ACMGenericCodec* CreateInstance(void);

  int16_t InternalEncode(uint8_t* bitstream, int16_t* bitstream_len_byte);

  int16_t InternalInitEncoder(WebRtcACMCodecParams* codec_params);

 protected:
  void DestructEncoderSafe();

  int16_t InternalCreateEncoder();
};

}  

}  

#endif  

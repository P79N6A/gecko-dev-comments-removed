









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_ACM_CNG_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_ACM_CNG_H_

#include "webrtc/modules/audio_coding/main/acm2/acm_generic_codec.h"


struct WebRtcCngEncInst;
struct WebRtcCngDecInst;

namespace webrtc {

namespace acm2 {

class ACMCNG: public ACMGenericCodec {
 public:
  explicit ACMCNG(int16_t codec_id);
  ~ACMCNG();

  
  ACMGenericCodec* CreateInstance(void);

  int16_t InternalEncode(uint8_t* bitstream,
                         int16_t* bitstream_len_byte);

  int16_t InternalInitEncoder(WebRtcACMCodecParams *codec_params);

 protected:
  void DestructEncoderSafe();

  int16_t InternalCreateEncoder();

  void InternalDestructEncoderInst(void* ptr_inst);

  int16_t EnableDTX() {
    return -1;
  }

  int16_t DisableDTX() {
    return -1;
  }

  WebRtcCngEncInst* encoder_inst_ptr_;
  uint16_t samp_freq_hz_;
};

}  

}  

#endif  

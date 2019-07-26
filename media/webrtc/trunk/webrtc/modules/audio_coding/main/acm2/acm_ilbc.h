









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_ACM_ILBC_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_ACM_ILBC_H_

#include "webrtc/modules/audio_coding/main/acm2/acm_generic_codec.h"


struct iLBC_encinst_t_;
struct iLBC_decinst_t_;

namespace webrtc {

namespace acm2 {

class ACMILBC : public ACMGenericCodec {
 public:
  explicit ACMILBC(int16_t codec_id);
  ~ACMILBC();

  
  ACMGenericCodec* CreateInstance(void);

  int16_t InternalEncode(uint8_t* bitstream, int16_t* bitstream_len_byte);

  int16_t InternalInitEncoder(WebRtcACMCodecParams* codec_params);

 protected:
  int16_t SetBitRateSafe(const int32_t rate);

  void DestructEncoderSafe();

  int16_t InternalCreateEncoder();

  void InternalDestructEncoderInst(void* ptr_inst);

  iLBC_encinst_t_* encoder_inst_ptr_;
};

}  

}  

#endif  

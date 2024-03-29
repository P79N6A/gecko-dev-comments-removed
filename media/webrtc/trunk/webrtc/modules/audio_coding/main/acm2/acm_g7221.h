









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_ACM_G7221_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_ACM_G7221_H_

#include "webrtc/modules/audio_coding/main/acm2/acm_generic_codec.h"


struct G722_1_16_encinst_t_;
struct G722_1_16_decinst_t_;
struct G722_1_24_encinst_t_;
struct G722_1_24_decinst_t_;
struct G722_1_32_encinst_t_;
struct G722_1_32_decinst_t_;
struct G722_1_Inst_t_;

namespace webrtc {

namespace acm2 {

class ACMG722_1 : public ACMGenericCodec {
 public:
  explicit ACMG722_1(int16_t codec_id);
  ~ACMG722_1();

  
  ACMGenericCodec* CreateInstance(void);

  int16_t InternalEncode(uint8_t* bitstream, int16_t* bitstream_len_byte);

  int16_t InternalInitEncoder(WebRtcACMCodecParams* codec_params);

 protected:
  void DestructEncoderSafe();

  int16_t InternalCreateEncoder();

  int32_t operational_rate_;

  G722_1_Inst_t_* encoder_inst_ptr_;
  G722_1_Inst_t_* encoder_inst_ptr_right_;  

  
  G722_1_16_encinst_t_* encoder_inst16_ptr_;
  G722_1_16_encinst_t_* encoder_inst16_ptr_right_;
  G722_1_24_encinst_t_* encoder_inst24_ptr_;
  G722_1_24_encinst_t_* encoder_inst24_ptr_right_;
  G722_1_32_encinst_t_* encoder_inst32_ptr_;
  G722_1_32_encinst_t_* encoder_inst32_ptr_right_;
};

}  

}  

#endif  

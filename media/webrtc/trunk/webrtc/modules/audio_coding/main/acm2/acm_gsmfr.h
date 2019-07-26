









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_ACM_GSMFR_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_ACM_GSMFR_H_

#include "webrtc/modules/audio_coding/main/acm2/acm_generic_codec.h"


struct GSMFR_encinst_t_;
struct GSMFR_decinst_t_;

namespace webrtc {

namespace acm2 {

class ACMGSMFR : public ACMGenericCodec {
 public:
  explicit ACMGSMFR(int16_t codec_id);
  ~ACMGSMFR();

  
  ACMGenericCodec* CreateInstance(void);

  int16_t InternalEncode(uint8_t* bitstream, int16_t* bitstream_len_byte);

  int16_t InternalInitEncoder(WebRtcACMCodecParams* codec_params);

 protected:
  void DestructEncoderSafe();

  int16_t InternalCreateEncoder();

  void InternalDestructEncoderInst(void* ptr_inst);

  int16_t EnableDTX();

  int16_t DisableDTX();

  GSMFR_encinst_t_* encoder_inst_ptr_;
};

}  

}  

#endif  

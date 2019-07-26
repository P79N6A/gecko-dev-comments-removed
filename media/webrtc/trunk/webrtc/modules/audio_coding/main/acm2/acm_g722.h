









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_ACM_G722_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_ACM_G722_H_

#include "webrtc/modules/audio_coding/main/acm2/acm_generic_codec.h"

typedef struct WebRtcG722EncInst G722EncInst;
typedef struct WebRtcG722DecInst G722DecInst;

namespace webrtc {

namespace acm2 {


struct ACMG722EncStr;
struct ACMG722DecStr;

class ACMG722 : public ACMGenericCodec {
 public:
  explicit ACMG722(int16_t codec_id);
  ~ACMG722();

  
  ACMGenericCodec* CreateInstance(void);

  int16_t InternalEncode(uint8_t* bitstream, int16_t* bitstream_len_byte);

  int16_t InternalInitEncoder(WebRtcACMCodecParams* codec_params);

 protected:
  int32_t Add10MsDataSafe(const uint32_t timestamp,
                          const int16_t* data,
                          const uint16_t length_smpl,
                          const uint8_t audio_channel);

  void DestructEncoderSafe();

  int16_t InternalCreateEncoder();

  void InternalDestructEncoderInst(void* ptr_inst);

  ACMG722EncStr* ptr_enc_str_;

  G722EncInst* encoder_inst_ptr_;
  G722EncInst* encoder_inst_ptr_right_;  
};

}  

}  

#endif  

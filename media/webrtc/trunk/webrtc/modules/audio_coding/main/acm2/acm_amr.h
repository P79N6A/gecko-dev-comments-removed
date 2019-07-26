









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_ACM_AMR_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_ACM_AMR_H_

#include "webrtc/modules/audio_coding/main/acm2/acm_generic_codec.h"


struct AMR_encinst_t_;
struct AMR_decinst_t_;

namespace webrtc {

enum ACMAMRPackingFormat;

namespace acm2 {

class ACMAMR : public ACMGenericCodec {
 public:
  explicit ACMAMR(int16_t codec_id);
  ~ACMAMR();

  
  ACMGenericCodec* CreateInstance(void);

  int16_t InternalEncode(uint8_t* bitstream, int16_t* bitstream_len_byte);

  int16_t InternalInitEncoder(WebRtcACMCodecParams* codec_params);

  int16_t SetAMREncoderPackingFormat(const ACMAMRPackingFormat packing_format);

  ACMAMRPackingFormat AMREncoderPackingFormat() const;

  int16_t SetAMRDecoderPackingFormat(const ACMAMRPackingFormat packing_format);

  ACMAMRPackingFormat AMRDecoderPackingFormat() const;

 protected:
  void DestructEncoderSafe();

  int16_t InternalCreateEncoder();

  void InternalDestructEncoderInst(void* ptr_inst);

  int16_t SetBitRateSafe(const int32_t rate);

  int16_t EnableDTX();

  int16_t DisableDTX();

  AMR_encinst_t_* encoder_inst_ptr_;
  int16_t encoding_mode_;
  int16_t encoding_rate_;
  ACMAMRPackingFormat encoder_packing_format_;
};

}  

}  

#endif  

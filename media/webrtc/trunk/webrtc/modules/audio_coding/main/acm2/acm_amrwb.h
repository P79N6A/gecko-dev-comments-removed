









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_ACM_AMRWB_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_ACM_AMRWB_H_

#include "webrtc/modules/audio_coding/main/acm2/acm_generic_codec.h"


struct AMRWB_encinst_t_;
struct AMRWB_decinst_t_;

namespace webrtc {

namespace acm2 {

class ACMAMRwb : public ACMGenericCodec {
 public:
  explicit ACMAMRwb(int16_t codec_id);
  ~ACMAMRwb();

  
  ACMGenericCodec* CreateInstance(void);

  int16_t InternalEncode(uint8_t* bitstream, int16_t* bitstream_len_byte);

  int16_t InternalInitEncoder(WebRtcACMCodecParams* codec_params);

  int16_t SetAMRwbEncoderPackingFormat(
      const ACMAMRPackingFormat packing_format);

  ACMAMRPackingFormat AMRwbEncoderPackingFormat() const;

  int16_t SetAMRwbDecoderPackingFormat(
      const ACMAMRPackingFormat packing_format);

  ACMAMRPackingFormat AMRwbDecoderPackingFormat() const;

 protected:
  void DestructEncoderSafe();

  int16_t InternalCreateEncoder();

  void InternalDestructEncoderInst(void* ptr_inst);

  int16_t SetBitRateSafe(const int32_t rate);

  int16_t EnableDTX();

  int16_t DisableDTX();

  AMRWB_encinst_t_* encoder_inst_ptr_;

  int16_t encoding_mode_;
  int16_t encoding_rate_;
  ACMAMRPackingFormat encoder_packing_format_;
};

}  

}  

#endif  











#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_ILBC_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_ILBC_H_

#include "webrtc/modules/audio_coding/main/source/acm_generic_codec.h"


struct iLBC_encinst_t_;
struct iLBC_decinst_t_;

namespace webrtc {

class ACMILBC : public ACMGenericCodec {
 public:
  explicit ACMILBC(int16_t codec_id);
  ~ACMILBC();

  
  ACMGenericCodec* CreateInstance(void);

  int16_t InternalEncode(uint8_t* bitstream,
                         int16_t* bitstream_len_byte);

  int16_t InternalInitEncoder(WebRtcACMCodecParams *codec_params);

  int16_t InternalInitDecoder(WebRtcACMCodecParams *codec_params);

 protected:
  int16_t DecodeSafe(uint8_t* bitstream,
                     int16_t bitstream_len_byte,
                     int16_t* audio,
                     int16_t* audio_samples,
                     int8_t* speech_type);

  int32_t CodecDef(WebRtcNetEQ_CodecDef& codec_def,
                   const CodecInst& codec_inst);

  int16_t SetBitRateSafe(const int32_t rate);

  void DestructEncoderSafe();

  void DestructDecoderSafe();

  int16_t InternalCreateEncoder();

  int16_t InternalCreateDecoder();

  void InternalDestructEncoderInst(void* ptr_inst);

  iLBC_encinst_t_* encoder_inst_ptr_;
  iLBC_decinst_t_* decoder_inst_ptr_;
};

}  

#endif  











#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_G7221C_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_G7221C_H_

#include "webrtc/modules/audio_coding/main/source/acm_generic_codec.h"


struct G722_1C_24_encinst_t_;
struct G722_1C_24_decinst_t_;
struct G722_1C_32_encinst_t_;
struct G722_1C_32_decinst_t_;
struct G722_1C_48_encinst_t_;
struct G722_1C_48_decinst_t_;
struct G722_1_Inst_t_;

namespace webrtc {

class ACMG722_1C : public ACMGenericCodec {
 public:
  explicit ACMG722_1C(WebRtc_Word16 codec_id);
  ~ACMG722_1C();

  
  ACMGenericCodec* CreateInstance(void);

  WebRtc_Word16 InternalEncode(
      WebRtc_UWord8* bitstream,
      WebRtc_Word16* bitstream_len_byte);

  WebRtc_Word16 InternalInitEncoder(
      WebRtcACMCodecParams *codec_params);

  WebRtc_Word16 InternalInitDecoder(
      WebRtcACMCodecParams *codec_params);

 protected:
  WebRtc_Word16 DecodeSafe(
      WebRtc_UWord8* bitstream,
      WebRtc_Word16 bitstream_len_byte,
      WebRtc_Word16* audio,
      WebRtc_Word16* audio_samples,
      WebRtc_Word8* speech_type);

  WebRtc_Word32 CodecDef(
      WebRtcNetEQ_CodecDef& codec_def,
      const CodecInst& codec_inst);

  void DestructEncoderSafe();

  void DestructDecoderSafe();

  WebRtc_Word16 InternalCreateEncoder();

  WebRtc_Word16 InternalCreateDecoder();

  void InternalDestructEncoderInst(
      void* ptr_inst);

  WebRtc_Word32 operational_rate_;

  G722_1_Inst_t_* encoder_inst_ptr_;
  G722_1_Inst_t_* encoder_inst_ptr_right_;  
  G722_1_Inst_t_* decoder_inst_ptr_;

  
  G722_1C_24_encinst_t_* encoder_inst24_ptr_;
  G722_1C_24_encinst_t_* encoder_inst24_ptr_right_;
  G722_1C_32_encinst_t_* encoder_inst32_ptr_;
  G722_1C_32_encinst_t_* encoder_inst32_ptr_right_;
  G722_1C_48_encinst_t_* encoder_inst48_ptr_;
  G722_1C_48_encinst_t_* encoder_inst48_ptr_right_;

  
  G722_1C_24_decinst_t_* decoder_inst24_ptr_;
  G722_1C_32_decinst_t_* decoder_inst32_ptr_;
  G722_1C_48_decinst_t_* decoder_inst48_ptr_;
};

}  

#endif  

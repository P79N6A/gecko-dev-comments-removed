









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_G7291_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_G7291_H_

#include "webrtc/modules/audio_coding/main/source/acm_generic_codec.h"


struct G729_1_inst_t_;
struct G729_1_inst_t_;

namespace webrtc {

class ACMG729_1 : public ACMGenericCodec {
 public:
  explicit ACMG729_1(WebRtc_Word16 codec_id);
  ~ACMG729_1();

  
  ACMGenericCodec* CreateInstance(void);

  WebRtc_Word16 InternalEncode(WebRtc_UWord8* bitstream,
                               WebRtc_Word16* bitstream_len_byte);

  WebRtc_Word16 InternalInitEncoder(WebRtcACMCodecParams *codec_params);

  WebRtc_Word16 InternalInitDecoder(WebRtcACMCodecParams *codec_params);

 protected:
  WebRtc_Word16 DecodeSafe(WebRtc_UWord8* bitstream,
                           WebRtc_Word16 bitstream_len_byte,
                           WebRtc_Word16* audio,
                           WebRtc_Word16* audio_samples,
                           WebRtc_Word8* speech_type);

  WebRtc_Word32 CodecDef(WebRtcNetEQ_CodecDef& codec_def,
                         const CodecInst& codec_inst);

  void DestructEncoderSafe();

  void DestructDecoderSafe();

  WebRtc_Word16 InternalCreateEncoder();

  WebRtc_Word16 InternalCreateDecoder();

  void InternalDestructEncoderInst(void* ptr_inst);

  WebRtc_Word16 SetBitRateSafe(const WebRtc_Word32 rate);

  G729_1_inst_t_* encoder_inst_ptr_;
  G729_1_inst_t_* decoder_inst_ptr_;

  WebRtc_UWord16 my_rate_;
  WebRtc_Word16 flag_8khz_;
  WebRtc_Word16 flag_g729_mode_;
};

}  

#endif  

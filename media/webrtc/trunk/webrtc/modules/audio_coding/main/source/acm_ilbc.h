









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_ILBC_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_ILBC_H_

#include "webrtc/modules/audio_coding/main/source/acm_generic_codec.h"


struct iLBC_encinst_t_;
struct iLBC_decinst_t_;

namespace webrtc {

class ACMILBC : public ACMGenericCodec {
 public:
  explicit ACMILBC(WebRtc_Word16 codec_id);
  ~ACMILBC();

  
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

  WebRtc_Word16 SetBitRateSafe(const WebRtc_Word32 rate);

  void DestructEncoderSafe();

  void DestructDecoderSafe();

  WebRtc_Word16 InternalCreateEncoder();

  WebRtc_Word16 InternalCreateDecoder();

  void InternalDestructEncoderInst(void* ptr_inst);

  iLBC_encinst_t_* encoder_inst_ptr_;
  iLBC_decinst_t_* decoder_inst_ptr_;
};

}  

#endif  

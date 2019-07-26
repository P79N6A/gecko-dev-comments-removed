









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_CELT_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_CELT_H_

#include "acm_generic_codec.h"


struct CELT_encinst_t_;
struct CELT_decinst_t_;

namespace webrtc {

class ACMCELT : public ACMGenericCodec {
 public:
  ACMCELT(int16_t codecID);
  ~ACMCELT();

  ACMGenericCodec* CreateInstance(void);

  int16_t InternalEncode(uint8_t* bitstream, int16_t* bitStreamLenByte);

  int16_t InternalInitEncoder(WebRtcACMCodecParams *codecParams);

  int16_t InternalInitDecoder(WebRtcACMCodecParams *codecParams);

 protected:

  WebRtc_Word16 DecodeSafe(
      uint8_t* ,
      int16_t ,
      int16_t* ,
      int16_t* ,
      
      
      WebRtc_Word8* );

  int32_t CodecDef(WebRtcNetEQ_CodecDef& codecDef, const CodecInst& codecInst);

  void DestructEncoderSafe();

  void DestructDecoderSafe();

  int16_t InternalCreateEncoder();

  int16_t InternalCreateDecoder();

  void InternalDestructEncoderInst(void* ptrInst);

  bool IsTrueStereoCodec();

  int16_t SetBitRateSafe(const int32_t rate);

  void SplitStereoPacket(uint8_t* payload, int32_t* payload_length);

  CELT_encinst_t_* enc_inst_ptr_;
  CELT_decinst_t_* dec_inst_ptr_;
  uint16_t sampling_freq_;
  int32_t bitrate_;
  uint16_t channels_;
  uint16_t dec_channels_;
};

} 

#endif  











#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_CELT_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_CELT_H_

#include "webrtc/modules/audio_coding/main/source/acm_generic_codec.h"


struct CELT_encinst_t_;
struct CELT_decinst_t_;

namespace webrtc {

namespace acm1 {

class ACMCELT : public ACMGenericCodec {
 public:
  explicit ACMCELT(int16_t codec_id);
  virtual ~ACMCELT();

  virtual ACMGenericCodec* CreateInstance(void) OVERRIDE;

  virtual int16_t InternalEncode(uint8_t* bitstream,
                                 int16_t* bitstream_len_byte) OVERRIDE;

  virtual int16_t InternalInitEncoder(
      WebRtcACMCodecParams* codec_params) OVERRIDE;

  virtual int16_t InternalInitDecoder(
      WebRtcACMCodecParams* codec_params) OVERRIDE;

 protected:
  virtual int16_t DecodeSafe(uint8_t* ,
                             int16_t ,
                             int16_t* ,
                             int16_t* ,
                             int8_t* ) OVERRIDE;

  virtual int32_t CodecDef(WebRtcNetEQ_CodecDef& codec_def,
                           const CodecInst& codec_inst) OVERRIDE;

  virtual void DestructEncoderSafe() OVERRIDE;

  virtual void DestructDecoderSafe() OVERRIDE;

  virtual int16_t InternalCreateEncoder() OVERRIDE;

  virtual int16_t InternalCreateDecoder() OVERRIDE;

  virtual void InternalDestructEncoderInst(void* ptr_inst) OVERRIDE;

  virtual bool IsTrueStereoCodec() OVERRIDE;

  virtual int16_t SetBitRateSafe(const int32_t rate) OVERRIDE;

  virtual void SplitStereoPacket(uint8_t* payload,
                                 int32_t* payload_length) OVERRIDE;

  CELT_encinst_t_* enc_inst_ptr_;
  CELT_decinst_t_* dec_inst_ptr_;
  uint16_t sampling_freq_;
  int32_t bitrate_;
  uint16_t channels_;
  uint16_t dec_channels_;
};

}  

}  

#endif  

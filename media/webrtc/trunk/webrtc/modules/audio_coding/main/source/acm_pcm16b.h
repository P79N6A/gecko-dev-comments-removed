









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_PCM16B_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_PCM16B_H_

#include "webrtc/modules/audio_coding/main/source/acm_generic_codec.h"

namespace webrtc {

class ACMPCM16B : public ACMGenericCodec {
 public:
  explicit ACMPCM16B(int16_t codec_id);
  ~ACMPCM16B();

  
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

  void DestructEncoderSafe();

  void DestructDecoderSafe();

  int16_t InternalCreateEncoder();

  int16_t InternalCreateDecoder();

  void InternalDestructEncoderInst(void* ptr_inst);

  void SplitStereoPacket(uint8_t* payload, int32_t* payload_length);

  int32_t sampling_freq_hz_;
};

}  

#endif  

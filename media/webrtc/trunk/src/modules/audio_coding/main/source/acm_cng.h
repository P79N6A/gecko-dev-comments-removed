









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_CNG_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_CNG_H_

#include "acm_generic_codec.h"


struct WebRtcCngEncInst;
struct WebRtcCngDecInst;

namespace webrtc {

class ACMCNG: public ACMGenericCodec {
 public:
  ACMCNG(WebRtc_Word16 codecID);
  ~ACMCNG();
  
  ACMGenericCodec* CreateInstance(void);

  WebRtc_Word16 InternalEncode(WebRtc_UWord8* bitstream,
                               WebRtc_Word16* bitStreamLenByte);

  WebRtc_Word16 InternalInitEncoder(WebRtcACMCodecParams *codecParams);

  WebRtc_Word16 InternalInitDecoder(WebRtcACMCodecParams *codecParams);

protected:
  WebRtc_Word16 DecodeSafe(WebRtc_UWord8* bitStream,
                           WebRtc_Word16 bitStreamLenByte,
                           WebRtc_Word16* audio, WebRtc_Word16* audioSamples,
                           WebRtc_Word8* speechType);

  WebRtc_Word32 CodecDef(WebRtcNetEQ_CodecDef& codecDef,
                         const CodecInst& codecInst);

  void DestructEncoderSafe();

  void DestructDecoderSafe();

  WebRtc_Word16 InternalCreateEncoder();

  WebRtc_Word16 InternalCreateDecoder();

  void InternalDestructEncoderInst(void* ptrInst);

  WebRtc_Word16 EnableDTX() {
    return -1;
  }

  WebRtc_Word16 DisableDTX() {
    return -1;
  }

  WebRtcCngEncInst* _encoderInstPtr;
  WebRtcCngDecInst* _decoderInstPtr;
  WebRtc_Word16 _sampFreqHz;
};

} 

#endif  

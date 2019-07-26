









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_AMR_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_AMR_H_

#include "acm_generic_codec.h"


struct AMR_encinst_t_;
struct AMR_decinst_t_;

namespace webrtc {

enum ACMAMRPackingFormat;

class ACMAMR: public ACMGenericCodec {
 public:
  ACMAMR(WebRtc_Word16 codecID);
  ~ACMAMR();
  
  ACMGenericCodec* CreateInstance(void);

  WebRtc_Word16 InternalEncode(WebRtc_UWord8* bitstream,
                               WebRtc_Word16* bitStreamLenByte);

  WebRtc_Word16 InternalInitEncoder(WebRtcACMCodecParams *codecParams);

  WebRtc_Word16 InternalInitDecoder(WebRtcACMCodecParams *codecParams);

  WebRtc_Word16 SetAMREncoderPackingFormat(
      const ACMAMRPackingFormat packingFormat);

  ACMAMRPackingFormat AMREncoderPackingFormat() const;

  WebRtc_Word16 SetAMRDecoderPackingFormat(
      const ACMAMRPackingFormat packingFormat);

  ACMAMRPackingFormat AMRDecoderPackingFormat() const;

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

  WebRtc_Word16 SetBitRateSafe(const WebRtc_Word32 rate);

  WebRtc_Word16 EnableDTX();

  WebRtc_Word16 DisableDTX();

  AMR_encinst_t_* _encoderInstPtr;
  AMR_decinst_t_* _decoderInstPtr;
  WebRtc_Word16 _encodingMode;
  WebRtc_Word16 _encodingRate;
  ACMAMRPackingFormat _encoderPackingFormat;
  ACMAMRPackingFormat _decoderPackingFormat;
};

} 

#endif  

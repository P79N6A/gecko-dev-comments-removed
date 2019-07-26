









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_AMRWB_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_AMRWB_H_

#include "acm_generic_codec.h"


struct AMRWB_encinst_t_;
struct AMRWB_decinst_t_;

namespace webrtc {

enum ACMAMRPackingFormat;

class ACMAMRwb: public ACMGenericCodec {
 public:
  ACMAMRwb(WebRtc_Word16 codecID);
  ~ACMAMRwb();
  
  ACMGenericCodec* CreateInstance(void);

  WebRtc_Word16 InternalEncode(WebRtc_UWord8* bitstream,
                               WebRtc_Word16* bitStreamLenByte);

  WebRtc_Word16 InternalInitEncoder(WebRtcACMCodecParams* codecParams);

  WebRtc_Word16 InternalInitDecoder(WebRtcACMCodecParams* codecParams);

  WebRtc_Word16 SetAMRwbEncoderPackingFormat(
      const ACMAMRPackingFormat packingFormat);

  ACMAMRPackingFormat AMRwbEncoderPackingFormat() const;

  WebRtc_Word16 SetAMRwbDecoderPackingFormat(
      const ACMAMRPackingFormat packingFormat);

  ACMAMRPackingFormat AMRwbDecoderPackingFormat() const;

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

  AMRWB_encinst_t_* _encoderInstPtr;
  AMRWB_decinst_t_* _decoderInstPtr;

  WebRtc_Word16 _encodingMode;
  WebRtc_Word16 _encodingRate;
  ACMAMRPackingFormat _encoderPackingFormat;
  ACMAMRPackingFormat _decoderPackingFormat;
};

} 

#endif  

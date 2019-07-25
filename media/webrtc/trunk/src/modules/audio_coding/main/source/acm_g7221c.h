









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_G722_1C_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_G722_1C_H_

#include "acm_generic_codec.h"


struct G722_1C_24_encinst_t_;
struct G722_1C_24_decinst_t_;
struct G722_1C_32_encinst_t_;
struct G722_1C_32_decinst_t_;
struct G722_1C_48_encinst_t_;
struct G722_1C_48_decinst_t_;
struct G722_1_Inst_t_;

namespace webrtc {

class ACMG722_1C : public ACMGenericCodec
{
public:
    ACMG722_1C(WebRtc_Word16 codecID);
    ~ACMG722_1C();
    
    ACMGenericCodec* CreateInstance(void);

    WebRtc_Word16 InternalEncode(
        WebRtc_UWord8* bitstream,
        WebRtc_Word16* bitStreamLenByte);

    WebRtc_Word16 InternalInitEncoder(
        WebRtcACMCodecParams *codecParams);

    WebRtc_Word16 InternalInitDecoder(
        WebRtcACMCodecParams *codecParams);

protected:
    WebRtc_Word16 DecodeSafe(
        WebRtc_UWord8* bitStream,
        WebRtc_Word16  bitStreamLenByte,
        WebRtc_Word16* audio,
        WebRtc_Word16* audioSamples,
        WebRtc_Word8*  speechType);

    WebRtc_Word32 CodecDef(
        WebRtcNetEQ_CodecDef& codecDef,
        const CodecInst&      codecInst);

    void DestructEncoderSafe();

    void DestructDecoderSafe();

    WebRtc_Word16 InternalCreateEncoder();

    WebRtc_Word16 InternalCreateDecoder();

    void InternalDestructEncoderInst(
        void* ptrInst);

    WebRtc_Word16 UnregisterFromNetEqSafe(
        ACMNetEQ*     netEq,
        WebRtc_Word16 payloadType);

    WebRtc_Word32    _operationalRate;

    G722_1_Inst_t_*  _encoderInstPtr;
    G722_1_Inst_t_*  _encoderInstPtrRight; 
    G722_1_Inst_t_*  _decoderInstPtr;

    
    G722_1C_24_encinst_t_* _encoderInst24Ptr;
    G722_1C_24_encinst_t_* _encoderInst24PtrR;
    G722_1C_32_encinst_t_* _encoderInst32Ptr;
    G722_1C_32_encinst_t_* _encoderInst32PtrR;
    G722_1C_48_encinst_t_* _encoderInst48Ptr;
    G722_1C_48_encinst_t_* _encoderInst48PtrR;

    
    G722_1C_24_decinst_t_* _decoderInst24Ptr;
    G722_1C_32_decinst_t_* _decoderInst32Ptr;
    G722_1C_48_decinst_t_* _decoderInst48Ptr;
};

} 

#endif  

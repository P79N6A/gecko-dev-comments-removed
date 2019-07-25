









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_OPUS_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_OPUS_H_

#include "acm_generic_codec.h"


struct OPUS_inst_t_;
struct OPUS_inst_t_;

namespace webrtc
{

class ACMOPUS: public ACMGenericCodec
{
public:
    ACMOPUS(WebRtc_Word16 codecID);
    ~ACMOPUS();
    
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
        const CodecInst& codecInst);

    void DestructEncoderSafe();

    void DestructDecoderSafe();

    WebRtc_Word16 InternalCreateEncoder();

    WebRtc_Word16 InternalCreateDecoder();

    void InternalDestructEncoderInst(
        void* ptrInst);

    WebRtc_Word16 UnregisterFromNetEqSafe(
        ACMNetEQ* netEq,
        WebRtc_Word16   payloadType);

    WebRtc_Word16 SetBitRateSafe(
        const WebRtc_Word32 rate);

    OPUS_inst_t_* _encoderInstPtr;
    OPUS_inst_t_* _decoderInstPtr;

    WebRtc_UWord16    _mySampFreq;
    WebRtc_UWord16    _myRate;
    WebRtc_Word16     _opusMode;
    WebRtc_Word16     _flagVBR;

};

} 

#endif  

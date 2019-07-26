









#include "acm_red.h"
#include "acm_neteq.h"
#include "acm_common_defs.h"
#include "trace.h"
#include "webrtc_neteq.h"
#include "webrtc_neteq_help_macros.h"

namespace webrtc
{

ACMRED::ACMRED(WebRtc_Word16 codecID)
{
    _codecID = codecID;
}


ACMRED::~ACMRED()
{
    return;
}


WebRtc_Word16
ACMRED::InternalEncode(
    WebRtc_UWord8* ,
    WebRtc_Word16* )
{
    
    
    return 0;
}


WebRtc_Word16
ACMRED::DecodeSafe(
    WebRtc_UWord8* ,
    WebRtc_Word16  ,
    WebRtc_Word16* ,
    WebRtc_Word16* ,
    WebRtc_Word8*  )
{
    return 0;
}


WebRtc_Word16
ACMRED::InternalInitEncoder(
    WebRtcACMCodecParams* )
{
    
    
    return 0;
}


WebRtc_Word16
ACMRED::InternalInitDecoder(
    WebRtcACMCodecParams* )
{
   
   
   return 0;
}


WebRtc_Word32
ACMRED::CodecDef(
    WebRtcNetEQ_CodecDef& codecDef,
    const CodecInst&      codecInst)
{
    if (!_decoderInitialized)
    {
        
        
        return -1;
    }

    
    
    
    
    SET_CODEC_PAR((codecDef), kDecoderRED, codecInst.pltype, NULL, 8000);
    SET_RED_FUNCTIONS((codecDef));
    return 0;
}


ACMGenericCodec*
ACMRED::CreateInstance(void)
{
    return NULL;
}


WebRtc_Word16
ACMRED::InternalCreateEncoder()
{
    
    return 0;
}


WebRtc_Word16
ACMRED::InternalCreateDecoder()
{
    
    return 0;
}


void
ACMRED::InternalDestructEncoderInst(
    void* )
{
    
    return;
}


void
ACMRED::DestructEncoderSafe()
{
    
    return;
}

void ACMRED::DestructDecoderSafe()
{
    
    return;
}

} 

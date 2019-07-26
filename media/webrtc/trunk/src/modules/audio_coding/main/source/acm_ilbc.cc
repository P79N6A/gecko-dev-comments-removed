









#include "acm_common_defs.h"
#include "acm_ilbc.h"
#include "acm_neteq.h"
#include "trace.h"
#include "webrtc_neteq.h"
#include "webrtc_neteq_help_macros.h"

#ifdef WEBRTC_CODEC_ILBC
    #include "ilbc.h"
#endif

namespace webrtc
{

#ifndef WEBRTC_CODEC_ILBC

ACMILBC::ACMILBC(WebRtc_Word16 )
    : _encoderInstPtr(NULL),
      _decoderInstPtr(NULL) {
  return;
}


ACMILBC::~ACMILBC()
{
    return;
}


WebRtc_Word16
ACMILBC::InternalEncode(
    WebRtc_UWord8* ,
    WebRtc_Word16* )
{
    return -1;
}


WebRtc_Word16
ACMILBC::DecodeSafe(
    WebRtc_UWord8* ,
    WebRtc_Word16  ,
    WebRtc_Word16* ,
    WebRtc_Word16* ,
    WebRtc_Word8*  )
{
    return -1;
}


WebRtc_Word16
ACMILBC::InternalInitEncoder(
    WebRtcACMCodecParams* )
{
    return -1;
}


WebRtc_Word16
ACMILBC::InternalInitDecoder(
    WebRtcACMCodecParams* )
{
    return -1;
}


WebRtc_Word32
ACMILBC::CodecDef(
    WebRtcNetEQ_CodecDef& ,
    const CodecInst&      )
{
    return -1;
}


ACMGenericCodec*
ACMILBC::CreateInstance(void)
{
    return NULL;
}


WebRtc_Word16
ACMILBC::InternalCreateEncoder()
{
    return -1;
}


void
ACMILBC::DestructEncoderSafe()
{
    return;
}


WebRtc_Word16
ACMILBC::InternalCreateDecoder()
{
    return -1;
}


void
ACMILBC::DestructDecoderSafe()
{
    return;
}


void
ACMILBC::InternalDestructEncoderInst(
    void* )
{
    return;
}

WebRtc_Word16
ACMILBC::SetBitRateSafe(const WebRtc_Word32 )
{
    return -1;
}

#else     


ACMILBC::ACMILBC(
    WebRtc_Word16 codecID):
_encoderInstPtr(NULL),
_decoderInstPtr(NULL)
{
    _codecID = codecID;
    return;
}


ACMILBC::~ACMILBC()
{
    if(_encoderInstPtr != NULL)
    {
        WebRtcIlbcfix_EncoderFree(_encoderInstPtr);
        _encoderInstPtr = NULL;
    }
    if(_decoderInstPtr != NULL)
    {
        WebRtcIlbcfix_DecoderFree(_decoderInstPtr);
        _decoderInstPtr = NULL;
    }
    return;
}


WebRtc_Word16
ACMILBC::InternalEncode(
    WebRtc_UWord8* bitStream,
    WebRtc_Word16* bitStreamLenByte)
{
    *bitStreamLenByte = WebRtcIlbcfix_Encode(_encoderInstPtr,
        &_inAudio[_inAudioIxRead], _frameLenSmpl, (WebRtc_Word16*)bitStream);
    if (*bitStreamLenByte < 0)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
            "InternalEncode: error in encode for ILBC");
        return -1;
    }
    
    
    _inAudioIxRead += _frameLenSmpl;
    return *bitStreamLenByte;
}


WebRtc_Word16
ACMILBC::DecodeSafe(
    WebRtc_UWord8* ,
    WebRtc_Word16  ,
    WebRtc_Word16* ,
    WebRtc_Word16* ,
    WebRtc_Word8*  )
{
    return 0;
}


WebRtc_Word16
ACMILBC::InternalInitEncoder(
    WebRtcACMCodecParams* codecParams)
{
    
    if((160 == (codecParams->codecInstant).pacsize) ||
        (320 == (codecParams->codecInstant).pacsize))
    {
        
        return WebRtcIlbcfix_EncoderInit(_encoderInstPtr, 20);
    }
    else if((240 == (codecParams->codecInstant).pacsize) ||
        (480 == (codecParams->codecInstant).pacsize))
    {
        
        return WebRtcIlbcfix_EncoderInit(_encoderInstPtr, 30);
    }
    else
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
            "InternalInitEncoder: invalid processing block");
        return -1;
    }
}


WebRtc_Word16
ACMILBC::InternalInitDecoder(
    WebRtcACMCodecParams* codecParams)
{
    
    if((160 == (codecParams->codecInstant).pacsize) ||
        (320 == (codecParams->codecInstant).pacsize))
    {
        
        return WebRtcIlbcfix_DecoderInit(_decoderInstPtr, 20);
    }
    else if((240 == (codecParams->codecInstant).pacsize) ||
        (480 == (codecParams->codecInstant).pacsize))
    {
        
        return WebRtcIlbcfix_DecoderInit(_decoderInstPtr, 30);
    }
    else
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
            "InternalInitDecoder: invalid processing block");
        return -1;
    }
}


WebRtc_Word32
ACMILBC::CodecDef(
    WebRtcNetEQ_CodecDef& codecDef,
    const CodecInst&      codecInst)
{
    if (!_decoderInitialized)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
            "CodeDef: decoder not initialized for ILBC");
        return -1;
    }
    
    
    
    
    SET_CODEC_PAR((codecDef), kDecoderILBC, codecInst.pltype,
        _decoderInstPtr, 8000);
    SET_ILBC_FUNCTIONS((codecDef));
    return 0;
}


ACMGenericCodec*
ACMILBC::CreateInstance(void)
{
    return NULL;
}


WebRtc_Word16
ACMILBC::InternalCreateEncoder()
{
    if (WebRtcIlbcfix_EncoderCreate(&_encoderInstPtr) < 0)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
            "InternalCreateEncoder: cannot create instance for ILBC encoder");
        return -1;
    }
    return 0;
}


void
ACMILBC::DestructEncoderSafe()
{
    _encoderInitialized = false;
    _encoderExist = false;
    if(_encoderInstPtr != NULL)
    {
        WebRtcIlbcfix_EncoderFree(_encoderInstPtr);
        _encoderInstPtr = NULL;
    }
}


WebRtc_Word16
ACMILBC::InternalCreateDecoder()
{
    if (WebRtcIlbcfix_DecoderCreate(&_decoderInstPtr) < 0)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
            "InternalCreateDecoder: cannot create instance for ILBC decoder");
        return -1;
    }
    return 0;
}


void
ACMILBC::DestructDecoderSafe()
{
    _decoderInitialized = false;
    _decoderExist = false;
    if(_decoderInstPtr != NULL)
    {
        WebRtcIlbcfix_DecoderFree(_decoderInstPtr);
        _decoderInstPtr = NULL;
    }
}


void
ACMILBC::InternalDestructEncoderInst(
    void* ptrInst)
{
    if(ptrInst != NULL)
    {
        WebRtcIlbcfix_EncoderFree((iLBC_encinst_t_*)ptrInst);
    }
    return;
}

WebRtc_Word16
ACMILBC::SetBitRateSafe(const WebRtc_Word32 rate)
{
    
    if (rate == 13300)
    {
        WebRtcIlbcfix_EncoderInit(_encoderInstPtr, 30);
    }
    else if (rate == 15200)
    {
        WebRtcIlbcfix_EncoderInit(_encoderInstPtr, 20);
    }
    else
    {
        return -1;
    }
    _encoderParams.codecInstant.rate = rate;

    return 0;
}

#endif

} 

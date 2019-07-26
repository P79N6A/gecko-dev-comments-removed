









#include "acm_gsmfr.h"
#include "acm_common_defs.h"
#include "acm_neteq.h"
#include "trace.h"
#include "webrtc_neteq.h"
#include "webrtc_neteq_help_macros.h"

#ifdef WEBRTC_CODEC_GSMFR
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    #include "gsmfr_interface.h"
#endif

namespace webrtc {

#ifndef WEBRTC_CODEC_GSMFR

ACMGSMFR::ACMGSMFR(WebRtc_Word16 )
    : _encoderInstPtr(NULL),
      _decoderInstPtr(NULL) {
  return;
}


ACMGSMFR::~ACMGSMFR()
{
    return;
}


WebRtc_Word16
ACMGSMFR::InternalEncode(
    WebRtc_UWord8* ,
    WebRtc_Word16* )
{
    return -1;
}


WebRtc_Word16
ACMGSMFR::DecodeSafe(
    WebRtc_UWord8* ,
    WebRtc_Word16  ,
    WebRtc_Word16* ,
    WebRtc_Word16* ,
    WebRtc_Word8*  )
{
    return -1;
}


WebRtc_Word16
ACMGSMFR::EnableDTX()
{
    return -1;
}


WebRtc_Word16
ACMGSMFR::DisableDTX()
{
    return -1;
}


WebRtc_Word16
ACMGSMFR::InternalInitEncoder(
    WebRtcACMCodecParams* )
{
    return -1;
}


WebRtc_Word16
ACMGSMFR::InternalInitDecoder(
    WebRtcACMCodecParams* )
{
    return -1;
}


WebRtc_Word32
ACMGSMFR::CodecDef(
    WebRtcNetEQ_CodecDef& ,
    const CodecInst&      )
{
    return -1;
}


ACMGenericCodec*
ACMGSMFR::CreateInstance(void)
{
    return NULL;
}


WebRtc_Word16
ACMGSMFR::InternalCreateEncoder()
{
    return -1;
}


void
ACMGSMFR::DestructEncoderSafe()
{
    return;
}


WebRtc_Word16
ACMGSMFR::InternalCreateDecoder()
{
    return -1;
}


void
ACMGSMFR::DestructDecoderSafe()
{
    return;
}


void
ACMGSMFR::InternalDestructEncoderInst(
    void* )
{
    return;
}

#else     

ACMGSMFR::ACMGSMFR(
    WebRtc_Word16 codecID):
_encoderInstPtr(NULL),
_decoderInstPtr(NULL)
{
    _codecID = codecID;
    _hasInternalDTX = true;
    return;
}


ACMGSMFR::~ACMGSMFR()
{
    if(_encoderInstPtr != NULL)
    {
        WebRtcGSMFR_FreeEnc(_encoderInstPtr);
        _encoderInstPtr = NULL;
    }
    if(_decoderInstPtr != NULL)
    {
        WebRtcGSMFR_FreeDec(_decoderInstPtr);
        _decoderInstPtr = NULL;
    }
    return;
}


WebRtc_Word16
ACMGSMFR::InternalEncode(
    WebRtc_UWord8* bitStream,
    WebRtc_Word16* bitStreamLenByte)
{
    *bitStreamLenByte = WebRtcGSMFR_Encode(_encoderInstPtr,
        &_inAudio[_inAudioIxRead], _frameLenSmpl, (WebRtc_Word16*)bitStream);
    
    
    _inAudioIxRead += _frameLenSmpl;
    return *bitStreamLenByte;
}


WebRtc_Word16
ACMGSMFR::DecodeSafe(
    WebRtc_UWord8* ,
    WebRtc_Word16  ,
    WebRtc_Word16* ,
    WebRtc_Word16* ,
    WebRtc_Word8*  )
{
    return 0;
}


WebRtc_Word16
ACMGSMFR::EnableDTX()
{
    if(_dtxEnabled)
    {
        return 0;
    }
    else if(_encoderExist)
    {
        if(WebRtcGSMFR_EncoderInit(_encoderInstPtr, 1) < 0)
        {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
        "EnableDTX: cannot init encoder for GSMFR");
            return -1;
        }
        _dtxEnabled = true;
        return 0;
    }
    else
    {
        return -1;
    }
}


WebRtc_Word16
ACMGSMFR::DisableDTX()
{
    if(!_dtxEnabled)
    {
        return 0;
    }
    else if(_encoderExist)
    {
        if(WebRtcGSMFR_EncoderInit(_encoderInstPtr, 0) < 0)
        {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
        "DisableDTX: cannot init encoder for GSMFR");
            return -1;
        }
        _dtxEnabled = false;
        return 0;
    }
    else
    {
        
        return 0;
    }
}


WebRtc_Word16
ACMGSMFR::InternalInitEncoder(
    WebRtcACMCodecParams* codecParams)
{
    if (WebRtcGSMFR_EncoderInit(_encoderInstPtr, ((codecParams->enableDTX)? 1:0)) < 0)
  {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
      "InternalInitEncoder: cannot init encoder for GSMFR");
  }
  return 0;
}


WebRtc_Word16
ACMGSMFR::InternalInitDecoder(
    WebRtcACMCodecParams* )
{
    if (WebRtcGSMFR_DecoderInit(_decoderInstPtr) < 0)
  {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
       "InternalInitDecoder: cannot init decoder for GSMFR");
    return -1;
  }
  return 0;
}


WebRtc_Word32
ACMGSMFR::CodecDef(
    WebRtcNetEQ_CodecDef& codecDef,
    const CodecInst&      codecInst)
{
    if (!_decoderInitialized)
    {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
      "CodecDef: decoder is not initialized for GSMFR");
        return -1;
    }
    
    
    
    
    SET_CODEC_PAR((codecDef), kDecoderGSMFR, codecInst.pltype,
        _decoderInstPtr, 8000);
    SET_GSMFR_FUNCTIONS((codecDef));
    return 0;
}


ACMGenericCodec*
ACMGSMFR::CreateInstance(void)
{
    return NULL;
}


WebRtc_Word16
ACMGSMFR::InternalCreateEncoder()
{
    if (WebRtcGSMFR_CreateEnc(&_encoderInstPtr) < 0)
  {
     WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
       "InternalCreateEncoder: cannot create instance for GSMFR encoder");
    return -1;
  }
  return 0;
}


void
ACMGSMFR::DestructEncoderSafe()
{
    if(_encoderInstPtr != NULL)
    {
        WebRtcGSMFR_FreeEnc(_encoderInstPtr);
        _encoderInstPtr = NULL;
    }
    _encoderExist = false;
    _encoderInitialized = false;
}


WebRtc_Word16
ACMGSMFR::InternalCreateDecoder()
{
    if (WebRtcGSMFR_CreateDec(&_decoderInstPtr) < 0)
  {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
       "InternalCreateDecoder: cannot create instance for GSMFR decoder");
    return -1;
  }
  return 0;
}


void
ACMGSMFR::DestructDecoderSafe()
{
    if(_decoderInstPtr != NULL)
    {
        WebRtcGSMFR_FreeDec(_decoderInstPtr);
        _decoderInstPtr = NULL;
    }
    _decoderExist = false;
    _decoderInitialized = false;
}


void
ACMGSMFR::InternalDestructEncoderInst(
    void* ptrInst)
{
    if(ptrInst != NULL)
    {
        WebRtcGSMFR_FreeEnc((GSMFR_encinst_t_*)ptrInst);
    }
    return;
}

#endif

} 

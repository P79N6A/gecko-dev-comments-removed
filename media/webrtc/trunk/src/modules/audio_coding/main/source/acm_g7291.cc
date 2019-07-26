









#include "acm_g7291.h"
#include "acm_common_defs.h"
#include "acm_neteq.h"
#include "trace.h"
#include "webrtc_neteq.h"
#include "webrtc_neteq_help_macros.h"

#ifdef WEBRTC_CODEC_G729_1
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    #include "g7291_interface.h"
#endif

namespace webrtc {

#ifndef WEBRTC_CODEC_G729_1

ACMG729_1::ACMG729_1( WebRtc_Word16 )
    : _encoderInstPtr(NULL),
      _decoderInstPtr(NULL),
      _myRate(32000),
      _flag8kHz(0),
      _flagG729mode(0) {
  return;
}


ACMG729_1::~ACMG729_1()
{
    return;
}


WebRtc_Word16
ACMG729_1::InternalEncode(
    WebRtc_UWord8* ,
    WebRtc_Word16* )
{
    return -1;
}


WebRtc_Word16
ACMG729_1::DecodeSafe(
    WebRtc_UWord8* ,
    WebRtc_Word16  ,
    WebRtc_Word16* ,
    WebRtc_Word16* ,
    WebRtc_Word8*  )
{
    return -1;
}


WebRtc_Word16
ACMG729_1::InternalInitEncoder(
    WebRtcACMCodecParams* )
{
    return -1;
}


WebRtc_Word16
ACMG729_1::InternalInitDecoder(
    WebRtcACMCodecParams* )
{
    return -1;
}


WebRtc_Word32
ACMG729_1::CodecDef(
    WebRtcNetEQ_CodecDef& ,
    const CodecInst&      )
{
    return -1;
}


ACMGenericCodec*
ACMG729_1::CreateInstance(void)
{
    return NULL;
}


WebRtc_Word16
ACMG729_1::InternalCreateEncoder()
{
    return -1;
}


void
ACMG729_1::DestructEncoderSafe()
{
    return;
}


WebRtc_Word16
ACMG729_1::InternalCreateDecoder()
{
    return -1;
}


void
ACMG729_1::DestructDecoderSafe()
{
    return;
}


void
ACMG729_1::InternalDestructEncoderInst(
    void* )
{
    return;
}

WebRtc_Word16
ACMG729_1::SetBitRateSafe(
    const WebRtc_Word32  )
{
  return -1;
}

#else     

struct G729_1_inst_t_;

ACMG729_1::ACMG729_1(WebRtc_Word16 codecID)
    : _encoderInstPtr(NULL),
      _decoderInstPtr(NULL),
      _myRate(32000),  
      _flag8kHz(0),
      _flagG729mode(0) {
  
  
  _codecID = codecID;
  return;
}

ACMG729_1::~ACMG729_1()
{
    if(_encoderInstPtr != NULL)
    {
        WebRtcG7291_Free(_encoderInstPtr);
        _encoderInstPtr = NULL;
    }
    if(_decoderInstPtr != NULL)
    {
        WebRtcG7291_Free(_decoderInstPtr);
        _decoderInstPtr = NULL;
    }
    return;
}


WebRtc_Word16
ACMG729_1::InternalEncode(
    WebRtc_UWord8* bitStream,
    WebRtc_Word16* bitStreamLenByte)
{

    
    WebRtc_Word16 noEncodedSamples = 0;
    *bitStreamLenByte = 0;

  WebRtc_Word16 byteLengthFrame = 0;

    
  
    WebRtc_Word16 n20msFrames = (_frameLenSmpl / 320);
    
    byteLengthFrame = _myRate/(8*50) * n20msFrames + (1 - _flagG729mode);

    
    do
    {
        *bitStreamLenByte = WebRtcG7291_Encode(_encoderInstPtr, &_inAudio[_inAudioIxRead],
       (WebRtc_Word16*)bitStream, _myRate, n20msFrames);

        
        
    _inAudioIxRead += 160;

        
        if(*bitStreamLenByte < 0)
        {
      
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
        "InternalEncode: Encode error for G729_1");
            *bitStreamLenByte = 0;
            return -1;
        }

    noEncodedSamples += 160;
    } while(*bitStreamLenByte == 0);


    
    if(*bitStreamLenByte != byteLengthFrame)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
            "InternalEncode: Encode error for G729_1");
        *bitStreamLenByte = 0;
        return -1;
    }


    if(noEncodedSamples != _frameLenSmpl)
    {
        *bitStreamLenByte = 0;
        return -1;
    }

    return *bitStreamLenByte;
}


WebRtc_Word16
ACMG729_1::DecodeSafe(
    WebRtc_UWord8* ,
    WebRtc_Word16  ,
    WebRtc_Word16* ,
    WebRtc_Word16* ,
    WebRtc_Word8*  )
{
    return 0;
}


WebRtc_Word16
ACMG729_1::InternalInitEncoder(
    WebRtcACMCodecParams* codecParams)
{
  
  _myRate = codecParams->codecInstant.rate;
    return SetBitRateSafe( (WebRtc_UWord32)_myRate);
}


WebRtc_Word16
ACMG729_1::InternalInitDecoder(
    WebRtcACMCodecParams* )
{
    if (WebRtcG7291_DecoderInit(_decoderInstPtr) < 0)
  {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
        "InternalInitDecoder: init decoder failed for G729_1");
    return -1;
  }
  return 0;
}


WebRtc_Word32
ACMG729_1::CodecDef(
    WebRtcNetEQ_CodecDef& codecDef,
    const CodecInst&      codecInst)
{
    if (!_decoderInitialized)
    {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
      "CodeDef: Decoder uninitialized for G729_1");
      return -1;
    }

    
    
    
    
    SET_CODEC_PAR((codecDef), kDecoderG729_1, codecInst.pltype,
        _decoderInstPtr, 16000);
    SET_G729_1_FUNCTIONS((codecDef));
    return 0;
}


ACMGenericCodec*
ACMG729_1::CreateInstance(void)
{
    return NULL;
}


WebRtc_Word16
ACMG729_1::InternalCreateEncoder()
{
    if (WebRtcG7291_Create(&_encoderInstPtr) < 0)
  {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
      "InternalCreateEncoder: create encoder failed for G729_1");
    return -1;
  }
  return 0;
}


void
ACMG729_1::DestructEncoderSafe()
{
    _encoderExist = false;
    _encoderInitialized = false;
    if(_encoderInstPtr != NULL)
    {
        WebRtcG7291_Free(_encoderInstPtr);
        _encoderInstPtr = NULL;
    }
}


WebRtc_Word16
ACMG729_1::InternalCreateDecoder()
{
   if (WebRtcG7291_Create(&_decoderInstPtr) < 0)
   {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
    "InternalCreateDecoder: create decoder failed for G729_1");
     return -1;
   }
   return 0;
}


void
ACMG729_1::DestructDecoderSafe()
{
    _decoderExist = false;
    _decoderInitialized = false;
    if(_decoderInstPtr != NULL)
    {
        WebRtcG7291_Free(_decoderInstPtr);
        _decoderInstPtr = NULL;
    }
}


void
ACMG729_1::InternalDestructEncoderInst(
    void* ptrInst)
{
    if(ptrInst != NULL)
    {
        
    }
    return;
}

WebRtc_Word16
ACMG729_1::SetBitRateSafe(
    const WebRtc_Word32 rate)
{
    
    
    
    
    switch(rate)
    {
    case 8000:
        {
            _myRate = 8000;
            break;
        }
  case 12000:
        {
            _myRate = 12000;
            break;
        }
  case 14000:
        {
            _myRate = 14000;
            break;
        }
  case 16000:
        {
            _myRate = 16000;
            break;
        }
  case 18000:
        {
            _myRate = 18000;
            break;
        }
  case 20000:
        {
            _myRate = 20000;
            break;
        }
  case 22000:
        {
            _myRate = 22000;
            break;
        }
  case 24000:
        {
            _myRate = 24000;
            break;
        }
  case 26000:
        {
            _myRate = 26000;
            break;
        }
  case 28000:
        {
            _myRate = 28000;
            break;
        }
  case 30000:
        {
            _myRate = 30000;
            break;
        }
  case 32000:
        {
            _myRate = 32000;
            break;
        }
    default:
        {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
        "SetBitRateSafe: Invalid rate G729_1");
            return -1;
        }
    }

    
    if (WebRtcG7291_EncoderInit(_encoderInstPtr, _myRate, _flag8kHz, _flagG729mode) >= 0)
    {
        _encoderParams.codecInstant.rate = _myRate;
        return 0;
    }
    else
    {
        return -1;
    }
}


#endif

} 

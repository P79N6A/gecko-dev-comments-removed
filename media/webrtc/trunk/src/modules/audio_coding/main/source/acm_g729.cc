









#include "acm_g729.h"
#include "acm_common_defs.h"
#include "acm_neteq.h"
#include "trace.h"
#include "webrtc_neteq.h"
#include "webrtc_neteq_help_macros.h"

#ifdef WEBRTC_CODEC_G729
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    #include "g729_interface.h"
#endif

namespace webrtc {

#ifndef WEBRTC_CODEC_G729

ACMG729::ACMG729(WebRtc_Word16 )
    : _encoderInstPtr(NULL),
      _decoderInstPtr(NULL) {
  return;
}


ACMG729::~ACMG729()
{
    return;
}


WebRtc_Word16
ACMG729::InternalEncode(
    WebRtc_UWord8* ,
    WebRtc_Word16* )
{
    return -1;
}


WebRtc_Word16
ACMG729::EnableDTX()
{
    return -1;
}


WebRtc_Word16
ACMG729::DisableDTX()
{
    return -1;
}

WebRtc_Word32
ACMG729::ReplaceInternalDTXSafe(
    const bool )
{
    return -1;
}

WebRtc_Word32
ACMG729::IsInternalDTXReplacedSafe(
    bool* )
{
    return -1;
}


WebRtc_Word16
ACMG729::DecodeSafe(
    WebRtc_UWord8* ,
    WebRtc_Word16  ,
    WebRtc_Word16* ,
    WebRtc_Word16* ,
    WebRtc_Word8*  )
{
    return -1;
}


WebRtc_Word16
ACMG729::InternalInitEncoder(
    WebRtcACMCodecParams* )
{
    return -1;
}


WebRtc_Word16
ACMG729::InternalInitDecoder(
    WebRtcACMCodecParams* )
{
    return -1;
}


WebRtc_Word32
ACMG729::CodecDef(
    WebRtcNetEQ_CodecDef& ,
    const CodecInst&      )
{
    return -1;
}


ACMGenericCodec*
ACMG729::CreateInstance(void)
{
    return NULL;
}


WebRtc_Word16
ACMG729::InternalCreateEncoder()
{
    return -1;
}


void
ACMG729::DestructEncoderSafe()
{
    return;
}


WebRtc_Word16
ACMG729::InternalCreateDecoder()
{
    return -1;
}


void
ACMG729::DestructDecoderSafe()
{
    return;
}


void
ACMG729::InternalDestructEncoderInst(
    void* )
{
    return;
}

#else     

ACMG729::ACMG729(
    WebRtc_Word16 codecID):
_encoderInstPtr(NULL),
_decoderInstPtr(NULL)
{
    _codecID = codecID;
    _hasInternalDTX = true;
    return;
}


ACMG729::~ACMG729()
{
    if(_encoderInstPtr != NULL)
    {
        
        WebRtcG729_FreeEnc(_encoderInstPtr);
        _encoderInstPtr = NULL;
    }
    if(_decoderInstPtr != NULL)
    {
        
        WebRtcG729_FreeDec(_decoderInstPtr);
        _decoderInstPtr = NULL;
    }
    return;
}


WebRtc_Word16
ACMG729::InternalEncode(
    WebRtc_UWord8* bitStream,
    WebRtc_Word16* bitStreamLenByte)
{
    
    WebRtc_Word16 noEncodedSamples = 0;
    WebRtc_Word16 tmpLenByte = 0;
    WebRtc_Word16 vadDecision = 0;
    *bitStreamLenByte = 0;
    while(noEncodedSamples < _frameLenSmpl)
    {
        
        
        tmpLenByte = WebRtcG729_Encode(_encoderInstPtr,
            &_inAudio[_inAudioIxRead], 80,
            (WebRtc_Word16*)(&(bitStream[*bitStreamLenByte])));

        
        
        _inAudioIxRead += 80;

        
        if(tmpLenByte < 0)
        {
            
            *bitStreamLenByte = 0;
            return -1;
        }

        
        *bitStreamLenByte += tmpLenByte;
        switch(tmpLenByte)
        {
        case 0:
            {
                if(0 == noEncodedSamples)
                {
                    
                    
                    
                    
                    return 0;
                }
                break;
            }
        case 2:
            {
                
                if(_hasInternalDTX && _dtxEnabled)
                {
                    vadDecision = 0;
                    for(WebRtc_Word16 n = 0; n < MAX_FRAME_SIZE_10MSEC; n++)
                    {
                        _vadLabel[n] = vadDecision;
                    }
                }
                
                
                return *bitStreamLenByte;
            }
        case 10:
            {
                vadDecision = 1;
                
                break;
            }
        default:
            {
                return -1;
            }
        }

        
        noEncodedSamples += 80;
    }

    
    if(_hasInternalDTX && !vadDecision && _dtxEnabled)
    {
        for(WebRtc_Word16 n = 0; n < MAX_FRAME_SIZE_10MSEC; n++)
        {
            _vadLabel[n] = vadDecision;
        }
    }

    
    return *bitStreamLenByte;
}


WebRtc_Word16
ACMG729::EnableDTX()
{
    if(_dtxEnabled)
    {
        
        return 0;
    }
    else if(_encoderExist)
    {
        
        if(WebRtcG729_EncoderInit(_encoderInstPtr, 1) < 0)
        {
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
ACMG729::DisableDTX()
{
    if(!_dtxEnabled)
    {
        
        return 0;
    }
    else if(_encoderExist)
    {
        
        if(WebRtcG729_EncoderInit(_encoderInstPtr, 0) < 0)
        {
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


WebRtc_Word32
ACMG729::ReplaceInternalDTXSafe(
    const bool replaceInternalDTX)
{
    
    

    if(replaceInternalDTX == _hasInternalDTX)
    {
        
        bool oldEnableDTX = _dtxEnabled;
        bool oldEnableVAD = _vadEnabled;
        ACMVADMode oldMode = _vadMode;
        if (replaceInternalDTX)
        {
            
            DisableDTX();
        }
        else
        {
            
            ACMGenericCodec::DisableDTX();
        }
        _hasInternalDTX = !replaceInternalDTX;
        WebRtc_Word16 status = SetVADSafe(oldEnableDTX, oldEnableVAD, oldMode);
        
        
        if (status == 1) {
            _vadEnabled = true;
            return status;
        } else if (status < 0) {
            _hasInternalDTX = replaceInternalDTX;
            return -1;
        }
    }
    return 0;
}


WebRtc_Word32
ACMG729::IsInternalDTXReplacedSafe(
    bool* internalDTXReplaced)
{
    
    *internalDTXReplaced = !_hasInternalDTX;
    return 0;
}


WebRtc_Word16
ACMG729::DecodeSafe(
    WebRtc_UWord8* ,
    WebRtc_Word16  ,
    WebRtc_Word16* ,
    WebRtc_Word16* ,
    WebRtc_Word8*  )
{
    
    return 0;
}


WebRtc_Word16
ACMG729::InternalInitEncoder(
    WebRtcACMCodecParams* codecParams)
{
    
    return WebRtcG729_EncoderInit(_encoderInstPtr,
        ((codecParams->enableDTX)? 1:0));
}


WebRtc_Word16
ACMG729::InternalInitDecoder(
    WebRtcACMCodecParams* )
{
    
    return WebRtcG729_DecoderInit(_decoderInstPtr);
}


WebRtc_Word32
ACMG729::CodecDef(
    WebRtcNetEQ_CodecDef& codecDef,
    const CodecInst&      codecInst)
{
    if (!_decoderInitialized)
    {
        
        
        return -1;
    }

    
    
    
    
    SET_CODEC_PAR((codecDef), kDecoderG729, codecInst.pltype,
        _decoderInstPtr, 8000);
    SET_G729_FUNCTIONS((codecDef));
    return 0;
}


ACMGenericCodec*
ACMG729::CreateInstance(void)
{
    
    return NULL;
}


WebRtc_Word16
ACMG729::InternalCreateEncoder()
{
    
    return WebRtcG729_CreateEnc(&_encoderInstPtr);
}


void
ACMG729::DestructEncoderSafe()
{
    
    _encoderExist = false;
    _encoderInitialized = false;
    if(_encoderInstPtr != NULL)
    {
        WebRtcG729_FreeEnc(_encoderInstPtr);
        _encoderInstPtr = NULL;
    }
}


WebRtc_Word16
ACMG729::InternalCreateDecoder()
{
    
    return WebRtcG729_CreateDec(&_decoderInstPtr);
}


void
ACMG729::DestructDecoderSafe()
{
    
    _decoderExist = false;
    _decoderInitialized = false;
    if(_decoderInstPtr != NULL)
    {
        WebRtcG729_FreeDec(_decoderInstPtr);
        _decoderInstPtr = NULL;
    }
}


void
ACMG729::InternalDestructEncoderInst(
    void* ptrInst)
{
    if(ptrInst != NULL)
    {
        WebRtcG729_FreeEnc((G729_encinst_t_*)ptrInst);
    }
    return;
}

#endif

} 

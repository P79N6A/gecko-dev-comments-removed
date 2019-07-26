









#include "acm_g7221.h"
#include "acm_codec_database.h"
#include "acm_common_defs.h"
#include "acm_neteq.h"
#include "trace.h"
#include "webrtc_neteq.h"
#include "webrtc_neteq_help_macros.h"

#ifdef WEBRTC_CODEC_G722_1































































#include "g7221_interface.h"
#endif

namespace webrtc {

#ifndef WEBRTC_CODEC_G722_1

ACMG722_1::ACMG722_1(WebRtc_Word16 )
    : _operationalRate(-1),
      _encoderInstPtr(NULL),
      _encoderInstPtrRight(NULL),
      _decoderInstPtr(NULL),
      _encoderInst16Ptr(NULL),
      _encoderInst16PtrR(NULL),
      _encoderInst24Ptr(NULL),
      _encoderInst24PtrR(NULL),
      _encoderInst32Ptr(NULL),
      _encoderInst32PtrR(NULL),
      _decoderInst16Ptr(NULL),
      _decoderInst24Ptr(NULL),
      _decoderInst32Ptr(NULL) {
  return;
}

ACMG722_1::~ACMG722_1() {
  return;
}

WebRtc_Word16 ACMG722_1::InternalEncode(WebRtc_UWord8* ,
                                        WebRtc_Word16* ) {
  return -1;
}

WebRtc_Word16 ACMG722_1::DecodeSafe(WebRtc_UWord8* ,
                                    WebRtc_Word16 ,
                                    WebRtc_Word16* ,
                                    WebRtc_Word16* ,
                                    WebRtc_Word8* ) {
  return -1;
}

WebRtc_Word16 ACMG722_1::InternalInitEncoder(
    WebRtcACMCodecParams* ) {
  return -1;
}

WebRtc_Word16 ACMG722_1::InternalInitDecoder(
    WebRtcACMCodecParams* ) {
  return -1;
}

WebRtc_Word32 ACMG722_1::CodecDef(WebRtcNetEQ_CodecDef& ,
                                  const CodecInst& ) {
  return -1;
}

ACMGenericCodec* ACMG722_1::CreateInstance(void) {
  return NULL;
}

WebRtc_Word16 ACMG722_1::InternalCreateEncoder() {
  return -1;
}

void ACMG722_1::DestructEncoderSafe() {
  return;
}

WebRtc_Word16 ACMG722_1::InternalCreateDecoder() {
  return -1;
}

void ACMG722_1::DestructDecoderSafe() {
  return;
}

void ACMG722_1::InternalDestructEncoderInst(void* ) {
  return;
}

#else     
ACMG722_1::ACMG722_1(
    WebRtc_Word16 codecID):
    _encoderInstPtr(NULL),
    _encoderInstPtrRight(NULL),
    _decoderInstPtr(NULL),
    _encoderInst16Ptr(NULL),
    _encoderInst16PtrR(NULL),
    _encoderInst24Ptr(NULL),
    _encoderInst24PtrR(NULL),
    _encoderInst32Ptr(NULL),
    _encoderInst32PtrR(NULL),
    _decoderInst16Ptr(NULL),
    _decoderInst24Ptr(NULL),
    _decoderInst32Ptr(NULL) {
  _codecID = codecID;
  if (_codecID == ACMCodecDB::kG722_1_16) {
    _operationalRate = 16000;
  } else if (_codecID == ACMCodecDB::kG722_1_24) {
    _operationalRate = 24000;
  } else if (_codecID == ACMCodecDB::kG722_1_32) {
    _operationalRate = 32000;
  } else {
    _operationalRate = -1;
  }
  return;
}

ACMG722_1::~ACMG722_1() {
  if (_encoderInstPtr != NULL) {
    delete _encoderInstPtr;
    _encoderInstPtr = NULL;
  }
  if (_encoderInstPtrRight != NULL) {
    delete _encoderInstPtrRight;
    _encoderInstPtrRight = NULL;
  }
  if (_decoderInstPtr != NULL) {
    delete _decoderInstPtr;
    _decoderInstPtr = NULL;
  }

  switch (_operationalRate) {
    case 16000: {
      _encoderInst16Ptr = NULL;
      _encoderInst16PtrR = NULL;
      _decoderInst16Ptr = NULL;
      break;
    }
    case 24000: {
      _encoderInst24Ptr = NULL;
      _encoderInst24PtrR = NULL;
      _decoderInst24Ptr = NULL;
      break;
    }
    case 32000: {
      _encoderInst32Ptr = NULL;
      _encoderInst32PtrR = NULL;
      _decoderInst32Ptr = NULL;
      break;
    }
    default: {
      break;
    }
  }
  return;
}

WebRtc_Word16 ACMG722_1::InternalEncode(WebRtc_UWord8* bitStream,
                                        WebRtc_Word16* bitStreamLenByte) {
  WebRtc_Word16 leftChannel[320];
  WebRtc_Word16 rightChannel[320];
  WebRtc_Word16 lenInBytes;
  WebRtc_Word16 outB[160];

  
  if (_noChannels == 2) {
    for (int i = 0, j = 0; i < _frameLenSmpl * 2; i += 2, j++) {
      leftChannel[j] = _inAudio[_inAudioIxRead + i];
      rightChannel[j] = _inAudio[_inAudioIxRead + i + 1];
    }
  } else {
    memcpy(leftChannel, &_inAudio[_inAudioIxRead], 320);
  }

  switch (_operationalRate) {
    case 16000: {
      Inst lenInBytes = WebRtcG7221_Encode16(_encoderInst16Ptr, leftChannel,
                                             320, &outB[0]);
      if (_noChannels == 2) {
        lenInBytes += WebRtcG7221_Encode16(_encoderInst16PtrR, rightChannel,
                                           320, &outB[lenInBytes / 2]);
      }
      break;
    }
    case 24000: {
      lenInBytes = WebRtcG7221_Encode24(_encoderInst24Ptr, leftChannel, 320,
                                        &outB[0]);
      if (_noChannels == 2) {
        lenInBytes += WebRtcG7221_Encode24(_encoderInst24PtrR, rightChannel,
                                           320, &outB[lenInBytes / 2]);
      }
      break;
    }
    case 32000: {
      lenInBytes = WebRtcG7221_Encode32(_encoderInst32Ptr, leftChannel, 320,
                                        &outB[0]);
      if (_noChannels == 2) {
        lenInBytes += WebRtcG7221_Encode32(_encoderInst32PtrR, rightChannel,
                                           320, &outB[lenInBytes / 2]);
      }
      break;
    }
    default: {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
                   "InternalInitEncode: Wrong rate for G722_1.");
      return -1;
    }
  }
  memcpy(bitStream, outB, lenInBytes);
  *bitStreamLenByte = lenInBytes;

  
  
  _inAudioIxRead += 320 * _noChannels;
  return *bitStreamLenByte;
}

WebRtc_Word16 ACMG722_1::DecodeSafe(WebRtc_UWord8* ,
                                    WebRtc_Word16 ,
                                    WebRtc_Word16* ,
                                    WebRtc_Word16* ,
                                    WebRtc_Word8* ) {
  return 0;
}

WebRtc_Word16 ACMG722_1::InternalInitEncoder(
    WebRtcACMCodecParams* codecParams) {
  WebRtc_Word16 ret;

  switch (_operationalRate) {
    case 16000: {
      ret = WebRtcG7221_EncoderInit16(_encoderInst16PtrR);
      if (ret < 0) {
        return ret;
      }
      return WebRtcG7221_EncoderInit16(_encoderInst16Ptr);
    }
    case 24000: {
      ret = WebRtcG7221_EncoderInit24(_encoderInst24PtrR);
      if (ret < 0) {
        return ret;
      }
      return WebRtcG7221_EncoderInit24(_encoderInst24Ptr);
    }
    case 32000: {
      ret = WebRtcG7221_EncoderInit32(_encoderInst32PtrR);
      if (ret < 0) {
        return ret;
      }
      return WebRtcG7221_EncoderInit32(_encoderInst32Ptr);
    }
    default: {
      WEBRTC_TRACE(webrtc::kTraceError,Inst webrtc::kTraceAudioCoding,
                   _uniqueID, "InternalInitEncoder: Wrong rate for G722_1.");
      return -1;
    }
  }
}

WebRtc_Word16 ACMG722_1::InternalInitDecoder(
    WebRtcACMCodecParams* ) {
  switch (_operationalRate) {
    case 16000: {
      return WebRtcG7221_DecoderInit16(_decoderInst16Ptr);
    }
    case 24000: {
      return WebRtcG7221_DecoderInit24(_decoderInst24Ptr);
    }
    case 32000: {
      return WebRtcG7221_DecoderInit32(_decoderInst32Ptr);
    }
    default: {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
                   "InternalInitDecoder: Wrong rate for G722_1.");
      return -1;
    }
  }
}

WebRtc_Word32 ACMG722_1::CodecDef(WebRtcNetEQ_CodecDef& codecDef,
                                  const CodecInst& codecInst) {
  if (!_decoderInitialized) {
    
    
    return -1;
  }
  
  
  
  
  
  
  
  
  switch (_operationalRate) {
    case 16000: {
      SET_CODEC_PAR((codecDef), kDecoderG722_1_16, codecInst.pltype,
          _decoderInst16Ptr, 16000);
      SET_G722_1_16_FUNCTIONS((codecDef));
      break;
    }
    case 24000: {
      SET_CODEC_PAR((codecDef), kDecoderG722_1_24, codecInst.pltype,
          _decoderInst24Ptr, 16000);
      SET_G722_1_24_FUNCTIONS((codecDef));
      break;
    }
    case 32000: {
      SET_CODEC_PAR((codecDef), kDecoderG722_1_32, codecInst.pltype,
          _decoderInst32Ptr, 16000);
      SET_G722_1_32_FUNCTIONS((codecDef));
      break;
    }
    default: {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
                   "CodecDef: Wrong rate for G722_1.");
      return -1;
    }
  }
  return 0;
}

ACMGenericCodec* ACMG722_1::CreateInstance(void) {
  return NULL;
}

WebRtc_Word16 ACMG722_1::InternalCreateEncoder() {
  if ((_encoderInstPtr == NULL) || (_encoderInstPtrRight == NULL)) {
    return -1;
  }
  switch (_operationalRate) {
    case 16000: {
      WebRtcG7221_CreateEnc16(&_encoderInst16Ptr);
      WebRtcG7221_CreateEnc16(&_encoderInst16PtrR);
      break;
    }
    case 24000: {
      WebRtcG7221_CreateEnc24(&_encoderInst24Ptr);
      WebRtcG7221_CreateEnc24(&_encoderInst24PtrR);
      break;
    }
    case 32000: {
      WebRtcG7221_CreateEnc32(&_encoderInst32Ptr);
      WebRtcG7221_CreateEnc32(&_encoderInst32PtrR);
      break;
    }
    default: {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
                   "InternalCreateEncoder: Wrong rate for G722_1.");
      return -1;
    }
  }
  return 0;
}

void ACMG722_1::DestructEncoderSafe() {
  _encoderExist = false;
  _encoderInitialized = false;
  if (_encoderInstPtr != NULL) {
    delete _encoderInstPtr;
    _encoderInstPtr = NULL;
  }
  if (_encoderInstPtrRight != NULL) {
    delete _encoderInstPtrRight;
    _encoderInstPtrRight = NULL;
  }
  _encoderInst16Ptr = NULL;
  _encoderInst24Ptr = NULL;
  _encoderInst32Ptr = NULL;
}

WebRtc_Word16 ACMG722_1::InternalCreateDecoder() {
  if (_decoderInstPtr == NULL) {
    return -1;
  }
  switch (_operationalRate) {
    case 16000: {
      WebRtcG7221_CreateDec16(&_decoderInst16Ptr);
      break;
    }
    case 24000: {
      WebRtcG7221_CreateDec24(&_decoderInst24Ptr);
      break;
    }
    case 32000: {
      WebRtcG7221_CreateDec32(&_decoderInst32Ptr);
      break;
    }
    default: {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
                   "InternalCreateDecoder: Wrong rate for G722_1.");
      return -1;
    }
  }
  return 0;
}

void ACMG722_1::DestructDecoderSafe() {
  _decoderExist = false;
  _decoderInitialized = false;
  if (_decoderInstPtr != NULL) {
    delete _decoderInstPtr;
    _decoderInstPtr = NULL;
  }
  _decoderInst16Ptr = NULL;
  _decoderInst24Ptr = NULL;
  _decoderInst32Ptr = NULL;
}

void ACMG722_1::InternalDestructEncoderInst(void* ptrInst) {
  if (ptrInst != NULL) {
    delete ptrInst;
  }
  return;
}

#endif

} 

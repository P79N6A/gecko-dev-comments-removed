









#include "acm_g722.h"
#include "acm_codec_database.h"
#include "acm_common_defs.h"
#include "acm_neteq.h"
#include "trace.h"
#include "webrtc_neteq.h"
#include "webrtc_neteq_help_macros.h"
#include "g722_interface.h"

namespace webrtc {

#ifndef WEBRTC_CODEC_G722

ACMG722::ACMG722(WebRtc_Word16 )
    : _ptrEncStr(NULL),
      _ptrDecStr(NULL),
      _encoderInstPtr(NULL),
      _encoderInstPtrRight(NULL),
      _decoderInstPtr(NULL) {
  return;
}

ACMG722::~ACMG722() {
  return;
}

WebRtc_Word32 ACMG722::Add10MsDataSafe(const WebRtc_UWord32 ,
                                       const WebRtc_Word16* ,
                                       const WebRtc_UWord16 ,
                                       const WebRtc_UWord8 ) {
  return -1;
}

WebRtc_Word16 ACMG722::InternalEncode(WebRtc_UWord8* ,
                                      WebRtc_Word16* ) {
  return -1;
}

WebRtc_Word16 ACMG722::DecodeSafe(WebRtc_UWord8* ,
                                  WebRtc_Word16 ,
                                  WebRtc_Word16* ,
                                  WebRtc_Word16* ,
                                  WebRtc_Word8* ) {
  return -1;
}

WebRtc_Word16 ACMG722::InternalInitEncoder(
    WebRtcACMCodecParams* ) {
  return -1;
}

WebRtc_Word16 ACMG722::InternalInitDecoder(
    WebRtcACMCodecParams* ) {
  return -1;
}

WebRtc_Word32 ACMG722::CodecDef(WebRtcNetEQ_CodecDef& ,
                                const CodecInst& ) {
  return -1;
}

ACMGenericCodec* ACMG722::CreateInstance(void) {
  return NULL;
}

WebRtc_Word16 ACMG722::InternalCreateEncoder() {
  return -1;
}

void ACMG722::DestructEncoderSafe() {
  return;
}

WebRtc_Word16 ACMG722::InternalCreateDecoder() {
  return -1;
}

void ACMG722::DestructDecoderSafe() {
  return;
}

void ACMG722::InternalDestructEncoderInst(void* ) {
  return;
}

void ACMG722::SplitStereoPacket(uint8_t* ,
                                int32_t* ) {}

#else     


struct ACMG722EncStr {
  G722EncInst* inst; 
  G722EncInst* instRight; 
};
struct ACMG722DecStr {
  G722DecInst* inst; 
  G722DecInst* instRight; 
};

ACMG722::ACMG722(WebRtc_Word16 codecID)
    : _encoderInstPtr(NULL),
      _encoderInstPtrRight(NULL),
      _decoderInstPtr(NULL) {
  
  _ptrEncStr = new ACMG722EncStr;
  if (_ptrEncStr != NULL) {
    _ptrEncStr->inst = NULL;
    _ptrEncStr->instRight = NULL;
  }
  
  _ptrDecStr = new ACMG722DecStr;
  if (_ptrDecStr != NULL) {
    _ptrDecStr->inst = NULL;
    _ptrDecStr->instRight = NULL; 
  }
  _codecID = codecID;
  return;
}

ACMG722::~ACMG722() {
  
  if (_ptrEncStr != NULL) {
    if (_ptrEncStr->inst != NULL) {
      WebRtcG722_FreeEncoder(_ptrEncStr->inst);
      _ptrEncStr->inst = NULL;
    }
    if (_ptrEncStr->instRight != NULL) {
      WebRtcG722_FreeEncoder(_ptrEncStr->instRight);
      _ptrEncStr->instRight = NULL;
    }
    delete _ptrEncStr;
    _ptrEncStr = NULL;
  }
  
  if (_ptrDecStr != NULL) {
    if (_ptrDecStr->inst != NULL) {
      WebRtcG722_FreeDecoder(_ptrDecStr->inst);
      _ptrDecStr->inst = NULL;
    }
    if (_ptrDecStr->instRight != NULL) {
      WebRtcG722_FreeDecoder(_ptrDecStr->instRight);
      _ptrDecStr->instRight = NULL;
    }
    delete _ptrDecStr;
    _ptrDecStr = NULL;
  }
  return;
}

WebRtc_Word32 ACMG722::Add10MsDataSafe(const WebRtc_UWord32 timestamp,
                                       const WebRtc_Word16* data,
                                       const WebRtc_UWord16 lengthSmpl,
                                       const WebRtc_UWord8 audioChannel) {
  return ACMGenericCodec::Add10MsDataSafe((timestamp >> 1), data, lengthSmpl,
                                          audioChannel);
}

WebRtc_Word16 ACMG722::InternalEncode(WebRtc_UWord8* bitStream,
                                      WebRtc_Word16* bitStreamLenByte) {
  
  if (_noChannels == 2) {
    WebRtc_Word16 leftChannel[960];
    WebRtc_Word16 rightChannel[960];
    WebRtc_UWord8 outLeft[480];
    WebRtc_UWord8 outRight[480];
    WebRtc_Word16 lenInBytes;
    for (int i = 0, j = 0; i < _frameLenSmpl * 2; i += 2, j++) {
      leftChannel[j] = _inAudio[_inAudioIxRead + i];
      rightChannel[j] = _inAudio[_inAudioIxRead + i + 1];
    }
    lenInBytes = WebRtcG722_Encode(_encoderInstPtr, leftChannel, _frameLenSmpl,
                                   (WebRtc_Word16*) outLeft);
    lenInBytes += WebRtcG722_Encode(_encoderInstPtrRight, rightChannel,
                                    _frameLenSmpl, (WebRtc_Word16*) outRight);
    *bitStreamLenByte = lenInBytes;

    
    for (int i = 0, j = 0; i < lenInBytes; i += 2, j++) {
      bitStream[i] = (outLeft[j] & 0xF0) + (outRight[j] >> 4);
      bitStream[i + 1] = ((outLeft[j] & 0x0F) << 4) + (outRight[j] & 0x0F);
    }
  } else {
    *bitStreamLenByte = WebRtcG722_Encode(_encoderInstPtr,
                                          &_inAudio[_inAudioIxRead],
                                          _frameLenSmpl,
                                          (WebRtc_Word16*) bitStream);
  }

  
  
  _inAudioIxRead += _frameLenSmpl * _noChannels;
  return *bitStreamLenByte;
}

WebRtc_Word16 ACMG722::DecodeSafe(WebRtc_UWord8* ,
                                  WebRtc_Word16 ,
                                  WebRtc_Word16* ,
                                  WebRtc_Word16* ,
                                  WebRtc_Word8* ) {
  return 0;
}

WebRtc_Word16 ACMG722::InternalInitEncoder(WebRtcACMCodecParams* codecParams) {
  if (codecParams->codecInstant.channels == 2) {
    
    if (_ptrEncStr->instRight == NULL) {
      WebRtcG722_CreateEncoder(&_ptrEncStr->instRight);
      if (_ptrEncStr->instRight == NULL) {
        return -1;
      }
    }
    _encoderInstPtrRight = _ptrEncStr->instRight;
    if (WebRtcG722_EncoderInit(_encoderInstPtrRight) < 0) {
      return -1;
    }
  }

  return WebRtcG722_EncoderInit(_encoderInstPtr);
}

WebRtc_Word16 ACMG722::InternalInitDecoder(
    WebRtcACMCodecParams* ) {
  return WebRtcG722_DecoderInit(_decoderInstPtr);
}

WebRtc_Word32 ACMG722::CodecDef(WebRtcNetEQ_CodecDef& codecDef,
                                const CodecInst& codecInst) {
  if (!_decoderInitialized) {
    
    return -1;
  }
  
  
  
  
  if (codecInst.channels == 1) {
    SET_CODEC_PAR(codecDef, kDecoderG722, codecInst.pltype, _decoderInstPtr,
                  16000);
  } else {
    SET_CODEC_PAR(codecDef, kDecoderG722_2ch, codecInst.pltype,
                  _decoderInstPtr, 16000);
  }
  SET_G722_FUNCTIONS(codecDef);
  return 0;
}

ACMGenericCodec* ACMG722::CreateInstance(void) {
  return NULL;
}

WebRtc_Word16 ACMG722::InternalCreateEncoder() {
  if (_ptrEncStr == NULL) {
    
    
    
    return -1;
  }
  WebRtcG722_CreateEncoder(&_ptrEncStr->inst);
  if (_ptrEncStr->inst == NULL) {
    return -1;
  }
  _encoderInstPtr = _ptrEncStr->inst;
  return 0;
}

void ACMG722::DestructEncoderSafe() {
  if (_ptrEncStr != NULL) {
    if (_ptrEncStr->inst != NULL) {
      WebRtcG722_FreeEncoder(_ptrEncStr->inst);
      _ptrEncStr->inst = NULL;
    }
  }
  _encoderExist = false;
  _encoderInitialized = false;
}

WebRtc_Word16 ACMG722::InternalCreateDecoder() {
  if (_ptrDecStr == NULL) {
    
    
    
    return -1;
  }

  WebRtcG722_CreateDecoder(&_ptrDecStr->inst);
  if (_ptrDecStr->inst == NULL) {
    return -1;
  }
  _decoderInstPtr = _ptrDecStr->inst;
  return 0;
}

void ACMG722::DestructDecoderSafe() {
  _decoderExist = false;
  _decoderInitialized = false;
  if (_ptrDecStr != NULL) {
    if (_ptrDecStr->inst != NULL) {
      WebRtcG722_FreeDecoder(_ptrDecStr->inst);
      _ptrDecStr->inst = NULL;
    }
  }
}

void ACMG722::InternalDestructEncoderInst(void* ptrInst) {
  if (ptrInst != NULL) {
    WebRtcG722_FreeEncoder(static_cast<G722EncInst*>(ptrInst));
  }
  return;
}



void ACMG722::SplitStereoPacket(uint8_t* payload, int32_t* payload_length) {
  uint8_t right_byte;

  
  assert(payload != NULL);
  assert(*payload_length > 0);

  
  
  
  for (int i = 0; i < *payload_length; i += 2) {
    right_byte = ((payload[i] & 0x0F) << 4) + (payload[i + 1] & 0x0F);
    payload[i] = (payload[i] & 0xF0) + (payload[i + 1] >> 4);
    payload[i + 1] = right_byte;
  }

  
  
  
  
  for (int i = 0; i < *payload_length / 2; i++) {
    right_byte = payload[i + 1];
    memmove(&payload[i + 1], &payload[i + 2], *payload_length - i - 2);
    payload[*payload_length - 1] = right_byte;
  }
}

#endif

} 

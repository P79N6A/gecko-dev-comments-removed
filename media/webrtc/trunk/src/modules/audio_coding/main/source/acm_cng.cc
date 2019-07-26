









#include "acm_cng.h"
#include "acm_codec_database.h"
#include "acm_common_defs.h"
#include "acm_neteq.h"
#include "trace.h"
#include "webrtc_cng.h"
#include "webrtc_neteq.h"
#include "webrtc_neteq_help_macros.h"

namespace webrtc {

ACMCNG::ACMCNG(WebRtc_Word16 codecID) {
  _encoderInstPtr = NULL;
  _decoderInstPtr = NULL;
  _codecID = codecID;
  _sampFreqHz = ACMCodecDB::CodecFreq(_codecID);
  return;
}

ACMCNG::~ACMCNG() {
  if (_encoderInstPtr != NULL) {
    WebRtcCng_FreeEnc(_encoderInstPtr);
    _encoderInstPtr = NULL;
  }
  if (_decoderInstPtr != NULL) {
    WebRtcCng_FreeDec(_decoderInstPtr);
    _decoderInstPtr = NULL;
  }
  return;
}





WebRtc_Word16 ACMCNG::InternalEncode(WebRtc_UWord8* ,
                                     WebRtc_Word16* ) {
  return -1;
}

WebRtc_Word16 ACMCNG::DecodeSafe(WebRtc_UWord8* ,
                                 WebRtc_Word16 ,
                                 WebRtc_Word16* ,
                                 WebRtc_Word16* ,
                                 WebRtc_Word8* ) {
  return 0;
}





WebRtc_Word16 ACMCNG::InternalInitEncoder(
    WebRtcACMCodecParams* ) {
  return -1;
}

WebRtc_Word16 ACMCNG::InternalInitDecoder(
    WebRtcACMCodecParams* ) {
  return WebRtcCng_InitDec(_decoderInstPtr);
}

WebRtc_Word32 ACMCNG::CodecDef(WebRtcNetEQ_CodecDef& codecDef,
                               const CodecInst& codecInst) {
  if (!_decoderInitialized) {
    
    return -1;
  }
  
  
  
  

  if (_sampFreqHz == 8000 || _sampFreqHz == 16000 || _sampFreqHz == 32000) {
    SET_CODEC_PAR((codecDef), kDecoderCNG, codecInst.pltype,
        _decoderInstPtr, _sampFreqHz);
    SET_CNG_FUNCTIONS((codecDef));
    return 0;
  } else {
    return -1;
  }
}

ACMGenericCodec* ACMCNG::CreateInstance(void) {
  return NULL;
}

WebRtc_Word16 ACMCNG::InternalCreateEncoder() {
  if (WebRtcCng_CreateEnc(&_encoderInstPtr) < 0) {
    _encoderInstPtr = NULL;
    return -1;
  } else {
    return 0;
  }
}

void ACMCNG::DestructEncoderSafe() {
  if (_encoderInstPtr != NULL) {
    WebRtcCng_FreeEnc(_encoderInstPtr);
    _encoderInstPtr = NULL;
  }
  _encoderExist = false;
  _encoderInitialized = false;
}

WebRtc_Word16 ACMCNG::InternalCreateDecoder() {
  if (WebRtcCng_CreateDec(&_decoderInstPtr) < 0) {
    _decoderInstPtr = NULL;
    return -1;
  } else {
    return 0;
  }
}

void ACMCNG::DestructDecoderSafe() {
  if (_decoderInstPtr != NULL) {
    WebRtcCng_FreeDec(_decoderInstPtr);
    _decoderInstPtr = NULL;
  }
  _decoderExist = false;
  _decoderInitialized = false;
}

void ACMCNG::InternalDestructEncoderInst(void* ptrInst) {
  if (ptrInst != NULL) {
    WebRtcCng_FreeEnc(static_cast<CNG_enc_inst*>(ptrInst));
  }
  return;
}

} 

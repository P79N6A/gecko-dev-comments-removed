









#include "acm_pcmu.h"

#include "acm_common_defs.h"
#include "acm_neteq.h"
#include "trace.h"
#include "webrtc_neteq.h"
#include "webrtc_neteq_help_macros.h"


#include "g711_interface.h"

namespace webrtc {

ACMPCMU::ACMPCMU(WebRtc_Word16 codecID) {
  _codecID = codecID;
}

ACMPCMU::~ACMPCMU() {
  return;
}

WebRtc_Word16 ACMPCMU::InternalEncode(WebRtc_UWord8* bitStream,
                                      WebRtc_Word16* bitStreamLenByte) {
  *bitStreamLenByte = WebRtcG711_EncodeU(NULL, &_inAudio[_inAudioIxRead],
                                         _frameLenSmpl * _noChannels,
                                         (WebRtc_Word16*) bitStream);
  
  
  _inAudioIxRead += _frameLenSmpl * _noChannels;
  return *bitStreamLenByte;
}

WebRtc_Word16 ACMPCMU::DecodeSafe(WebRtc_UWord8* ,
                                  WebRtc_Word16 ,
                                  WebRtc_Word16* ,
                                  WebRtc_Word16* ,
                                  WebRtc_Word8* ) {
  return 0;
}

WebRtc_Word16 ACMPCMU::InternalInitEncoder(
    WebRtcACMCodecParams* ) {
  
  return 0;
}

WebRtc_Word16 ACMPCMU::InternalInitDecoder(
    WebRtcACMCodecParams* ) {
   
   return 0;
}

WebRtc_Word32 ACMPCMU::CodecDef(WebRtcNetEQ_CodecDef& codecDef,
                                const CodecInst& codecInst) {
  
  
  
  if (codecInst.channels == 1) {
    
    SET_CODEC_PAR(codecDef, kDecoderPCMu, codecInst.pltype, NULL, 8000);
  } else {
    
    SET_CODEC_PAR(codecDef, kDecoderPCMu_2ch, codecInst.pltype, NULL, 8000);
  }
  SET_PCMU_FUNCTIONS(codecDef);
  return 0;
}

ACMGenericCodec* ACMPCMU::CreateInstance(void) {
  return NULL;
}

WebRtc_Word16 ACMPCMU::InternalCreateEncoder() {
  
  return 0;
}

WebRtc_Word16 ACMPCMU::InternalCreateDecoder() {
  
  return 0;
}

void ACMPCMU::InternalDestructEncoderInst(void* ) {
  
  return;
}

void ACMPCMU::DestructEncoderSafe() {
  
  _encoderExist = false;
  _encoderInitialized = false;
  return;
}

void ACMPCMU::DestructDecoderSafe() {
  
  _decoderInitialized = false;
  _decoderExist = false;
  return;
}



void ACMPCMU::SplitStereoPacket(uint8_t* payload, int32_t* payload_length) {
  uint8_t right_byte;

  
  assert(payload != NULL);
  assert(*payload_length > 0);

  
  
  
  
  for (int i = 0; i < *payload_length / 2; i ++) {
    right_byte = payload[i + 1];
    memmove(&payload[i + 1], &payload[i + 2], *payload_length - i - 2);
    payload[*payload_length - 1] = right_byte;
  }
}

} 

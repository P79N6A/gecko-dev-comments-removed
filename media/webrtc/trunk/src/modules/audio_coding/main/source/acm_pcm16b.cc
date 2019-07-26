









#include "acm_pcm16b.h"

#include "acm_codec_database.h"
#include "acm_common_defs.h"
#include "acm_neteq.h"
#include "trace.h"
#include "webrtc_neteq.h"
#include "webrtc_neteq_help_macros.h"

#ifdef WEBRTC_CODEC_PCM16
    #include "pcm16b.h"
#endif

namespace webrtc {

#ifndef WEBRTC_CODEC_PCM16

ACMPCM16B::ACMPCM16B(WebRtc_Word16 ) {
  return;
}

ACMPCM16B::~ACMPCM16B() {
  return;
}

WebRtc_Word16 ACMPCM16B::InternalEncode(WebRtc_UWord8* ,
                                        WebRtc_Word16* ) {
  return -1;
}

WebRtc_Word16 ACMPCM16B::DecodeSafe(WebRtc_UWord8* ,
                                    WebRtc_Word16 ,
                                    WebRtc_Word16* ,
                                    WebRtc_Word16* ,
                                    WebRtc_Word8* ) {
  return -1;
}

WebRtc_Word16 ACMPCM16B::InternalInitEncoder(
    WebRtcACMCodecParams* ) {
  return -1;
}

WebRtc_Word16 ACMPCM16B::InternalInitDecoder(
    WebRtcACMCodecParams* ) {
  return -1;
}

WebRtc_Word32 ACMPCM16B::CodecDef(WebRtcNetEQ_CodecDef& ,
                                  const CodecInst& ) {
  return -1;
}

ACMGenericCodec* ACMPCM16B::CreateInstance(void) {
  return NULL;
}

WebRtc_Word16 ACMPCM16B::InternalCreateEncoder() {
  return -1;
}

WebRtc_Word16 ACMPCM16B::InternalCreateDecoder() {
  return -1;
}

void ACMPCM16B::InternalDestructEncoderInst(void* ) {
  return;
}

void ACMPCM16B::DestructEncoderSafe() {
  return;
}

void ACMPCM16B::DestructDecoderSafe() {
  return;
}

void ACMPCM16B::SplitStereoPacket(uint8_t* ,
                                  int32_t* ) {
}

#else     

ACMPCM16B::ACMPCM16B(WebRtc_Word16 codecID) {
  _codecID = codecID;
  _samplingFreqHz = ACMCodecDB::CodecFreq(_codecID);
}

ACMPCM16B::~ACMPCM16B() {
  return;
}

WebRtc_Word16 ACMPCM16B::InternalEncode(WebRtc_UWord8* bitStream,
                                        WebRtc_Word16* bitStreamLenByte) {
  *bitStreamLenByte = WebRtcPcm16b_Encode(&_inAudio[_inAudioIxRead],
                                          _frameLenSmpl * _noChannels,
                                          bitStream);
  
  
  _inAudioIxRead += _frameLenSmpl * _noChannels;
  return *bitStreamLenByte;
}

WebRtc_Word16 ACMPCM16B::DecodeSafe(WebRtc_UWord8* ,
                                    WebRtc_Word16 ,
                                    WebRtc_Word16* ,
                                    WebRtc_Word16* ,
                                    WebRtc_Word8* ) {
  return 0;
}

WebRtc_Word16 ACMPCM16B::InternalInitEncoder(
    WebRtcACMCodecParams* ) {
  
  return 0;
}

WebRtc_Word16 ACMPCM16B::InternalInitDecoder(
    WebRtcACMCodecParams* ) {
  
  return 0;
}

WebRtc_Word32 ACMPCM16B::CodecDef(WebRtcNetEQ_CodecDef& codecDef,
                                  const CodecInst& codecInst) {
  
  
  if (codecInst.channels == 1) {
    switch(_samplingFreqHz) {
      case 8000: {
        SET_CODEC_PAR(codecDef, kDecoderPCM16B, codecInst.pltype, NULL, 8000);
        SET_PCM16B_FUNCTIONS(codecDef);
        break;
      }
      case 16000: {
        SET_CODEC_PAR(codecDef, kDecoderPCM16Bwb, codecInst.pltype, NULL,
                      16000);
        SET_PCM16B_WB_FUNCTIONS(codecDef);
        break;
      }
      case 32000: {
        SET_CODEC_PAR(codecDef, kDecoderPCM16Bswb32kHz, codecInst.pltype,
                      NULL, 32000);
        SET_PCM16B_SWB32_FUNCTIONS(codecDef);
        break;
      }
      default: {
        return -1;
      }
    }
  } else {
    switch(_samplingFreqHz) {
      case 8000: {
        SET_CODEC_PAR(codecDef, kDecoderPCM16B_2ch, codecInst.pltype, NULL,
                      8000);
        SET_PCM16B_FUNCTIONS(codecDef);
        break;
      }
      case 16000: {
        SET_CODEC_PAR(codecDef, kDecoderPCM16Bwb_2ch, codecInst.pltype,
                      NULL, 16000);
        SET_PCM16B_WB_FUNCTIONS(codecDef);
        break;
      }
      case 32000: {
        SET_CODEC_PAR(codecDef, kDecoderPCM16Bswb32kHz_2ch, codecInst.pltype,
                      NULL, 32000);
        SET_PCM16B_SWB32_FUNCTIONS(codecDef);
        break;
      }
      default: {
        return -1;
      }
    }
  }
  return 0;
}

ACMGenericCodec* ACMPCM16B::CreateInstance(void) {
  return NULL;
}

WebRtc_Word16 ACMPCM16B::InternalCreateEncoder() {
  
  return 0;
}

WebRtc_Word16 ACMPCM16B::InternalCreateDecoder() {
  
  return 0;
}

void ACMPCM16B::InternalDestructEncoderInst(void* ) {
  
  return;
}

void ACMPCM16B::DestructEncoderSafe() {
  
  _encoderExist = false;
  _encoderInitialized = false;
  return;
}

void ACMPCM16B::DestructDecoderSafe() {
  
  _decoderExist = false;
  _decoderInitialized = false;
  return;
}



void ACMPCM16B::SplitStereoPacket(uint8_t* payload, int32_t* payload_length) {
  uint8_t right_byte_msb;
  uint8_t right_byte_lsb;

  
  assert(payload != NULL);
  assert(*payload_length > 0);

  
  
  
  

  for (int i = 0; i < *payload_length / 2; i += 2) {
    right_byte_msb = payload[i + 2];
    right_byte_lsb = payload[i + 3];
    memmove(&payload[i + 2], &payload[i + 4], *payload_length - i - 4);
    payload[*payload_length - 2] = right_byte_msb;
    payload[*payload_length - 1] = right_byte_lsb;
  }
}
#endif

} 











#include "acm_common_defs.h"
#include "acm_neteq.h"
#include "acm_celt.h"
#include "trace.h"
#include "webrtc_neteq.h"
#include "webrtc_neteq_help_macros.h"


#ifdef WEBRTC_CODEC_CELT


#include "celt_interface.h"
#endif

namespace webrtc {

#ifndef WEBRTC_CODEC_CELT

ACMCELT::ACMCELT(int16_t )
    : enc_inst_ptr_(NULL),
      dec_inst_ptr_(NULL),
      sampling_freq_(0),
      bitrate_(0),
      channels_(1),
      dec_channels_(1) {
  return;
}

ACMCELT::~ACMCELT() {
  return;
}

int16_t ACMCELT::InternalEncode(uint8_t* ,
                                int16_t* ) {
  return -1;
}

int16_t ACMCELT::DecodeSafe(uint8_t* ,
                            int16_t ,
                            int16_t* ,
                            int16_t* ,
                            WebRtc_Word8* ) {
  return -1;
}

int16_t ACMCELT::InternalInitEncoder(WebRtcACMCodecParams* ) {
  return -1;
}

int16_t ACMCELT::InternalInitDecoder(WebRtcACMCodecParams* ) {
  return -1;
}

int32_t ACMCELT::CodecDef(WebRtcNetEQ_CodecDef& ,
                          const CodecInst& ) {
  return -1;
}

ACMGenericCodec* ACMCELT::CreateInstance(void) {
  return NULL;
}

int16_t ACMCELT::InternalCreateEncoder() {
  return -1;
}

void ACMCELT::DestructEncoderSafe() {
  return;
}

int16_t ACMCELT::InternalCreateDecoder() {
  return -1;
}

void ACMCELT::DestructDecoderSafe() {
  return;
}

void ACMCELT::InternalDestructEncoderInst(void* ) {
  return;
}

bool ACMCELT::IsTrueStereoCodec() {
  return true;
}

int16_t ACMCELT::SetBitRateSafe(const int32_t ) {
  return -1;
}

void ACMCELT::SplitStereoPacket(uint8_t* ,
                                int32_t* ) {}

#else  

ACMCELT::ACMCELT(int16_t codecID)
    : enc_inst_ptr_(NULL),
      dec_inst_ptr_(NULL),
      sampling_freq_(32000),  
      bitrate_(64000),  
      channels_(1),  
      dec_channels_(1) {  
  
  _codecID = codecID;

  return;
}

ACMCELT::~ACMCELT() {
  if (enc_inst_ptr_ != NULL) {
    WebRtcCelt_FreeEnc(enc_inst_ptr_);
    enc_inst_ptr_ = NULL;
  }
  if (dec_inst_ptr_ != NULL) {
    WebRtcCelt_FreeDec(dec_inst_ptr_);
    dec_inst_ptr_ = NULL;
  }
  return;
}

int16_t ACMCELT::InternalEncode(uint8_t* bitStream, int16_t* bitStreamLenByte) {
  *bitStreamLenByte = 0;

  
  *bitStreamLenByte = WebRtcCelt_Encode(enc_inst_ptr_,
                                        &_inAudio[_inAudioIxRead],
                                        bitStream);

  
  
  _inAudioIxRead += _frameLenSmpl * channels_;

  if (*bitStreamLenByte < 0) {
    
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
                 "InternalEncode: Encode error for Celt");
    *bitStreamLenByte = 0;
    return -1;
  }

  return *bitStreamLenByte;
}

int16_t ACMCELT::DecodeSafe(uint8_t* ,
                            int16_t ,
                            int16_t* ,
                            int16_t* ,
                            WebRtc_Word8* ) {
  return 0;
}

int16_t ACMCELT::InternalInitEncoder(WebRtcACMCodecParams* codecParams) {
  
  int16_t status = SetBitRateSafe((codecParams->codecInstant).rate);
  if (status < 0) {
    return -1;
  }

  
  if (codecParams->codecInstant.channels != channels_) {
    WebRtcCelt_FreeEnc(enc_inst_ptr_);
    enc_inst_ptr_ = NULL;
    
    channels_ = codecParams->codecInstant.channels;
    if (WebRtcCelt_CreateEnc(&enc_inst_ptr_, channels_) < 0) {
       return -1;
    }
  }

  
  if (WebRtcCelt_EncoderInit(enc_inst_ptr_, channels_, bitrate_) >= 0) {
    return 0;
  } else {
    return -1;
  }
}

int16_t ACMCELT::InternalInitDecoder(WebRtcACMCodecParams* codecParams) {
  
  if (codecParams->codecInstant.channels != dec_channels_) {
    WebRtcCelt_FreeDec(dec_inst_ptr_);
    dec_inst_ptr_ = NULL;
    
    dec_channels_ = codecParams->codecInstant.channels;
    if (WebRtcCelt_CreateDec(&dec_inst_ptr_, dec_channels_) < 0) {
       return -1;
    }
  }

  
  if (WebRtcCelt_DecoderInit(dec_inst_ptr_) < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
                 "InternalInitDecoder: init decoder failed for Celt.");
    return -1;
  }
  if (WebRtcCelt_DecoderInitSlave(dec_inst_ptr_) < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
                 "InternalInitDecoder: init decoder failed for Celt.");
    return -1;
  }
  return 0;
}

int32_t ACMCELT::CodecDef(WebRtcNetEQ_CodecDef& codecDef,
                          const CodecInst& codecInst) {
  if (!_decoderInitialized) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
                 "CodecDef: Decoder uninitialized for Celt");
    return -1;
  }

  
  
  
  
  if (codecInst.channels == 1) {
    SET_CODEC_PAR(codecDef, kDecoderCELT_32, codecInst.pltype, dec_inst_ptr_,
                  32000);
  } else {
    SET_CODEC_PAR(codecDef, kDecoderCELT_32_2ch, codecInst.pltype,
                  dec_inst_ptr_, 32000);
  }

  
  
  if (_isMaster) {
    SET_CELT_FUNCTIONS(codecDef);
  } else {
    SET_CELTSLAVE_FUNCTIONS(codecDef);
  }
  return 0;
}

ACMGenericCodec* ACMCELT::CreateInstance(void) {
  return NULL;
}

int16_t ACMCELT::InternalCreateEncoder() {
  if (WebRtcCelt_CreateEnc(&enc_inst_ptr_, _noChannels) < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
                 "InternalCreateEncoder: create encoder failed for Celt");
    return -1;
  }
  channels_ = _noChannels;
  return 0;
}

void ACMCELT::DestructEncoderSafe() {
  _encoderExist = false;
  _encoderInitialized = false;
  if (enc_inst_ptr_ != NULL) {
    WebRtcCelt_FreeEnc(enc_inst_ptr_);
    enc_inst_ptr_ = NULL;
  }
}

int16_t ACMCELT::InternalCreateDecoder() {
  if (WebRtcCelt_CreateDec(&dec_inst_ptr_, dec_channels_) < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
                 "InternalCreateDecoder: create decoder failed for Celt");
    return -1;
  }

  return 0;
}

void ACMCELT::DestructDecoderSafe() {
  _decoderExist = false;
  _decoderInitialized = false;
  if (dec_inst_ptr_ != NULL) {
    WebRtcCelt_FreeDec(dec_inst_ptr_);
    dec_inst_ptr_ = NULL;
  }
}

void ACMCELT::InternalDestructEncoderInst(void* ptrInst) {
  if (ptrInst != NULL) {
    WebRtcCelt_FreeEnc(static_cast<CELT_encinst_t*>(ptrInst));
  }
  return;
}

bool ACMCELT::IsTrueStereoCodec() {
  return true;
}

int16_t ACMCELT::SetBitRateSafe(const int32_t rate) {
  
  if ((rate >= 48000) && (rate <= 128000)) {
    
    bitrate_ = rate;

    
    if (WebRtcCelt_EncoderInit(enc_inst_ptr_, channels_, bitrate_) >= 0) {
      return 0;
    } else {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
                   "SetBitRateSafe: Failed to initiate Celt with rate %d",
                   rate);
      return -1;
    }
  } else {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
                 "SetBitRateSafe: Invalid rate Celt, %d", rate);
    return -1;
  }
}


void ACMCELT::SplitStereoPacket(uint8_t* payload, int32_t* payload_length) {
  
  assert(payload != NULL);
  assert(*payload_length > 0);

  
  memcpy(&payload[*payload_length], &payload[0],
         sizeof(uint8_t) * (*payload_length));
  
  *payload_length *= 2;
}

#endif

}  

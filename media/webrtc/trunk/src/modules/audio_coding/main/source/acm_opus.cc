









#include "acm_codec_database.h"
#include "acm_common_defs.h"
#include "acm_neteq.h"
#include "acm_opus.h"
#include "trace.h"
#include "webrtc_neteq.h"
#include "webrtc_neteq_help_macros.h"

#ifdef WEBRTC_CODEC_OPUS
#include "opus_interface.h"
#endif

namespace webrtc
{

#ifndef WEBRTC_CODEC_OPUS

ACMOpus::ACMOpus(WebRtc_Word16 )
    : _encoderInstPtr(NULL),
      _decoderInstPtr(NULL),
      _sampleFreq(0),
      _bitrate(0)
{
  return;
}

ACMOpus::~ACMOpus()
{
  return;
}

WebRtc_Word16 ACMOpus::InternalEncode(
    WebRtc_UWord8* ,
    WebRtc_Word16* )
{
  return -1;
}

WebRtc_Word16 ACMOpus::DecodeSafe(
    WebRtc_UWord8* ,
    WebRtc_Word16  ,
    WebRtc_Word16* ,
    WebRtc_Word16* ,
    WebRtc_Word8*  )
{
  return -1;
}

WebRtc_Word16 ACMOpus::InternalInitEncoder(
    WebRtcACMCodecParams* )
{
  return -1;
}

WebRtc_Word16 ACMOpus::InternalInitDecoder(
    WebRtcACMCodecParams* )
{
  return -1;
}

WebRtc_Word32 ACMOpus::CodecDef(
    WebRtcNetEQ_CodecDef& ,
    const CodecInst&      )
{
  return -1;
}

ACMGenericCodec* ACMOpus::CreateInstance(void)
{
  return NULL;
}

WebRtc_Word16 ACMOpus::InternalCreateEncoder()
{
  return -1;
}

void ACMOpus::DestructEncoderSafe()
{
  return;
}

WebRtc_Word16 ACMOpus::InternalCreateDecoder()
{
  return -1;
}

void ACMOpus::DestructDecoderSafe()
{
  return;
}

void ACMOpus::InternalDestructEncoderInst(void* )
{
  return;
}

WebRtc_Word16
ACMOpus::SetBitRateSafe(const WebRtc_Word32 )
{
  return -1;
}

#else     


ACMOpus::ACMOpus(WebRtc_Word16 codecID)
    : _encoderInstPtr(NULL),
      _decoderInstPtr(NULL),
      _sampleFreq(32000),  
      _bitrate(20000)      
{
  _codecID = codecID;
  
  _hasInternalDTX = false;

  if (_codecID != ACMCodecDB::kOpus)
  {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding,
                 _uniqueID, "Wrong codec id for Opus.");
    _sampleFreq = -1;
    _bitrate = -1;
  }
  return;
}

ACMOpus::~ACMOpus()
{
  if(_encoderInstPtr != NULL)
  {
    WebRtcOpus_EncoderFree(_encoderInstPtr);
    _encoderInstPtr = NULL;
  }
  if(_decoderInstPtr != NULL)
  {
    WebRtcOpus_DecoderFree(_decoderInstPtr);
    _decoderInstPtr = NULL;
  }
  return;
}

WebRtc_Word32 ACMOpus::Add10MsDataSafe(
    const WebRtc_UWord32 timestamp,
    const WebRtc_Word16* data,
    const WebRtc_UWord16 lengthSmpl,
    const WebRtc_UWord8 audioChannel) {
  return ACMGenericCodec::Add10MsDataSafe(timestamp*3/2, data, lengthSmpl,
                                          audioChannel);
}

WebRtc_Word16 ACMOpus::InternalEncode(
    WebRtc_UWord8* bitStream,
    WebRtc_Word16* bitStreamLenByte)
{
  
  *bitStreamLenByte = WebRtcOpus_Encode(_encoderInstPtr,
                                        &_inAudio[_inAudioIxRead],
                                        bitStream,
                                        MAX_PAYLOAD_SIZE_BYTE,
                                        _frameLenSmpl);
  
  if(*bitStreamLenByte < 0)
  {
    
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
        "InternalEncode: Encode error for Opus");
    *bitStreamLenByte = 0;
    return -1;
  }

  
  
  _inAudioIxRead += _frameLenSmpl;

  return *bitStreamLenByte;
}


WebRtc_Word16 ACMOpus::DecodeSafe(
    WebRtc_UWord8* bitStream,
    WebRtc_Word16  bitStreamLenByte,
    WebRtc_Word16* audio,
    WebRtc_Word16* audioSamples,
    WebRtc_Word8*  speechType)
{
  return 0;
}

WebRtc_Word16 ACMOpus::InternalInitEncoder(
    WebRtcACMCodecParams* codecParams)
{
  assert(_encoderInstPtr == NULL);
  return WebRtcOpus_EncoderCreate(&_encoderInstPtr,
      codecParams->codecInstant.channels);
}

WebRtc_Word16 ACMOpus::InternalInitDecoder(
    WebRtcACMCodecParams* codecParams)
{
  assert(_decoderInstPtr == NULL);
  return WebRtcOpus_DecoderCreate(&_decoderInstPtr,
      codecParams->codecInstant.plfreq,
      codecParams->codecInstant.channels);
}

WebRtc_Word32 ACMOpus::CodecDef(
    WebRtcNetEQ_CodecDef& codecDef,
    const CodecInst& codecInst)
{
  if (!_decoderInitialized)
  {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
        "CodeDef: Decoder uninitialized for Opus");
    return -1;
  }

  
  
  
  
  SET_CODEC_PAR((codecDef), kDecoderOpus, codecInst.pltype,
      _decoderInstPtr, codecInst.plfreq);
  SET_OPUS_FUNCTIONS((codecDef));
  return 0;
}

ACMGenericCodec* ACMOpus::CreateInstance(void)
{
  return NULL;
}


WebRtc_Word16 ACMOpus::InternalCreateEncoder()
{
  
  return 0;
}

void ACMOpus::DestructEncoderSafe()
{
  if (_encoderInstPtr) {
    WebRtcOpus_EncoderFree(_encoderInstPtr);
    _encoderInstPtr = NULL;
  }
}

WebRtc_Word16 ACMOpus::InternalCreateDecoder()
{
  
  return 0;
}

void ACMOpus::DestructDecoderSafe()
{
  _decoderInitialized = false;
  if (_decoderInstPtr) {
      WebRtcOpus_DecoderFree(_decoderInstPtr);
      _decoderInstPtr = NULL;
  }
}

void ACMOpus::InternalDestructEncoderInst(
    void* ptrInst)
{
  if(ptrInst != NULL) {
    WebRtcOpus_EncoderFree((OpusEncInst*)ptrInst);
  }
  return;
}

WebRtc_Word16
ACMOpus::SetBitRateSafe(
    const WebRtc_Word32 rate)
{
  if (rate < 6000 || rate > 510000) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
        "SetBitRateSafe: Invalid rate Opus");
    return -1;
  }

  _bitrate = rate;

  
  if (WebRtcOpus_SetBitRate(_encoderInstPtr, _bitrate) >= 0)
  {
    _encoderParams.codecInstant.rate = _bitrate;
    return 0;
  }

  return -1;
}

#endif 

} 

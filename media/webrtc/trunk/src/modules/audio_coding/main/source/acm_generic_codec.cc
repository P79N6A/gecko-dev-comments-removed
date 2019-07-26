









#include <assert.h>
#include <string.h>

#include "acm_codec_database.h"
#include "acm_common_defs.h"
#include "acm_generic_codec.h"
#include "acm_neteq.h"
#include "trace.h"
#include "webrtc_vad.h"
#include "webrtc_cng.h"

namespace webrtc
{


enum
{
    kMaxPLCParamsCNG = WEBRTC_CNG_MAX_LPC_ORDER,
    kNewCNGNumPLCParams = 8
};

#define ACM_SID_INTERVAL_MSEC 100




ACMGenericCodec::ACMGenericCodec()
    : _inAudioIxWrite(0),
      _inAudioIxRead(0),
      _inTimestampIxWrite(0),
      _inAudio(NULL),
      _inTimestamp(NULL),
      _frameLenSmpl(-1),  
      _noChannels(1),
      _codecID(-1),  
      _noMissedSamples(0),
      _encoderExist(false),
      _decoderExist(false),
      _encoderInitialized(false),
      _decoderInitialized(false),
      _registeredInNetEq(false),
      _hasInternalDTX(false),
      _ptrVADInst(NULL),
      _vadEnabled(false),
      _vadMode(VADNormal),
      _dtxEnabled(false),
      _ptrDTXInst(NULL),
      _numLPCParams(kNewCNGNumPLCParams),
      _sentCNPrevious(false),
      _isMaster(true),
      _netEqDecodeLock(NULL),
      _codecWrapperLock(*RWLockWrapper::CreateRWLock()),
      _lastEncodedTimestamp(0),
      _lastTimestamp(0xD87F3F9F),
      _isAudioBuffFresh(true),
      _uniqueID(0) {
  
  for (int i = 0; i < MAX_FRAME_SIZE_10MSEC; i++) {
    _vadLabel[i] = 0;
  }

  
  
  memset(&_encoderParams, 0, sizeof(WebRtcACMCodecParams));
  _encoderParams.codecInstant.pltype = -1;
  memset(&_decoderParams, 0, sizeof(WebRtcACMCodecParams));
  _decoderParams.codecInstant.pltype = -1;
}

ACMGenericCodec::~ACMGenericCodec()
{
    
    

    if(_ptrVADInst != NULL)
    {
        WebRtcVad_Free(_ptrVADInst);
        _ptrVADInst = NULL;
    }

    if (_inAudio != NULL)
    {
        delete [] _inAudio;
        _inAudio = NULL;
    }

    if (_inTimestamp != NULL)
    {
        delete [] _inTimestamp;
        _inTimestamp = NULL;
    }
    if(_ptrDTXInst != NULL)
    {
        WebRtcCng_FreeEnc(_ptrDTXInst);
        _ptrDTXInst = NULL;
    }
    delete &_codecWrapperLock;
}

WebRtc_Word32
ACMGenericCodec::Add10MsData(
    const WebRtc_UWord32 timestamp,
    const WebRtc_Word16* data,
    const WebRtc_UWord16 lengthSmpl,
    const WebRtc_UWord8  audioChannel)
{
    WriteLockScoped wl(_codecWrapperLock);
    return Add10MsDataSafe(timestamp, data, lengthSmpl, audioChannel);
}

WebRtc_Word32
ACMGenericCodec::Add10MsDataSafe(
    const WebRtc_UWord32 timestamp,
    const WebRtc_Word16* data,
    const WebRtc_UWord16 lengthSmpl,
    const WebRtc_UWord8  audioChannel)
{
    
    
    WebRtc_UWord16 plFreqHz;

    if(EncoderSampFreq(plFreqHz) < 0)
    {
        
        return -1;
    }

    
    if((plFreqHz / 100) != lengthSmpl)
    {
        
        
        return -1;
    }
    if(_lastTimestamp == timestamp)
    {
        
        if((_inAudioIxWrite >= lengthSmpl * audioChannel) &&
           (_inTimestampIxWrite > 0))
        {
            _inAudioIxWrite -= lengthSmpl * audioChannel;
            _inTimestampIxWrite--;
            WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceAudioCoding, _uniqueID,
                "Adding 10ms with previous timestamp, \
overwriting the previous 10ms");
        }
        else
        {
            WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceAudioCoding, _uniqueID,
                "Adding 10ms with previous timestamp, this will sound bad");
        }
    }

    _lastTimestamp = timestamp;

    if ((_inAudioIxWrite + lengthSmpl*audioChannel) > AUDIO_BUFFER_SIZE_W16)
    {
        
        WebRtc_Word16 missedSamples = _inAudioIxWrite + lengthSmpl*audioChannel -
            AUDIO_BUFFER_SIZE_W16;

        
        memmove(_inAudio, _inAudio + missedSamples,
            (AUDIO_BUFFER_SIZE_W16 - lengthSmpl*audioChannel)*sizeof(WebRtc_Word16));
        
        memcpy(_inAudio + (AUDIO_BUFFER_SIZE_W16 - lengthSmpl*audioChannel), data,
            lengthSmpl*audioChannel * sizeof(WebRtc_Word16));

        
        WebRtc_Word16 missed10MsecBlocks =
            (WebRtc_Word16)((missedSamples/audioChannel * 100) / plFreqHz);

        
        memmove(_inTimestamp, _inTimestamp + missed10MsecBlocks,
            (_inTimestampIxWrite - missed10MsecBlocks) * sizeof(WebRtc_UWord32));
        _inTimestampIxWrite -= missed10MsecBlocks;
        _inTimestamp[_inTimestampIxWrite] = timestamp;
        _inTimestampIxWrite++;

        
        _inAudioIxWrite = AUDIO_BUFFER_SIZE_W16;
        IncreaseNoMissedSamples(missedSamples);
        _isAudioBuffFresh = false;
        return -missedSamples;
    }
    memcpy(_inAudio + _inAudioIxWrite, data, lengthSmpl*audioChannel * sizeof(WebRtc_Word16));
    _inAudioIxWrite += lengthSmpl*audioChannel;

    assert(_inTimestampIxWrite < TIMESTAMP_BUFFER_SIZE_W32);
    assert(_inTimestampIxWrite >= 0);

    _inTimestamp[_inTimestampIxWrite] = timestamp;
    _inTimestampIxWrite++;
    _isAudioBuffFresh = false;
    return 0;
}

WebRtc_Word16
ACMGenericCodec::Encode(
    WebRtc_UWord8*         bitStream,
    WebRtc_Word16*         bitStreamLenByte,
    WebRtc_UWord32*        timeStamp,
    WebRtcACMEncodingType* encodingType)
{
    WriteLockScoped lockCodec(_codecWrapperLock);
    ReadLockScoped lockNetEq(*_netEqDecodeLock);
    return EncodeSafe(bitStream, bitStreamLenByte,
        timeStamp, encodingType);
}


WebRtc_Word16
ACMGenericCodec::EncodeSafe(
    WebRtc_UWord8*         bitStream,
    WebRtc_Word16*         bitStreamLenByte,
    WebRtc_UWord32*        timeStamp,
    WebRtcACMEncodingType* encodingType)
{
    
    
    if(_inAudioIxWrite < _frameLenSmpl*_noChannels)
    {
        
        *timeStamp = 0;
        *bitStreamLenByte = 0;
        
        *encodingType = kNoEncoding;
        return 0;
    }

    
    
    const WebRtc_Word16 myBasicCodingBlockSmpl =
        ACMCodecDB::BasicCodingBlock(_codecID);
    if((myBasicCodingBlockSmpl < 0) ||
        (!_encoderInitialized) ||
        (!_encoderExist))
    {
        
        *timeStamp = 0;
        *bitStreamLenByte = 0;
        *encodingType = kNoEncoding;
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
            "EncodeSafe: error, basic coding sample block is negative");
        return -1;
    }

    
    _inAudioIxRead = 0;
    *timeStamp = _inTimestamp[0];

    
    
    WebRtc_Word16 status = 0;
    WebRtc_Word16 dtxProcessedSamples = 0;

    status = ProcessFrameVADDTX(bitStream, bitStreamLenByte,
        &dtxProcessedSamples);

    if(status < 0)
    {
        *timeStamp = 0;
        *bitStreamLenByte = 0;
        *encodingType = kNoEncoding;
    }
    else
    {
        if(dtxProcessedSamples > 0)
        {
            
            
            

            
            
            _inAudioIxRead = dtxProcessedSamples;
            
            
            WebRtc_UWord16 sampFreqHz;
            EncoderSampFreq(sampFreqHz);
            if (sampFreqHz == 8000) {
                *encodingType = kPassiveDTXNB;
            } else if (sampFreqHz == 16000) {
                *encodingType = kPassiveDTXWB;
            } else if (sampFreqHz == 32000) {
                *encodingType = kPassiveDTXSWB;
            } else {
                status = -1;
                WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
                    "EncodeSafe: Wrong sampling frequency for DTX.");
            }

            
            if ((*bitStreamLenByte == 0)
                && (_sentCNPrevious || ((_inAudioIxWrite - _inAudioIxRead) <= 0))
                )
            {
                
                *bitStreamLenByte = 1;
                *encodingType = kNoEncoding;
            }
            _sentCNPrevious = true;
        }
        else
        {
            _sentCNPrevious = false;
            
            
            
            
            if(myBasicCodingBlockSmpl == 0)
            {
                
                
                status = InternalEncode(bitStream, bitStreamLenByte);

                if(status < 0)
                {
                    
                    
                    
                    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
                        "EncodeSafe: error in internalEncode");
                    *bitStreamLenByte = 0;
                    *encodingType = kNoEncoding;
                }
            }
            else
            {
                
                
                
                WebRtc_Word16 tmpBitStreamLenByte;

                
                *bitStreamLenByte = 0;
                bool done = false;
                while(!done)
                {
                    status = InternalEncode(&bitStream[*bitStreamLenByte],
                        &tmpBitStreamLenByte);
                    *bitStreamLenByte += tmpBitStreamLenByte;

                    
                    if((status < 0) ||
                        (*bitStreamLenByte > MAX_PAYLOAD_SIZE_BYTE))
                    {
                        
                        
                        
                        
                        *bitStreamLenByte = 0;
                        *encodingType = kNoEncoding;
                        
                        
                        status = -1;
                         WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding,
                            _uniqueID, "EncodeSafe: error in InternalEncode");
                        
                        break;
                    }

                    
                    
                    
                    done = _inAudioIxRead >= _frameLenSmpl;
                }
            }
            if(status >= 0)
            {
                *encodingType = (_vadLabel[0] == 1)?
                kActiveNormalEncoded:kPassiveNormalEncoded;
                
                if ((*bitStreamLenByte == 0) && ((_inAudioIxWrite - _inAudioIxRead) <= 0))
                {
                    
                    *bitStreamLenByte = 1;
                    *encodingType = kNoEncoding;
                }
            }
        }
    }

    
    
    WebRtc_UWord16 sampFreqHz;
    EncoderSampFreq(sampFreqHz);

    WebRtc_Word16 num10MsecBlocks =
            (WebRtc_Word16)((_inAudioIxRead/_noChannels * 100) / sampFreqHz);
    if(_inTimestampIxWrite > num10MsecBlocks)
    {
        memmove(_inTimestamp, _inTimestamp + num10MsecBlocks,
            (_inTimestampIxWrite - num10MsecBlocks) * sizeof(WebRtc_Word32));
    }
    _inTimestampIxWrite -= num10MsecBlocks;

    
    
    if(_inAudioIxRead < _inAudioIxWrite)
    {
        memmove(_inAudio, &_inAudio[_inAudioIxRead],
            (_inAudioIxWrite - _inAudioIxRead)*sizeof(WebRtc_Word16));
    }

    _inAudioIxWrite -= _inAudioIxRead;

    _inAudioIxRead = 0;
    _lastEncodedTimestamp = *timeStamp;
    return (status < 0) ? (-1):(*bitStreamLenByte);
}

WebRtc_Word16
ACMGenericCodec::Decode(
    WebRtc_UWord8* bitStream,
    WebRtc_Word16  bitStreamLenByte,
    WebRtc_Word16* audio,
    WebRtc_Word16* audioSamples,
    WebRtc_Word8*  speechType)
{
    WriteLockScoped wl(_codecWrapperLock);
    return DecodeSafe(bitStream, bitStreamLenByte, audio,
        audioSamples, speechType);
}

bool
ACMGenericCodec::EncoderInitialized()
{
    ReadLockScoped rl(_codecWrapperLock);
    return _encoderInitialized;
}

bool
ACMGenericCodec::DecoderInitialized()
{
    ReadLockScoped rl(_codecWrapperLock);
    return _decoderInitialized;
}


WebRtc_Word32
ACMGenericCodec::RegisterInNetEq(
    ACMNetEQ*   netEq,
    const CodecInst& codecInst)
{
    WebRtcNetEQ_CodecDef codecDef;
    WriteLockScoped wl(_codecWrapperLock);

    if(CodecDef(codecDef, codecInst) < 0)
    {
        
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
            "RegisterInNetEq: error, failed to register");
        _registeredInNetEq = false;
        return -1;
    }
    else
    {
        if(netEq->AddCodec(&codecDef, _isMaster) < 0)
        {
            WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
                "RegisterInNetEq: error, failed to add codec");
            _registeredInNetEq = false;
            return -1;
        }
        
        _registeredInNetEq = true;
        return 0;
    }
}

WebRtc_Word16
ACMGenericCodec::EncoderParams(
    WebRtcACMCodecParams* encParams)
{
    ReadLockScoped rl(_codecWrapperLock);
    return EncoderParamsSafe(encParams);
}

WebRtc_Word16
ACMGenericCodec::EncoderParamsSafe(
    WebRtcACMCodecParams* encParams)
{
    
    if(_encoderInitialized)
    {
        WebRtc_Word32 currentRate;
        memcpy(encParams, &_encoderParams, sizeof(WebRtcACMCodecParams));
        currentRate = encParams->codecInstant.rate;
        CurrentRate(currentRate);
        encParams->codecInstant.rate = currentRate;
        return 0;
    }
    else
    {
        encParams->codecInstant.plname[0] = '\0';
        encParams->codecInstant.pltype    = -1;
        encParams->codecInstant.pacsize   = 0;
        encParams->codecInstant.rate      = 0;
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
            "EncoderParamsSafe: error, encoder not initialized");
        return -1;
    }
}

bool
ACMGenericCodec::DecoderParams(
    WebRtcACMCodecParams* decParams,
    const WebRtc_UWord8   payloadType)
{
    ReadLockScoped rl(_codecWrapperLock);
    return DecoderParamsSafe(decParams, payloadType);
}

bool
ACMGenericCodec::DecoderParamsSafe(
    WebRtcACMCodecParams* decParams,
    const WebRtc_UWord8   payloadType)
{
    
    if(_decoderInitialized)
    {
        if(payloadType == _decoderParams.codecInstant.pltype)
        {
            memcpy(decParams, &_decoderParams, sizeof(WebRtcACMCodecParams));
            return true;
        }
    }

    decParams->codecInstant.plname[0] = '\0';
    decParams->codecInstant.pltype    = -1;
    decParams->codecInstant.pacsize   = 0;
    decParams->codecInstant.rate      = 0;
    return false;
}

WebRtc_Word16
ACMGenericCodec::ResetEncoder()
{
    WriteLockScoped lockCodec(_codecWrapperLock);
    ReadLockScoped lockNetEq(*_netEqDecodeLock);
    return ResetEncoderSafe();
}

WebRtc_Word16
ACMGenericCodec::ResetEncoderSafe()
{
    if(!_encoderExist || !_encoderInitialized)
    {
        
        return 0;
    }

    _inAudioIxWrite     = 0;
    _inAudioIxRead      = 0;
    _inTimestampIxWrite = 0;
    _noMissedSamples    = 0;
    _isAudioBuffFresh   = true;
    memset(_inAudio, 0, AUDIO_BUFFER_SIZE_W16 * sizeof(WebRtc_Word16));
    memset(_inTimestamp, 0, TIMESTAMP_BUFFER_SIZE_W32 * sizeof(WebRtc_Word32));

    
    bool enableVAD = _vadEnabled;
    bool enableDTX = _dtxEnabled;
    ACMVADMode mode = _vadMode;

    
    if(InternalResetEncoder() < 0)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
            "ResetEncoderSafe: error in reset encoder");
        return -1;
    }

    
    
    DisableDTX();
    DisableVAD();

    
    return SetVADSafe(enableDTX, enableVAD, mode);
}

WebRtc_Word16
ACMGenericCodec::InternalResetEncoder()
{
    
    
    
    
    
    
    
    return InternalInitEncoder(&_encoderParams);
}

WebRtc_Word16
ACMGenericCodec::InitEncoder(
    WebRtcACMCodecParams* codecParams,
    bool                  forceInitialization)
{
    WriteLockScoped lockCodec(_codecWrapperLock);
    ReadLockScoped lockNetEq(*_netEqDecodeLock);
    return InitEncoderSafe(codecParams, forceInitialization);
}

WebRtc_Word16
ACMGenericCodec::InitEncoderSafe(
    WebRtcACMCodecParams* codecParams,
    bool                  forceInitialization)
{
    
    int mirrorID;
    int codecNumber =
        ACMCodecDB::CodecNumber(&(codecParams->codecInstant), &mirrorID);

    if(codecNumber < 0)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
            "InitEncoderSafe: error, codec number negative");
        return -1;
    }
    
    if((_codecID >= 0) && (_codecID != codecNumber) && (_codecID != mirrorID))
    {
        
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
            "InitEncoderSafe: current codec is not the same as the one given by codecParams");
        return -1;
    }

    if(!CanChangeEncodingParam(codecParams->codecInstant))
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
            "InitEncoderSafe: cannot change encoding parameters");
        return -1;
    }

    if(_encoderInitialized && !forceInitialization)
    {
        
        return 0;
    }
    WebRtc_Word16 status;
    if(!_encoderExist)
    {
        _encoderInitialized = false;
        status = CreateEncoder();
        if(status < 0)
        {
            WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
            "InitEncoderSafe: cannot create encoder");
            return -1;
        }
        else
        {
            _encoderExist = true;
        }
    }
    _frameLenSmpl = (codecParams->codecInstant).pacsize;
    _noChannels = codecParams->codecInstant.channels;
    status = InternalInitEncoder(codecParams);
    if(status < 0)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
            "InitEncoderSafe: error in init encoder");
        _encoderInitialized = false;
        return -1;
    }
    else
    {
        memcpy(&_encoderParams, codecParams, sizeof(WebRtcACMCodecParams));
        _encoderInitialized = true;
        if(_inAudio == NULL)
        {
            _inAudio = new WebRtc_Word16[AUDIO_BUFFER_SIZE_W16];
            if(_inAudio == NULL)
            {
                return -1;
            }
            memset(_inAudio, 0, AUDIO_BUFFER_SIZE_W16 * sizeof(WebRtc_Word16));
        }
        if(_inTimestamp == NULL)
        {
            _inTimestamp = new WebRtc_UWord32[TIMESTAMP_BUFFER_SIZE_W32];
            if(_inTimestamp == NULL)
            {
                return -1;
            }
            memset(_inTimestamp, 0, sizeof(WebRtc_UWord32) *
                TIMESTAMP_BUFFER_SIZE_W32);
        }
        _isAudioBuffFresh = true;
    }
    status = SetVADSafe(codecParams->enableDTX, codecParams->enableVAD,
        codecParams->vadMode);

    return status;
}

bool
ACMGenericCodec::CanChangeEncodingParam(
    CodecInst& )
{
    return true;
}

WebRtc_Word16
ACMGenericCodec::InitDecoder(
    WebRtcACMCodecParams* codecParams,
    bool                  forceInitialization)
{
    WriteLockScoped lockCodc(_codecWrapperLock);
    WriteLockScoped lockNetEq(*_netEqDecodeLock);
    return InitDecoderSafe(codecParams, forceInitialization);
}

WebRtc_Word16
ACMGenericCodec::InitDecoderSafe(
    WebRtcACMCodecParams* codecParams,
    bool                  forceInitialization)
{
    int mirrorID;
    
    int codecNumber =
        ACMCodecDB::ReceiverCodecNumber(&codecParams->codecInstant, &mirrorID);

    if(codecNumber < 0)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
                    "InitDecoderSafe: error, invalid codec number");
        return -1;
    }
    
    if((_codecID >= 0) && (_codecID != codecNumber) && (_codecID != mirrorID))
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
                    "InitDecoderSafe: current codec is not the same as the one given "
                    "by codecParams");
        
        return -1;
    }


    if(_decoderInitialized && !forceInitialization)
    {
        
        return 0;
    }

    WebRtc_Word16 status;
    if(!_decoderExist)
    {
        _decoderInitialized = false;
        status = CreateDecoder();
        if(status < 0)
        {
            WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
                    "InitDecoderSafe: cannot create decoder");
            return -1;
        }
        else
        {
            _decoderExist = true;
        }
    }

    status = InternalInitDecoder(codecParams);
    if(status < 0)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
                "InitDecoderSafe: cannot init decoder");
        _decoderInitialized = false;
        return -1;
    }
    else
    {
        
        SaveDecoderParamSafe(codecParams);
        _decoderInitialized = true;
    }
    return 0;
}

WebRtc_Word16
ACMGenericCodec::ResetDecoder(WebRtc_Word16 payloadType)
{
    WriteLockScoped lockCodec(_codecWrapperLock);
    WriteLockScoped lockNetEq(*_netEqDecodeLock);
    return ResetDecoderSafe(payloadType);
}

WebRtc_Word16
ACMGenericCodec::ResetDecoderSafe(WebRtc_Word16 payloadType)
{
    WebRtcACMCodecParams decoderParams;
    if(!_decoderExist || !_decoderInitialized)
    {
        return 0;
    }
    
    
    
    
    DecoderParamsSafe(&decoderParams, (WebRtc_UWord8) payloadType);
    return InternalInitDecoder(&decoderParams);
}

void
ACMGenericCodec::ResetNoMissedSamples()
{
    WriteLockScoped cs(_codecWrapperLock);
    _noMissedSamples = 0;
}

void
ACMGenericCodec::IncreaseNoMissedSamples(
    const WebRtc_Word16 noSamples)
{
    _noMissedSamples += noSamples;
}


WebRtc_UWord32
ACMGenericCodec::NoMissedSamples() const
{
    ReadLockScoped cs(_codecWrapperLock);
    return _noMissedSamples;
}
void
ACMGenericCodec::DestructEncoder()
{
    WriteLockScoped wl(_codecWrapperLock);

    
    if(_ptrVADInst != NULL)
    {
        WebRtcVad_Free(_ptrVADInst);
        _ptrVADInst = NULL;
    }
    _vadEnabled = false;
    _vadMode = VADNormal;

    
    _dtxEnabled = false;
    if(_ptrDTXInst != NULL)
    {
        WebRtcCng_FreeEnc(_ptrDTXInst);
        _ptrDTXInst = NULL;
    }
    _numLPCParams = kNewCNGNumPLCParams;

    DestructEncoderSafe();
}

void
ACMGenericCodec::DestructDecoder()
{
    WriteLockScoped wl(_codecWrapperLock);
    _decoderParams.codecInstant.pltype = -1;
    DestructDecoderSafe();
}

WebRtc_Word16
ACMGenericCodec::SetBitRate(
    const WebRtc_Word32 bitRateBPS)
{
    WriteLockScoped wl(_codecWrapperLock);
    return SetBitRateSafe(bitRateBPS);
}

WebRtc_Word16
ACMGenericCodec::SetBitRateSafe(
    const WebRtc_Word32 bitRateBPS)
{
    
    
    
    CodecInst codecParams;
    if(ACMCodecDB::Codec(_codecID, &codecParams) < 0)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
            "SetBitRateSafe: error in ACMCodecDB::Codec");
        return -1;
    }
    if(codecParams.rate != bitRateBPS)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
            "SetBitRateSafe: rate value is not acceptable");
        return -1;
    }
    else
    {
        return 0;
    }
}

WebRtc_Word32
ACMGenericCodec::GetEstimatedBandwidth()
{
    WriteLockScoped wl(_codecWrapperLock);
    return GetEstimatedBandwidthSafe();
}

WebRtc_Word32
ACMGenericCodec::GetEstimatedBandwidthSafe()
{
    
    return -1;
}

WebRtc_Word32
ACMGenericCodec::SetEstimatedBandwidth(
    WebRtc_Word32 estimatedBandwidth)
{
    WriteLockScoped wl(_codecWrapperLock);
    return SetEstimatedBandwidthSafe(estimatedBandwidth);
}

WebRtc_Word32
ACMGenericCodec::SetEstimatedBandwidthSafe(
    WebRtc_Word32 )
{
    
    return -1;
}

WebRtc_Word32
ACMGenericCodec::GetRedPayload(
    WebRtc_UWord8* redPayload,
    WebRtc_Word16* payloadBytes)
{
    WriteLockScoped wl(_codecWrapperLock);
    return GetRedPayloadSafe(redPayload, payloadBytes);
}

WebRtc_Word32
ACMGenericCodec::GetRedPayloadSafe(
    WebRtc_UWord8* ,
    WebRtc_Word16* )
{
    return -1; 
}

WebRtc_Word16
ACMGenericCodec::CreateEncoder()
{
    WebRtc_Word16 status = 0;
    if(!_encoderExist)
    {
        status = InternalCreateEncoder();
        
        _encoderInitialized = false;
    }

    if(status < 0)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
            "CreateEncoder: error in internal create encoder");
        _encoderExist = false;
    }
    else
    {
        _encoderExist = true;
    }
    return status;
}

WebRtc_Word16
ACMGenericCodec::CreateDecoder()
{
    WebRtc_Word16 status = 0;
    if(!_decoderExist)
    {
        status = InternalCreateDecoder();
        
        _decoderInitialized = false;
    }

    if(status < 0)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
            "CreateDecoder: error in internal create decoder");
        _decoderExist = false;
    }
    else
    {
        _decoderExist = true;
    }
    return status;
}


void ACMGenericCodec::DestructEncoderInst(void* ptrInst)
{
    if(ptrInst != NULL)
    {
        WriteLockScoped lockCodec(_codecWrapperLock);
        ReadLockScoped lockNetEq(*_netEqDecodeLock);
        InternalDestructEncoderInst(ptrInst);
    }
}


WebRtc_Word16
ACMGenericCodec::AudioBuffer(
    WebRtcACMAudioBuff& audioBuff)
{
    ReadLockScoped cs(_codecWrapperLock);
    memcpy(audioBuff.inAudio, _inAudio,
        AUDIO_BUFFER_SIZE_W16 * sizeof(WebRtc_Word16));
    audioBuff.inAudioIxRead = _inAudioIxRead;
    audioBuff.inAudioIxWrite = _inAudioIxWrite;
    memcpy(audioBuff.inTimestamp, _inTimestamp,
        TIMESTAMP_BUFFER_SIZE_W32*sizeof(WebRtc_UWord32));
    audioBuff.inTimestampIxWrite = _inTimestampIxWrite;
    audioBuff.lastTimestamp = _lastTimestamp;
    return 0;
}


WebRtc_Word16
ACMGenericCodec::SetAudioBuffer(
    WebRtcACMAudioBuff& audioBuff)
{
    WriteLockScoped cs(_codecWrapperLock);
    memcpy(_inAudio, audioBuff.inAudio,
        AUDIO_BUFFER_SIZE_W16 * sizeof(WebRtc_Word16));
    _inAudioIxRead = audioBuff.inAudioIxRead;
    _inAudioIxWrite = audioBuff.inAudioIxWrite;
    memcpy(_inTimestamp, audioBuff.inTimestamp,
        TIMESTAMP_BUFFER_SIZE_W32*sizeof(WebRtc_UWord32));
    _inTimestampIxWrite = audioBuff.inTimestampIxWrite;
    _lastTimestamp = audioBuff.lastTimestamp;
    _isAudioBuffFresh = false;
    return 0;
}


WebRtc_UWord32
ACMGenericCodec::LastEncodedTimestamp() const
{
    ReadLockScoped cs(_codecWrapperLock);
    return _lastEncodedTimestamp;
}


WebRtc_UWord32
ACMGenericCodec::EarliestTimestamp() const
{
    ReadLockScoped cs(_codecWrapperLock);
    return _inTimestamp[0];
}


WebRtc_Word16
ACMGenericCodec::SetVAD(
    const bool       enableDTX,
    const bool       enableVAD,
    const ACMVADMode mode)
{
    WriteLockScoped cs(_codecWrapperLock);
    return SetVADSafe(enableDTX, enableVAD, mode);
}


WebRtc_Word16
ACMGenericCodec::SetVADSafe(
    const bool       enableDTX,
    const bool       enableVAD,
    const ACMVADMode mode)
{
    if(enableDTX)
    {
        
        if (!STR_CASE_CMP(_encoderParams.codecInstant.plname, "G729") && !_hasInternalDTX)
        {
            if (ACMGenericCodec::EnableDTX() < 0)
            {
            WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
                "SetVADSafe: error in enable DTX");
                return -1;
            }
        }
        else
        {
            if(EnableDTX() < 0)
            {
                WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
                    "SetVADSafe: error in enable DTX");
                return -1;
            }
        }

        if(_hasInternalDTX)
        {
            
            
            
            _vadMode = mode;
            return (enableVAD)? EnableVAD(mode):DisableVAD();
        }
        else
        {
            
            
            if(EnableVAD(mode) < 0)
            {
                
                if(!_vadEnabled)
                {
                    DisableDTX();
                }
                WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
                    "SetVADSafe: error in enable VAD");
                return -1;
            }

            
            
            if (enableVAD == false) {
                return 1;
            } else {
                return 0;
            }
        }
    }
    else
    {
        
        if (!STR_CASE_CMP(_encoderParams.codecInstant.plname, "G729") && !_hasInternalDTX)
        {
            ACMGenericCodec::DisableDTX();
        }
        else
        {
            DisableDTX();
        }
        return (enableVAD)? EnableVAD(mode):DisableVAD();
    }
}

WebRtc_Word16
ACMGenericCodec::EnableDTX()
{
    if(_hasInternalDTX)
    {
        
        
        
        return -1;
    }
    if(!_dtxEnabled)
    {
        if(WebRtcCng_CreateEnc(&_ptrDTXInst) < 0)
        {
            _ptrDTXInst = NULL;
            return -1;
        }
        WebRtc_UWord16 freqHz;
        EncoderSampFreq(freqHz);
        if(WebRtcCng_InitEnc(_ptrDTXInst, (WebRtc_Word16)freqHz,
            ACM_SID_INTERVAL_MSEC, _numLPCParams) < 0)
        {
            
            WebRtcCng_FreeEnc(_ptrDTXInst);
            _ptrDTXInst = NULL;
            return -1;
        }
        _dtxEnabled = true;
    }
    return 0;
}

WebRtc_Word16
ACMGenericCodec::DisableDTX()
{
    if(_hasInternalDTX)
    {
        
        
        
        return -1;
    }
    if(_ptrDTXInst != NULL)
    {
        WebRtcCng_FreeEnc(_ptrDTXInst);
        _ptrDTXInst = NULL;
    }
    _dtxEnabled = false;
    return 0;
}

WebRtc_Word16
ACMGenericCodec::EnableVAD(
    ACMVADMode mode)
{
    if((mode < VADNormal) || (mode > VADVeryAggr))
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
            "EnableVAD: error in VAD mode range");
        return -1;
    }

    if(!_vadEnabled)
    {
        if(WebRtcVad_Create(&_ptrVADInst) < 0)
        {
            _ptrVADInst = NULL;
            WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
                "EnableVAD: error in create VAD");
            return -1;
        }
        if(WebRtcVad_Init(_ptrVADInst) < 0)
        {
            WebRtcVad_Free(_ptrVADInst);
            _ptrVADInst = NULL;
            WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
                "EnableVAD: error in init VAD");
            return -1;
        }
    }

    
    if(WebRtcVad_set_mode(_ptrVADInst, mode) < 0)
    {
        
        
        
        
        if(!_vadEnabled)
        {
            
            
            WebRtcVad_Free(_ptrVADInst);
            _ptrVADInst = NULL;
        }
        WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceAudioCoding, _uniqueID,
            "EnableVAD: failed to set the VAD mode");
        return -1;
    }
    _vadMode = mode;
    _vadEnabled = true;
    return 0;
}

WebRtc_Word16
ACMGenericCodec::DisableVAD()
{
    if(_ptrVADInst != NULL)
    {
        WebRtcVad_Free(_ptrVADInst);
        _ptrVADInst = NULL;
    }
    _vadEnabled = false;
    return 0;
}

WebRtc_Word32
ACMGenericCodec::ReplaceInternalDTX(
    const bool replaceInternalDTX)
{
    WriteLockScoped cs(_codecWrapperLock);
    return ReplaceInternalDTXSafe(replaceInternalDTX);
}

WebRtc_Word32
ACMGenericCodec::ReplaceInternalDTXSafe(
    const bool )
{
    return -1;
}

WebRtc_Word32
ACMGenericCodec::IsInternalDTXReplaced(
    bool* internalDTXReplaced)
{
    WriteLockScoped cs(_codecWrapperLock);
    return IsInternalDTXReplacedSafe(internalDTXReplaced);
}

WebRtc_Word32
ACMGenericCodec::IsInternalDTXReplacedSafe(
    bool* internalDTXReplaced)
{
    *internalDTXReplaced = false;
    return 0;
}

WebRtc_Word16
ACMGenericCodec::ProcessFrameVADDTX(
    WebRtc_UWord8* bitStream,
    WebRtc_Word16* bitStreamLenByte,
    WebRtc_Word16* samplesProcessed)
{
    if(!_vadEnabled)
    {
        
        for(WebRtc_Word16 n = 0; n < MAX_FRAME_SIZE_10MSEC; n++)
        {
            _vadLabel[n] = 1;
        }
        *samplesProcessed = 0;
        return 0;
    }
    WebRtc_UWord16 freqHz;
    EncoderSampFreq(freqHz);

    
    WebRtc_Word16 samplesIn10Msec = (WebRtc_Word16)(freqHz / 100);
    WebRtc_Word32 frameLenMsec = (((WebRtc_Word32)_frameLenSmpl * 1000) / freqHz);
    WebRtc_Word16 status;

    
    WebRtc_Word16 audio[960];

    
    int noSamplesToProcess[2];
    if (frameLenMsec == 40)
    {
        
        noSamplesToProcess[0] = noSamplesToProcess[1] = 2*samplesIn10Msec;
    }
    else
    {
        
        
        noSamplesToProcess[0] = (frameLenMsec > 30)? 3*samplesIn10Msec : _frameLenSmpl;
        noSamplesToProcess[1] = _frameLenSmpl-noSamplesToProcess[0];
    }

    int offSet = 0;
    int loops = (noSamplesToProcess[1]>0) ? 2 : 1;
    for (int i=0; i<loops; i++) {
        
        if(_noChannels == 2) {
            for (int j=0; j<noSamplesToProcess[i]; j++) {
                audio[j] = (_inAudio[(offSet+j)*2]+_inAudio[(offSet+j)*2+1])/2;
        }
        offSet = noSamplesToProcess[0];
        } else {
            
            memcpy(audio, _inAudio, sizeof(WebRtc_Word16)*noSamplesToProcess[i]);
        }

        
        status = (WebRtc_Word16)WebRtcVad_Process(_ptrVADInst, (int)freqHz,
            audio, noSamplesToProcess[i]);

        _vadLabel[i] = status;

        if(status < 0)
        {
            
            *samplesProcessed += noSamplesToProcess[i];
            return -1;
        }

        
        
        
        
        *samplesProcessed = 0;
        if((status == 0) && (i==0) && _dtxEnabled && !_hasInternalDTX)
        {
            WebRtc_Word16 bitStreamLen;
            WebRtc_Word16 num10MsecFrames = noSamplesToProcess[i] / samplesIn10Msec;
            *bitStreamLenByte = 0;
            for(WebRtc_Word16 n = 0; n < num10MsecFrames; n++)
            {
                
                status = WebRtcCng_Encode(_ptrDTXInst, &audio[n*samplesIn10Msec],
                    samplesIn10Msec, bitStream, &bitStreamLen, 0);
                if (status < 0) {
                    return -1;
                }

                *samplesProcessed += samplesIn10Msec*_noChannels;

                
                *bitStreamLenByte += bitStreamLen;
            }


            
            if(*samplesProcessed != noSamplesToProcess[i]*_noChannels) {
                
                *samplesProcessed = 0;
            }
        }

        if(*samplesProcessed > 0)
        {
            
            
            break;
        }
    }

    return status;
}

WebRtc_Word16
ACMGenericCodec::SamplesLeftToEncode()
{
    ReadLockScoped rl(_codecWrapperLock);
    return (_frameLenSmpl <= _inAudioIxWrite)?
        0:(_frameLenSmpl - _inAudioIxWrite);
}

void
ACMGenericCodec::SetUniqueID(
    const WebRtc_UWord32 id)
{
    _uniqueID = id;
}

bool
ACMGenericCodec::IsAudioBufferFresh() const
{
    ReadLockScoped rl(_codecWrapperLock);
    return _isAudioBuffFresh;
}


WebRtc_Word16
ACMGenericCodec::EncoderSampFreq(WebRtc_UWord16& sampFreqHz)
{
    WebRtc_Word32 f;
    f = ACMCodecDB::CodecFreq(_codecID);
    if(f < 0)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
                     "EncoderSampFreq: codec frequency is negative");
        return -1;
    }
    else
    {
        sampFreqHz = (WebRtc_UWord16)f;
        return 0;
    }
}


WebRtc_Word32
ACMGenericCodec::ConfigISACBandwidthEstimator(
    const WebRtc_UWord8  ,
    const WebRtc_UWord16 ,
    const bool           )
{
    WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceAudioCoding, _uniqueID,
        "The send-codec is not iSAC, failed to config iSAC bandwidth estimator.");
    return -1;
}

WebRtc_Word32
ACMGenericCodec::SetISACMaxRate(
    const WebRtc_UWord32 )
{
    WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceAudioCoding, _uniqueID,
        "The send-codec is not iSAC, failed to set iSAC max rate.");
    return -1;
}

WebRtc_Word32
ACMGenericCodec::SetISACMaxPayloadSize(
    const WebRtc_UWord16 )
{
    WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceAudioCoding, _uniqueID,
        "The send-codec is not iSAC, failed to set iSAC max payload-size.");
    return -1;
}


void
ACMGenericCodec::SaveDecoderParam(
    const WebRtcACMCodecParams* codecParams)
{
    WriteLockScoped wl(_codecWrapperLock);
    SaveDecoderParamSafe(codecParams);
}


void
ACMGenericCodec::SaveDecoderParamSafe(
    const WebRtcACMCodecParams* codecParams)
{
    memcpy(&_decoderParams, codecParams, sizeof(WebRtcACMCodecParams));
}

WebRtc_Word16
ACMGenericCodec::UpdateEncoderSampFreq(
    WebRtc_UWord16 )
{
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
        "It is asked for a change in smapling frequency while the \
current send-codec supports only one sampling rate.");
    return -1;
}


void
ACMGenericCodec::SetIsMaster(
    bool isMaster)
{
    WriteLockScoped wl(_codecWrapperLock);
    _isMaster = isMaster;
}



WebRtc_Word16
ACMGenericCodec::REDPayloadISAC(
        const WebRtc_Word32  ,
        const WebRtc_Word16  ,
        WebRtc_UWord8*       ,
        WebRtc_Word16*       )
{
   WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
       "Error: REDPayloadISAC is an iSAC specific function");
    return -1;
}

} 











#include "acm_codec_database.h"
#include "acm_common_defs.h"
#include "acm_isac.h"
#include "acm_neteq.h"
#include "trace.h"
#include "webrtc_neteq.h"
#include "webrtc_neteq_help_macros.h"


#ifdef WEBRTC_CODEC_ISAC
    #include "acm_isac_macros.h"
    #include "isac.h"
#endif

#ifdef WEBRTC_CODEC_ISACFX
    #include "acm_isac_macros.h"
    #include "isacfix.h"
#endif

namespace webrtc
{



#if (defined(WEBRTC_CODEC_ISAC) || defined(WEBRTC_CODEC_ISACFX))
struct ACMISACInst
{
    ACM_ISAC_STRUCT *inst;
};
#endif

#define ISAC_MIN_RATE 10000
#define ISAC_MAX_RATE 56000
















#define ISAC_NUM_SUPPORTED_RATES 9
const WebRtc_UWord16 isacSuportedRates[ISAC_NUM_SUPPORTED_RATES] = {
    32000,    30000,    26000,   23000,   21000,
    19000,    17000,   15000,    12000};

const float isacScale[ISAC_NUM_SUPPORTED_RATES] = {
     1.0f,    0.8954f,  0.7178f, 0.6081f, 0.5445f,
     0.4875f, 0.4365f,  0.3908f, 0.3311f};


#define NR_ISAC_BANDWIDTHS 24
const WebRtc_Word32 isacRatesWB[NR_ISAC_BANDWIDTHS] =
{
    10000, 11100, 12300, 13700, 15200, 16900,
    18800, 20900, 23300, 25900, 28700, 31900,
    10100, 11200, 12400, 13800, 15300, 17000,
    18900, 21000, 23400, 26000, 28800, 32000};


const WebRtc_Word32 isacRatesSWB[NR_ISAC_BANDWIDTHS] =
{
    10000, 11000, 12400, 13800, 15300, 17000,
    18900, 21000, 23200, 25400, 27600, 29800,
    32000, 34100, 36300, 38500, 40700, 42900,
    45100, 47300, 49500, 51700, 53900, 56000,
};

#if (!defined(WEBRTC_CODEC_ISAC) && !defined(WEBRTC_CODEC_ISACFX))

ACMISAC::ACMISAC(WebRtc_Word16 )
    : _codecInstPtr(NULL),
      _isEncInitialized(false),
      _isacCodingMode(CHANNEL_INDEPENDENT),
      _enforceFrameSize(false),
      _isacCurrentBN(32000),
      _samplesIn10MsAudio(160) {  
  
  memset(&_decoderParams32kHz, 0, sizeof(WebRtcACMCodecParams));
  _decoderParams32kHz.codecInstant.pltype = -1;

  return;
}


ACMISAC::~ACMISAC()
{
    return;
}


ACMGenericCodec*
ACMISAC::CreateInstance(void)
{
    return NULL;
}


WebRtc_Word16
ACMISAC::InternalEncode(
    WebRtc_UWord8* ,
    WebRtc_Word16* )
{
    return -1;
}


WebRtc_Word16
ACMISAC::DecodeSafe(
    WebRtc_UWord8* ,
    WebRtc_Word16  ,
    WebRtc_Word16* ,
    WebRtc_Word16* ,
    WebRtc_Word8*  )
{
    return 0;
}


WebRtc_Word16
ACMISAC::InternalInitEncoder(
    WebRtcACMCodecParams* )
{
    return -1;
}


WebRtc_Word16
ACMISAC::InternalInitDecoder(
    WebRtcACMCodecParams* )
{
    return -1;
}


WebRtc_Word16
ACMISAC::InternalCreateDecoder()
{
    return -1;
}


void
ACMISAC::DestructDecoderSafe()
{
    return;
}


WebRtc_Word16
ACMISAC::InternalCreateEncoder()
{
    return -1;
}


void
ACMISAC::DestructEncoderSafe()
{
    return;
}


WebRtc_Word32
ACMISAC::CodecDef(
    WebRtcNetEQ_CodecDef& ,
    const CodecInst&      )
{
    return -1;
}


void
ACMISAC::InternalDestructEncoderInst(
    void* )
{
    return;
}

WebRtc_Word16
ACMISAC::DeliverCachedIsacData(
    WebRtc_UWord8*         ,
    WebRtc_Word16*         ,
    WebRtc_UWord32*        ,
    WebRtcACMEncodingType* ,
    const WebRtc_UWord16   ,
    const WebRtc_UWord8    )
{
    return -1;
}


WebRtc_Word16
ACMISAC::Transcode(
    WebRtc_UWord8* ,
    WebRtc_Word16* ,
    WebRtc_Word16  ,
    WebRtc_Word32  ,
    bool           )
{
    return -1;
}

WebRtc_Word16
ACMISAC::SetBitRateSafe(
    WebRtc_Word32 )
{
    return -1;
}

WebRtc_Word32
ACMISAC::GetEstimatedBandwidthSafe()
{
    return -1;
}

WebRtc_Word32
ACMISAC::SetEstimatedBandwidthSafe(
    WebRtc_Word32 )
{
    return -1;
}

WebRtc_Word32
ACMISAC::GetRedPayloadSafe(
    WebRtc_UWord8* ,
    WebRtc_Word16* )
{
    return -1;
}

WebRtc_Word16
ACMISAC::UpdateDecoderSampFreq(
    WebRtc_Word16 )
{
    return -1;
}


WebRtc_Word16
ACMISAC::UpdateEncoderSampFreq(
    WebRtc_UWord16 )
{
    return -1;
}

WebRtc_Word16
ACMISAC::EncoderSampFreq(
        WebRtc_UWord16& )
{
    return -1;
}

WebRtc_Word32
ACMISAC::ConfigISACBandwidthEstimator(
    const WebRtc_UWord8  ,
    const WebRtc_UWord16 ,
    const bool           )
{
    return -1;
}

WebRtc_Word32
ACMISAC::SetISACMaxPayloadSize(
    const WebRtc_UWord16 )
{
    return -1;
}

WebRtc_Word32
ACMISAC::SetISACMaxRate(
        const WebRtc_UWord32 )
{
    return -1;
}


void
ACMISAC::UpdateFrameLen()
{
    return;
}

void
ACMISAC::CurrentRate(
    WebRtc_Word32& )
{
    return;
}

bool
ACMISAC::DecoderParamsSafe(
    WebRtcACMCodecParams* ,
    const WebRtc_UWord8   )
{
    return false;
}

void
ACMISAC::SaveDecoderParamSafe(
    const WebRtcACMCodecParams* )
{
    return;
}

WebRtc_Word16
ACMISAC::REDPayloadISAC(
        const WebRtc_Word32 ,
        const WebRtc_Word16 ,
        WebRtc_UWord8*      ,
        WebRtc_Word16*      )
{
    return -1;
}


#else     



#ifdef WEBRTC_CODEC_ISACFX

enum IsacSamplingRate
{
    kIsacWideband = 16,
    kIsacSuperWideband = 32
};

static float
ACMISACFixTranscodingScale(
    WebRtc_UWord16 rate)
{
    
    
    float  scale = -1;
    for(WebRtc_Word16 n=0; n < ISAC_NUM_SUPPORTED_RATES; n++)
    {
        if(rate >= isacSuportedRates[n])
        {
            scale = isacScale[n];
            break;
        }
    }
    return scale;
}

static void
ACMISACFixGetSendBitrate(
    ACM_ISAC_STRUCT* inst,
    WebRtc_Word32*   bottleNeck)
{
    *bottleNeck = WebRtcIsacfix_GetUplinkBw(inst);
}

static WebRtc_Word16
ACMISACFixGetNewBitstream(
    ACM_ISAC_STRUCT* inst,
    WebRtc_Word16    BWEIndex,
    WebRtc_Word16    ,
    WebRtc_Word32    rate,
    WebRtc_Word16*   bitStream,
    bool             isRED)
{
    if (isRED)
    {
        
        return -1;
    }
    float scale = ACMISACFixTranscodingScale((WebRtc_UWord16)rate);
    return WebRtcIsacfix_GetNewBitStream(inst, BWEIndex, scale, bitStream);
}


static WebRtc_Word16
ACMISACFixGetSendBWE(
    ACM_ISAC_STRUCT* inst,
    WebRtc_Word16*   rateIndex,
    WebRtc_Word16*   )
{
    WebRtc_Word16 localRateIndex;
    WebRtc_Word16 status = WebRtcIsacfix_GetDownLinkBwIndex(inst, &localRateIndex);
    if(status < 0)
    {
        return -1;
    }
    else
    {
        *rateIndex = localRateIndex;
        return 0;
    }
}

static WebRtc_Word16
ACMISACFixControlBWE(
    ACM_ISAC_STRUCT* inst,
    WebRtc_Word32    rateBPS,
    WebRtc_Word16    frameSizeMs,
    WebRtc_Word16    enforceFrameSize)
{
    return WebRtcIsacfix_ControlBwe(inst, (WebRtc_Word16)rateBPS,
        frameSizeMs, enforceFrameSize);
}

static WebRtc_Word16
ACMISACFixControl(
    ACM_ISAC_STRUCT* inst,
    WebRtc_Word32    rateBPS,
    WebRtc_Word16    frameSizeMs)
{
    return WebRtcIsacfix_Control(inst, (WebRtc_Word16)rateBPS,
        frameSizeMs);
}

static IsacSamplingRate
ACMISACFixGetEncSampRate(
    ACM_ISAC_STRUCT* )
{
    return kIsacWideband;
}


static IsacSamplingRate
ACMISACFixGetDecSampRate(
    ACM_ISAC_STRUCT* )
{
    return kIsacWideband;
}

#endif






ACMISAC::ACMISAC(WebRtc_Word16 codecID)
    : _isEncInitialized(false),
      _isacCodingMode(CHANNEL_INDEPENDENT),
      _enforceFrameSize(false),
      _isacCurrentBN(32000),
      _samplesIn10MsAudio(160) {  
  _codecID = codecID;

  
  _codecInstPtr = new ACMISACInst;
  if (_codecInstPtr == NULL) {
    return;
  }
  _codecInstPtr->inst = NULL;

  
  memset(&_decoderParams32kHz, 0, sizeof(WebRtcACMCodecParams));
  _decoderParams32kHz.codecInstant.pltype = -1;

  
  
  
  _decoderParams.codecInstant.pltype = -1;
}


ACMISAC::~ACMISAC()
{
    if (_codecInstPtr != NULL)
    {
        if(_codecInstPtr->inst != NULL)
        {
            ACM_ISAC_FREE(_codecInstPtr->inst);
            _codecInstPtr->inst = NULL;
        }
        delete _codecInstPtr;
        _codecInstPtr = NULL;
    }
    return;
}


ACMGenericCodec*
ACMISAC::CreateInstance(void)
{
    return NULL;
}


WebRtc_Word16
ACMISAC::InternalEncode(
    WebRtc_UWord8* bitstream,
    WebRtc_Word16* bitStreamLenByte)
{
    
    
    
    
    
    
    

    if (_codecInstPtr == NULL)
    {
        return -1;
    }
    *bitStreamLenByte = 0;
    while((*bitStreamLenByte == 0) && (_inAudioIxRead < _frameLenSmpl))
    {
        if(_inAudioIxRead > _inAudioIxWrite)
        {
            
            WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
                "The actual fram-size of iSAC appears to be larger that expected. All audio \
pushed in but no bit-stream is generated.");
            return -1;
        }
        *bitStreamLenByte = ACM_ISAC_ENCODE(_codecInstPtr->inst,
            &_inAudio[_inAudioIxRead], (WebRtc_Word16*)bitstream);
        
        
        _inAudioIxRead += _samplesIn10MsAudio;
    }
    if(*bitStreamLenByte == 0)
    {
        WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceAudioCoding, _uniqueID,
            "ISAC Has encoded the whole frame but no bit-stream is generated.");
    }

    
    
    
    if((*bitStreamLenByte > 0) && (_isacCodingMode == ADAPTIVE))
    {
        
        ACM_ISAC_GETSENDBITRATE(_codecInstPtr->inst, &_isacCurrentBN);
    }
    UpdateFrameLen();
    return *bitStreamLenByte;
}


WebRtc_Word16
ACMISAC::DecodeSafe(
    WebRtc_UWord8* ,
    WebRtc_Word16  ,
    WebRtc_Word16* ,
    WebRtc_Word16* ,
    WebRtc_Word8*  )
{
    return 0;
}


WebRtc_Word16
ACMISAC::InternalInitEncoder(
    WebRtcACMCodecParams* codecParams)
{
    
    if(codecParams->codecInstant.rate == -1)
    {
        _isacCodingMode = ADAPTIVE;
    }

    
    else if((codecParams->codecInstant.rate >= ISAC_MIN_RATE) &&
        (codecParams->codecInstant.rate <= ISAC_MAX_RATE))
    {
        _isacCodingMode = CHANNEL_INDEPENDENT;
        _isacCurrentBN = codecParams->codecInstant.rate;
    }
    else
    {
        return -1;
    }

    
    if(UpdateEncoderSampFreq((WebRtc_UWord16)codecParams->codecInstant.plfreq) < 0)
    {
        return -1;
    }
    if(ACM_ISAC_ENCODERINIT(_codecInstPtr->inst, _isacCodingMode) < 0)
    {
        return -1;
    }

    
    
    if(_isacCodingMode == CHANNEL_INDEPENDENT)
    {
        if(ACM_ISAC_CONTROL(_codecInstPtr->inst,
            codecParams->codecInstant.rate,
            codecParams->codecInstant.pacsize /
            (codecParams->codecInstant.plfreq / 1000)) < 0)
        {
            return -1;
        }
    }
    else
    {
        
        
        ACM_ISAC_GETSENDBITRATE(
            _codecInstPtr->inst, &_isacCurrentBN);
    }
    _frameLenSmpl = ACM_ISAC_GETNEWFRAMELEN(_codecInstPtr->inst);
    return 0;
}

WebRtc_Word16
ACMISAC::InternalInitDecoder(
    WebRtcACMCodecParams*  codecParams)
{
    if (_codecInstPtr == NULL)
    {
        return -1;
    }

    
    if(codecParams->codecInstant.plfreq == 32000)
    {
        UpdateDecoderSampFreq(ACMCodecDB::kISACSWB);
    }
    else
    {
        UpdateDecoderSampFreq(ACMCodecDB::kISAC);
    }

    
    
    
    
    if(!_encoderInitialized)
    {
        
        
        codecParams->codecInstant.rate = kIsacWbDefaultRate;
        codecParams->codecInstant.pacsize = kIsacPacSize960;
        if(InternalInitEncoder(codecParams) < 0)
        {
            return -1;
        }
        _encoderInitialized = true;
    }

    return ACM_ISAC_DECODERINIT(_codecInstPtr->inst);
}


WebRtc_Word16
ACMISAC::InternalCreateDecoder()
{
    if (_codecInstPtr == NULL)
    {
        return -1;
    }
    WebRtc_Word16 status = ACM_ISAC_CREATE (&(_codecInstPtr->inst));

    
    _encoderInitialized = false;
    if(status < 0)
    {
        _encoderExist = false;
    }
    else
    {
        _encoderExist = true;
    }
    return status;
}


void
ACMISAC::DestructDecoderSafe()
{
    
    _decoderInitialized = false;
    return;
}


WebRtc_Word16
ACMISAC::InternalCreateEncoder()
{
    if (_codecInstPtr == NULL)
    {
        return -1;
    }
    WebRtc_Word16 status = ACM_ISAC_CREATE(&(_codecInstPtr->inst));

    
    _decoderInitialized = false;
    if(status < 0)
    {
        _decoderExist = false;
    }
    else
    {
        _decoderExist = true;
    }
    return status;
}


void
ACMISAC::DestructEncoderSafe()
{
    
    _encoderInitialized = false;
    return;
}


WebRtc_Word32
ACMISAC::CodecDef(
    WebRtcNetEQ_CodecDef& codecDef,
    const CodecInst&      codecInst)
{
    
    if (_codecInstPtr == NULL)
    {
        return -1;
    }
    if (!_decoderInitialized || !_decoderExist)
    {
        
        
        return -1;
    }
    
    
    
    
    if(codecInst.plfreq == 16000)
    {
        SET_CODEC_PAR((codecDef), kDecoderISAC, codecInst.pltype,
            _codecInstPtr->inst, 16000);
#ifdef WEBRTC_CODEC_ISAC
        SET_ISAC_FUNCTIONS((codecDef));
#else
        SET_ISACfix_FUNCTIONS((codecDef));
#endif
    }
    else
    {
#ifdef WEBRTC_CODEC_ISAC
        SET_CODEC_PAR((codecDef), kDecoderISACswb, codecInst.pltype,
            _codecInstPtr->inst, 32000);
        SET_ISACSWB_FUNCTIONS((codecDef));
#else
        return -1;
#endif
    }

    return 0;
}


void
ACMISAC::InternalDestructEncoderInst(
    void* ptrInst)
{
    if(ptrInst != NULL)
    {
        ACM_ISAC_FREE((ACM_ISAC_STRUCT *)ptrInst);
    }
    return;
}

WebRtc_Word16
ACMISAC::Transcode(
    WebRtc_UWord8* bitStream,
    WebRtc_Word16* bitStreamLenByte,
    WebRtc_Word16  qBWE,
    WebRtc_Word32  rate,
    bool           isRED)
{
    WebRtc_Word16 jitterInfo = 0;
    
    
    if (_codecInstPtr == NULL)
    {
        return -1;
    }

    *bitStreamLenByte = ACM_ISAC_GETNEWBITSTREAM(_codecInstPtr->inst,
        qBWE, jitterInfo, rate, (WebRtc_Word16*)bitStream, (isRED)? 1:0);

    if(*bitStreamLenByte < 0)
    {
        
        *bitStreamLenByte = 0;
        return -1;
    }
    else
    {
        return *bitStreamLenByte;
    }
}

WebRtc_Word16
ACMISAC::SetBitRateSafe(
    WebRtc_Word32 bitRate)
{
    if (_codecInstPtr == NULL)
    {
        return -1;
    }
    WebRtc_UWord16 encoderSampFreq;
    EncoderSampFreq(encoderSampFreq);
    bool reinit = false;
    
    if(bitRate == -1)
    {
        
        
        if(_isacCodingMode != ADAPTIVE)
        {
            
            
            _isacCodingMode = ADAPTIVE;
            reinit = true;
        }
    }
    
    else if((bitRate >= ISAC_MIN_RATE) &&
        (bitRate <= ISAC_MAX_RATE))
    {
        
        if(_isacCodingMode != CHANNEL_INDEPENDENT)
        {
            
            
            _isacCodingMode = CHANNEL_INDEPENDENT;
            reinit = true;
        }
        
        _isacCurrentBN = (WebRtc_UWord16)bitRate;
    }
    else
    {
        
        return -1;
    }

    WebRtc_Word16 status = 0;
    if(reinit)
    {
        
        if(ACM_ISAC_ENCODERINIT(_codecInstPtr->inst, _isacCodingMode) < 0)
        {
            
            return -1;
        }
    }
    if(_isacCodingMode == CHANNEL_INDEPENDENT)
    {

        status = ACM_ISAC_CONTROL(_codecInstPtr->inst, _isacCurrentBN,
            (encoderSampFreq == 32000)? 30:(_frameLenSmpl / 16));
        if(status < 0)
        {
            status = -1;
        }
    }

    
    _encoderParams.codecInstant.rate = bitRate;

    UpdateFrameLen();
    return status;
}


WebRtc_Word32
ACMISAC::GetEstimatedBandwidthSafe()
{
    WebRtc_Word16 bandwidthIndex = 0;
    WebRtc_Word16 delayIndex = 0;
    IsacSamplingRate sampRate;

    
    ACM_ISAC_GETSENDBWE(_codecInstPtr->inst, &bandwidthIndex, &delayIndex);

    
    if ((bandwidthIndex < 0) || (bandwidthIndex >= NR_ISAC_BANDWIDTHS))
    {
        return -1;
    }

    
    sampRate = ACM_ISAC_GETDECSAMPRATE(_codecInstPtr->inst);
    if(sampRate == kIsacWideband)
    {
        return isacRatesWB[bandwidthIndex];
    }
    else
    {
        return isacRatesSWB[bandwidthIndex];
    }
}

WebRtc_Word32
ACMISAC::SetEstimatedBandwidthSafe(
    WebRtc_Word32 estimatedBandwidth)
{
    IsacSamplingRate sampRate;
    WebRtc_Word16 bandwidthIndex;

    
    sampRate = ACM_ISAC_GETENCSAMPRATE(_codecInstPtr->inst);

    if(sampRate == kIsacWideband)
    {
        

        bandwidthIndex = NR_ISAC_BANDWIDTHS/2 - 1;
        for (int i=0; i<(NR_ISAC_BANDWIDTHS/2); i++)
        {
            if (estimatedBandwidth == isacRatesWB[i])
            {
                bandwidthIndex = i;
                break;
            } else if (estimatedBandwidth == isacRatesWB[i+NR_ISAC_BANDWIDTHS/2])
            {
                bandwidthIndex = i + NR_ISAC_BANDWIDTHS/2;
                break;
            } else if (estimatedBandwidth < isacRatesWB[i])
            {
                bandwidthIndex = i;
                break;
             }
        }
    }
    else
    {
        
        bandwidthIndex = NR_ISAC_BANDWIDTHS - 1;
        for (int i=0; i<NR_ISAC_BANDWIDTHS; i++)
        {
            if(estimatedBandwidth <= isacRatesSWB[i])
            {
                bandwidthIndex = i;
                break;
            }
        }
    }

    
    ACM_ISAC_SETBWE(_codecInstPtr->inst, bandwidthIndex);

    return 0;
}

WebRtc_Word32
ACMISAC::GetRedPayloadSafe(
#if (!defined(WEBRTC_CODEC_ISAC))
    WebRtc_UWord8* ,
    WebRtc_Word16* )
{
    return -1;
#else
    WebRtc_UWord8* redPayload,
    WebRtc_Word16* payloadBytes)
{

    WebRtc_Word16 bytes = WebRtcIsac_GetRedPayload(_codecInstPtr->inst, (WebRtc_Word16*)redPayload);
    if (bytes < 0)
    {
        return -1;
    }
    *payloadBytes = bytes;
    return 0;
#endif
}

WebRtc_Word16
ACMISAC::UpdateDecoderSampFreq(
#ifdef WEBRTC_CODEC_ISAC
    WebRtc_Word16 codecId)
{
    if(ACMCodecDB::kISAC == codecId)
    {
        return WebRtcIsac_SetDecSampRate(_codecInstPtr->inst, kIsacWideband);
    }
    else if(ACMCodecDB::kISACSWB == codecId)
    {
        return WebRtcIsac_SetDecSampRate(_codecInstPtr->inst, kIsacSuperWideband);
    }
    else
    {
        return -1;
    }

#else
    WebRtc_Word16 /* codecId */)
{
    return 0;
#endif
}


WebRtc_Word16
ACMISAC::UpdateEncoderSampFreq(
#ifdef WEBRTC_CODEC_ISAC
    WebRtc_UWord16 encoderSampFreqHz)
{
    WebRtc_UWord16 currentSampRateHz;
    EncoderSampFreq(currentSampRateHz);

    if(currentSampRateHz != encoderSampFreqHz)
    {
        if((encoderSampFreqHz != 16000) && (encoderSampFreqHz != 32000))
        {
            return -1;
        }
        else
        {
            _inAudioIxRead = 0;
            _inAudioIxWrite = 0;
            _inTimestampIxWrite = 0;
            if(encoderSampFreqHz == 16000)
            {
                if(WebRtcIsac_SetEncSampRate(_codecInstPtr->inst, kIsacWideband) < 0)
                {
                    return -1;
                }
                _samplesIn10MsAudio = 160;
            }
            else
            {

                if(WebRtcIsac_SetEncSampRate(_codecInstPtr->inst, kIsacSuperWideband) < 0)
                {
                    return -1;
                }
                _samplesIn10MsAudio = 320;
            }
            _frameLenSmpl = ACM_ISAC_GETNEWFRAMELEN(_codecInstPtr->inst);
            _encoderParams.codecInstant.pacsize = _frameLenSmpl;
            _encoderParams.codecInstant.plfreq = encoderSampFreqHz;
            return 0;
        }
    }
#else
    WebRtc_UWord16 /* codecId */)
{
#endif
    return 0;
}

WebRtc_Word16
ACMISAC::EncoderSampFreq(
    WebRtc_UWord16& sampFreqHz)
{
    IsacSamplingRate sampRate;
    sampRate = ACM_ISAC_GETENCSAMPRATE(_codecInstPtr->inst);
    if(sampRate == kIsacSuperWideband)
    {
        sampFreqHz = 32000;
    }
    else
    {
        sampFreqHz = 16000;
    }
    return 0;
}

WebRtc_Word32
ACMISAC::ConfigISACBandwidthEstimator(
    const WebRtc_UWord8  initFrameSizeMsec,
    const WebRtc_UWord16 initRateBitPerSec,
    const bool           enforceFrameSize)
{
    WebRtc_Word16 status;
    {
        WebRtc_UWord16 sampFreqHz;
        EncoderSampFreq(sampFreqHz);
        
        
        
        if(sampFreqHz == 32000)
        {
            status = ACM_ISAC_CONTROL_BWE(_codecInstPtr->inst,
                initRateBitPerSec, 30, 1);
        }
        else
        {
            status = ACM_ISAC_CONTROL_BWE(_codecInstPtr->inst,
                initRateBitPerSec, initFrameSizeMsec, enforceFrameSize? 1:0);
        }
    }
    if(status < 0)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _uniqueID,
            "Coutn't config iSAC BWE.");
        return -1;
    }
    UpdateFrameLen();
    ACM_ISAC_GETSENDBITRATE(_codecInstPtr->inst, &_isacCurrentBN);
    return 0;
}

WebRtc_Word32
ACMISAC::SetISACMaxPayloadSize(
    const WebRtc_UWord16 maxPayloadLenBytes)
{
    return ACM_ISAC_SETMAXPAYLOADSIZE(_codecInstPtr->inst, maxPayloadLenBytes);
}

WebRtc_Word32
ACMISAC::SetISACMaxRate(
    const WebRtc_UWord32 maxRateBitPerSec)
{
    return ACM_ISAC_SETMAXRATE(_codecInstPtr->inst, maxRateBitPerSec);
}


void
ACMISAC::UpdateFrameLen()
{
    _frameLenSmpl = ACM_ISAC_GETNEWFRAMELEN(_codecInstPtr->inst);
    _encoderParams.codecInstant.pacsize = _frameLenSmpl;
}

void
ACMISAC::CurrentRate(WebRtc_Word32& rateBitPerSec)
{
    if(_isacCodingMode == ADAPTIVE)
    {
        ACM_ISAC_GETSENDBITRATE(_codecInstPtr->inst, &rateBitPerSec);
    }
}


bool
ACMISAC::DecoderParamsSafe(
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
        if(payloadType == _decoderParams32kHz.codecInstant.pltype)
        {
            memcpy(decParams, &_decoderParams32kHz,
                sizeof(WebRtcACMCodecParams));
            return true;
        }
    }
    return false;
}

void
ACMISAC::SaveDecoderParamSafe(
    const WebRtcACMCodecParams* codecParams)
{
    
    if(codecParams->codecInstant.plfreq == 32000)
    {
        memcpy(&_decoderParams32kHz, codecParams, sizeof(WebRtcACMCodecParams));
    }
    else
    {
        memcpy(&_decoderParams, codecParams, sizeof(WebRtcACMCodecParams));
    }
}


WebRtc_Word16
ACMISAC::REDPayloadISAC(
    const WebRtc_Word32  isacRate,
    const WebRtc_Word16  isacBwEstimate,
    WebRtc_UWord8*       payload,
    WebRtc_Word16*       payloadLenBytes)
{
    WebRtc_Word16 status;
    ReadLockScoped rl(_codecWrapperLock);
    status = Transcode(payload, payloadLenBytes, isacBwEstimate, isacRate, true);
    return status;
}

#endif

} 











#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_GENERIC_CODEC_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_GENERIC_CODEC_H_

#include "acm_common_defs.h"
#include "audio_coding_module_typedefs.h"
#include "rw_lock_wrapper.h"
#include "trace.h"
#include "webrtc_neteq.h"

#define MAX_FRAME_SIZE_10MSEC 6


struct WebRtcVadInst;
struct WebRtcCngEncInst;

namespace webrtc
{


struct CodecInst;
class  ACMNetEQ;

class ACMGenericCodec
{
public:
    
    
    
    ACMGenericCodec();


    
    
    
    virtual ~ACMGenericCodec();


    
    
    
    
    virtual ACMGenericCodec* CreateInstance() = 0;


    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    WebRtc_Word16 Encode(
        WebRtc_UWord8*         bitStream,
        WebRtc_Word16*         bitStreamLenByte,
        WebRtc_UWord32*        timeStamp,
        WebRtcACMEncodingType* encodingType);


    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    WebRtc_Word16 Decode(
        WebRtc_UWord8* bitStream,
        WebRtc_Word16  bitStreamLenByte,
        WebRtc_Word16* audio,
        WebRtc_Word16* audioSamples,
        WebRtc_Word8*  speechType);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual void SplitStereoPacket(WebRtc_UWord8* ,
                                   WebRtc_Word32* ) {}

    
    
    
    
    
    
    
    bool EncoderInitialized();


    
    
    
    
    
    
    
    bool DecoderInitialized();


    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    WebRtc_Word16 EncoderParams(
        WebRtcACMCodecParams *encParams);


    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    bool DecoderParams(
        WebRtcACMCodecParams *decParams,
        const WebRtc_UWord8  payloadType);


    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    WebRtc_Word16 InitEncoder(
        WebRtcACMCodecParams* codecParams,
        bool                  forceInitialization);


    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    WebRtc_Word16 InitDecoder(
        WebRtcACMCodecParams* codecParams,
        bool                 forceInitialization);


    
    
    
    
    
    
    
    
    
    
    
    
    
    WebRtc_Word32 RegisterInNetEq(
        ACMNetEQ*             netEq,
        const CodecInst& codecInst);


    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    WebRtc_Word32 Add10MsData(
        const WebRtc_UWord32 timeStamp,
        const WebRtc_Word16* data,
        const WebRtc_UWord16 length,
        const WebRtc_UWord8  audioChannel);


    
    
    
    
    
    
    
    
    
    
    WebRtc_UWord32 NoMissedSamples() const;


    
    
    
    
    
    void ResetNoMissedSamples();


    
    
    
    
    
    
    
    
    
    
    
    
    WebRtc_Word16 SetBitRate(const WebRtc_Word32 bitRateBPS);


    
    
    
    
    
    
    
    
    
    
    void DestructEncoderInst(
        void* ptrInst);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    WebRtc_Word16 AudioBuffer(
        WebRtcACMAudioBuff& audioBuff);


    
    
    
    
    
    
    
    
    WebRtc_UWord32 EarliestTimestamp() const;


    
    
    
    
    
    
    
    
    
    WebRtc_Word16 SetAudioBuffer(WebRtcACMAudioBuff& audioBuff);



    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    WebRtc_Word16 SetVAD(
        const bool             enableDTX = true,
        const bool             enableVAD = false,
        const ACMVADMode mode      = VADNormal);


    
    
    
    
    
    
    
    
    
    
    
    
    
    WebRtc_Word32 ReplaceInternalDTX(const bool replaceInternalDTX);

    
    
    
    
    
    
    
    
    
    
    
    
    
    WebRtc_Word32 IsInternalDTXReplaced(bool* internalDTXReplaced);

    
    
    
    
    
    
    
    void SetNetEqDecodeLock(
        RWLockWrapper* netEqDecodeLock)
    {
        _netEqDecodeLock = netEqDecodeLock;
    }


    
    
    
    
    
    
    
    
    bool HasInternalDTX() const
    {
        return _hasInternalDTX;
    }


   
    
    
    
    
    
    
    
    
    WebRtc_Word32 GetEstimatedBandwidth();


    
    
    
    
    
    
    
    
    
    
    
    
    WebRtc_Word32 SetEstimatedBandwidth(WebRtc_Word32 estimatedBandwidth);

    
    
    
    
    
    
    
    
    
    
    
    
    
    WebRtc_Word32 GetRedPayload(
        WebRtc_UWord8* redPayload,
        WebRtc_Word16* payloadBytes);


    
    
    
    
    
    
    
    
    
    
    WebRtc_Word16 ResetEncoder();


    
    
    
    
    
    
    
    
    
    WebRtc_Word16 ResetDecoder(
        WebRtc_Word16 payloadType);


    
    
    
    
    
    
    
    void DestructEncoder();


    
    
    
    
    
    
    
    
    void DestructDecoder();


    
    
    
    
    
    
    
    WebRtc_Word16 SamplesLeftToEncode();


    
    
    
    
    
    
    
    WebRtc_UWord32 LastEncodedTimestamp() const;


    
    
    
    
    
    
    
    void SetUniqueID(
        const WebRtc_UWord32 id);


    
    
    
    
    
    
    
    
    bool IsAudioBufferFresh() const;


    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual WebRtc_Word16 UpdateDecoderSampFreq(
        WebRtc_Word16 )
    {
        return 0;
    }


    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual WebRtc_Word16 UpdateEncoderSampFreq(
        WebRtc_UWord16 encoderSampFreqHz);


    
    
    
    
    
    
    
    
    
    
    
    
    virtual WebRtc_Word16 EncoderSampFreq(
        WebRtc_UWord16& sampFreqHz);


    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual WebRtc_Word32 ConfigISACBandwidthEstimator(
        const WebRtc_UWord8  initFrameSizeMsec,
        const WebRtc_UWord16 initRateBitPerSec,
        const bool           enforceFrameSize);


    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual WebRtc_Word32 SetISACMaxPayloadSize(
        const WebRtc_UWord16 maxPayloadLenBytes);


    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual WebRtc_Word32 SetISACMaxRate(
        const WebRtc_UWord32 maxRateBitPerSec);


    
    
    
    
    
    
    
    
    void SaveDecoderParam(
        const WebRtcACMCodecParams* codecParams);


    WebRtc_Word32 FrameSize()
    {
        return _frameLenSmpl;
    }

    void SetIsMaster(bool isMaster);




    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual WebRtc_Word16 REDPayloadISAC(
        const WebRtc_Word32 isacRate,
        const WebRtc_Word16 isacBwEstimate,
        WebRtc_UWord8*      payload,
        WebRtc_Word16*      payloadLenBytes);

    
    
    
    
    
    
    
    
    virtual bool IsTrueStereoCodec() {
      return false;
    }

protected:
    
    
    
    
    
    
    

    
    
    
    
    WebRtc_Word16 EncodeSafe(
        WebRtc_UWord8*         bitStream,
        WebRtc_Word16*         bitStreamLenByte,
        WebRtc_UWord32*        timeStamp,
        WebRtcACMEncodingType* encodingType);

    
    
    
    
    virtual WebRtc_Word16 DecodeSafe(
        WebRtc_UWord8* bitStream,
        WebRtc_Word16  bitStreamLenByte,
        WebRtc_Word16* audio,
        WebRtc_Word16* audioSamples,
        WebRtc_Word8*  speechType) = 0;

    
    
    
    
    virtual WebRtc_Word32 Add10MsDataSafe(
        const WebRtc_UWord32 timeStamp,
        const WebRtc_Word16* data,
        const WebRtc_UWord16 length,
        const WebRtc_UWord8  audioChannel);

    
    
    
    
    virtual WebRtc_Word32 CodecDef(
        WebRtcNetEQ_CodecDef& codecDef,
        const CodecInst&  codecInst) = 0;

    
    
    
    
    WebRtc_Word16 EncoderParamsSafe(
        WebRtcACMCodecParams *encParams);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual bool  DecoderParamsSafe(
        WebRtcACMCodecParams *decParams,
        const WebRtc_UWord8  payloadType);

    
    
    
    
    WebRtc_Word16 ResetEncoderSafe();

    
    
    
    
    WebRtc_Word16 InitEncoderSafe(
        WebRtcACMCodecParams *codecParams,
        bool                 forceInitialization);

    
    
    
    
    WebRtc_Word16 InitDecoderSafe(
        WebRtcACMCodecParams *codecParams,
        bool                 forceInitialization);

    
    
    
    
    WebRtc_Word16 ResetDecoderSafe(
        WebRtc_Word16 payloadType);

    
    
    
    
    virtual void DestructEncoderSafe() = 0;

    
    
    
    
    virtual void DestructDecoderSafe() = 0;

    
    
    
    
    
    
    virtual WebRtc_Word16 SetBitRateSafe(
        const WebRtc_Word32 bitRateBPS);

    
    
    
    
    virtual WebRtc_Word32 GetEstimatedBandwidthSafe();

    
    
    
    
    virtual WebRtc_Word32 SetEstimatedBandwidthSafe(WebRtc_Word32 estimatedBandwidth);

    
    
    
    
    virtual WebRtc_Word32 GetRedPayloadSafe(
        WebRtc_UWord8* redPayload,
        WebRtc_Word16* payloadBytes);

    
    
    
    
    WebRtc_Word16 SetVADSafe(
        const bool       enableDTX = true,
        const bool       enableVAD = false,
        const ACMVADMode mode      = VADNormal);

    
    
    
    
    virtual WebRtc_Word32 ReplaceInternalDTXSafe(
        const bool replaceInternalDTX);

    
    
    
    
    virtual WebRtc_Word32 IsInternalDTXReplacedSafe(
        bool* internalDTXReplaced);

    
    
    
    
    
    
    
    
    WebRtc_Word16 CreateEncoder();


    
    
    
    
    
    
    
    
    WebRtc_Word16 CreateDecoder();


    
    
    
    
    
    
    
    
    
    
    
    
    
    WebRtc_Word16 EnableVAD(ACMVADMode mode);


    
    
    
    
    
    
    
    
    WebRtc_Word16 DisableVAD();


    
    
    
    
    
    
    
    
    
    virtual WebRtc_Word16 EnableDTX();


    
    
    
    
    
    
    
    
    
    virtual WebRtc_Word16 DisableDTX();


    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual WebRtc_Word16 InternalEncode(
        WebRtc_UWord8* bitStream,
        WebRtc_Word16* bitStreamLenByte) = 0;


    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual WebRtc_Word16 InternalInitEncoder(
        WebRtcACMCodecParams *codecParams) = 0;


    
    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual WebRtc_Word16 InternalInitDecoder(
        WebRtcACMCodecParams *codecParams) = 0;


    
    
    
    
    
    
    
    
    
    void IncreaseNoMissedSamples(
        const WebRtc_Word16 noSamples);


    
    
    
    
    
    
    
    
    
    
    virtual WebRtc_Word16 InternalCreateEncoder() = 0;


    
    
    
    
    
    
    
    
    
    
    virtual WebRtc_Word16 InternalCreateDecoder() = 0;


    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual void InternalDestructEncoderInst(
        void* ptrInst) = 0;


    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual WebRtc_Word16 InternalResetEncoder();



    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    WebRtc_Word16 ProcessFrameVADDTX(
        WebRtc_UWord8* bitStream,
        WebRtc_Word16* bitStreamLenByte,
        WebRtc_Word16* samplesProcessed);


    
    
    
    
    
    
    
    
    
    virtual bool CanChangeEncodingParam(CodecInst& codecInst);


    
    
    
    
    
    
    
    
    
    
    virtual void CurrentRate(
        WebRtc_Word32& )
    {
        return;
    }

    virtual void SaveDecoderParamSafe(
        const WebRtcACMCodecParams* codecParams);

    
    
    WebRtc_Word16         _inAudioIxWrite;

    
    WebRtc_Word16         _inAudioIxRead;

    WebRtc_Word16         _inTimestampIxWrite;

    
    
    
    
    WebRtc_Word16*        _inAudio;
    WebRtc_UWord32*       _inTimestamp;

    WebRtc_Word16         _frameLenSmpl;
    WebRtc_UWord16        _noChannels;

    
    WebRtc_Word16         _codecID;

    
    
    
    WebRtc_UWord32        _noMissedSamples;

    
    bool                  _encoderExist;
    bool                  _decoderExist;
    
    bool                  _encoderInitialized;
    bool                  _decoderInitialized;

    bool                  _registeredInNetEq;

    
    bool                  _hasInternalDTX;
    WebRtcVadInst*        _ptrVADInst;
    bool                  _vadEnabled;
    ACMVADMode            _vadMode;
    WebRtc_Word16         _vadLabel[MAX_FRAME_SIZE_10MSEC];
    bool                  _dtxEnabled;
    WebRtcCngEncInst*     _ptrDTXInst;
    WebRtc_UWord8         _numLPCParams;
    bool                  _sentCNPrevious;
    bool                  _isMaster;

    WebRtcACMCodecParams  _encoderParams;
    WebRtcACMCodecParams  _decoderParams;

    
    
    RWLockWrapper*        _netEqDecodeLock;
    
    
    RWLockWrapper&        _codecWrapperLock;

    WebRtc_UWord32        _lastEncodedTimestamp;
    WebRtc_UWord32        _lastTimestamp;
    bool                  _isAudioBuffFresh;
    WebRtc_UWord32        _uniqueID;
};

} 

#endif  

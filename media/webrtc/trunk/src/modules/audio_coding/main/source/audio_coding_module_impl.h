









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_AUDIO_CODING_MODULE_IMPL_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_AUDIO_CODING_MODULE_IMPL_H_

#include "acm_codec_database.h"
#include "acm_neteq.h"
#include "acm_resampler.h"
#include "common_types.h"
#include "engine_configurations.h"

namespace webrtc {

class ACMDTMFDetection;
class ACMGenericCodec;
class CriticalSectionWrapper;
class RWLockWrapper;



#ifdef TIMED_LOGGING
    #include "../test/timedtrace.h"
#endif

#ifdef ACM_QA_TEST
#   include <stdio.h>
#endif

class AudioCodingModuleImpl : public AudioCodingModule
{
public:
    
    AudioCodingModuleImpl(
        const WebRtc_Word32 id);

    
    ~AudioCodingModuleImpl();

    
    WebRtc_Word32 Version(
        char*   version,
        WebRtc_UWord32& remainingBufferInBytes,
        WebRtc_UWord32& position) const;

    
    virtual WebRtc_Word32 ChangeUniqueId(
        const WebRtc_Word32 id);

    
    
    WebRtc_Word32 TimeUntilNextProcess();

    
    WebRtc_Word32 Process();

    
    
    WebRtc_Word32 SetMode(
        const bool passive);



    
    
    

    
    WebRtc_Word32 InitializeSender();

    
    WebRtc_Word32 ResetEncoder();

    
    WebRtc_Word32 RegisterSendCodec(
        const CodecInst& sendCodec);

    
    WebRtc_Word32 SendCodec(
        CodecInst& currentSendCodec) const;

    
    WebRtc_Word32 SendFrequency() const;

    
    
    
    WebRtc_Word32 SendBitrate() const;

    
    
    virtual WebRtc_Word32 SetReceivedEstimatedBandwidth(
        const WebRtc_Word32 bw);

    
    
    WebRtc_Word32 RegisterTransportCallback(
        AudioPacketizationCallback* transport);

    
    
    WebRtc_Word32 RegisterIncomingMessagesCallback(
        AudioCodingFeedback* incomingMessagesCallback,
        const ACMCountries cpt);

     
    WebRtc_Word32 Add10MsData(
        const AudioFrame& audioFrame);

    
    WebRtc_Word32 SetBackgroundNoiseMode(
        const ACMBackgroundNoiseMode mode);

    
    WebRtc_Word32 BackgroundNoiseMode(
        ACMBackgroundNoiseMode& mode);

    
    
    

    
    WebRtc_Word32 SetFECStatus(
        const bool enable);

    
    bool FECStatus() const;

    
    
    
    
    

    WebRtc_Word32 SetVAD(
        const bool             enableDTX = true,
        const bool             enableVAD = false,
        const ACMVADMode vadMode   = VADNormal);

    WebRtc_Word32 VAD(
        bool&             dtxEnabled,
        bool&             vadEnabled,
        ACMVADMode& vadMode) const;

    WebRtc_Word32 RegisterVADCallback(
        ACMVADCallback* vadCallback);

    
    ACMVADMode ReceiveVADMode() const;

    
    WebRtc_Word16 SetReceiveVADMode(
        const ACMVADMode mode);


    
    
    

    
    WebRtc_Word32 InitializeReceiver();

    
    WebRtc_Word32 ResetDecoder();

    
    WebRtc_Word32 ReceiveFrequency() const;

    
    WebRtc_Word32 PlayoutFrequency() const;

    
    
    WebRtc_Word32 RegisterReceiveCodec(
        const CodecInst& receiveCodec);

    
    WebRtc_Word32 ReceiveCodec(
        CodecInst& currentReceiveCodec) const;

    
    WebRtc_Word32 IncomingPacket(
        const WebRtc_UWord8*   incomingPayload,
        const WebRtc_Word32    payloadLength,
        const WebRtcRTPHeader& rtpInfo);

    
    
    WebRtc_Word32 IncomingPayload(
        const WebRtc_UWord8* incomingPayload,
        const WebRtc_Word32  payloadLength,
        const WebRtc_UWord8  payloadType,
        const WebRtc_UWord32 timestamp = 0);

    
    WebRtc_Word32 SetMinimumPlayoutDelay(
        const WebRtc_Word32 timeMs);

    
    WebRtc_Word32 SetDtmfPlayoutStatus(
        const bool enable);

    
    bool DtmfPlayoutStatus() const;

    
    
    
    WebRtc_Word32 DecoderEstimatedBandwidth() const;

    
    WebRtc_Word32 SetPlayoutMode(
        const AudioPlayoutMode mode);

    
    AudioPlayoutMode PlayoutMode() const;

    
    WebRtc_Word32 PlayoutTimestamp(
        WebRtc_UWord32& timestamp);

    
    
    WebRtc_Word32 PlayoutData10Ms(
        const WebRtc_Word32   desiredFreqHz,
        AudioFrame            &audioFrame);


    
    
    

    WebRtc_Word32  NetworkStatistics(
        ACMNetworkStatistics& statistics) const;

    void DestructEncoderInst(void* ptrInst);

    WebRtc_Word16 AudioBuffer(WebRtcACMAudioBuff& audioBuff);

    
    
    WebRtc_Word32 REDPayloadISAC(
        const WebRtc_Word32  isacRate,
        const WebRtc_Word16  isacBwEstimate,
        WebRtc_UWord8*       payload,
        WebRtc_Word16*       payloadLenByte);

    WebRtc_Word16 SetAudioBuffer(WebRtcACMAudioBuff& audioBuff);

    WebRtc_UWord32 EarliestTimestamp() const;

    WebRtc_Word32 LastEncodedTimestamp(WebRtc_UWord32& timestamp) const;

    WebRtc_Word32 ReplaceInternalDTXWithWebRtc(
        const bool useWebRtcDTX);

    WebRtc_Word32 IsInternalDTXReplacedWithWebRtc(
        bool& usesWebRtcDTX);

    WebRtc_Word32 SetISACMaxRate(
        const WebRtc_UWord32 rateBitPerSec);

    WebRtc_Word32 SetISACMaxPayloadSize(
        const WebRtc_UWord16 payloadLenBytes);

    WebRtc_Word32 ConfigISACBandwidthEstimator(
        const WebRtc_UWord8  initFrameSizeMsec,
        const WebRtc_UWord16 initRateBitPerSec,
        const bool           enforceFrameSize = false);

    WebRtc_Word32 UnregisterReceiveCodec(
        const WebRtc_Word16 payloadType);

protected:
    void UnregisterSendCodec();

    WebRtc_Word32 UnregisterReceiveCodecSafe(
        const WebRtc_Word16 codecID);

    ACMGenericCodec* CreateCodec(
        const CodecInst& codec);

    WebRtc_Word16 DecoderParamByPlType(
        const WebRtc_UWord8    payloadType,
        WebRtcACMCodecParams&  codecParams) const;

    WebRtc_Word16 DecoderListIDByPlName(
        const char*  payloadName,
        const WebRtc_UWord16 sampFreqHz = 0) const;

    WebRtc_Word32 InitializeReceiverSafe();

    bool HaveValidEncoder(const char* callerName) const;

    WebRtc_Word32 RegisterRecCodecMSSafe(
        const CodecInst& receiveCodec,
        WebRtc_Word16         codecId,
        WebRtc_Word16         mirrorId,
        ACMNetEQ::JB          jitterBuffer);

private:
    AudioPacketizationCallback*    _packetizationCallback;
    WebRtc_Word32                  _id;
    WebRtc_UWord32                 _lastTimestamp;
    WebRtc_UWord32                 _lastInTimestamp;
    CodecInst                      _sendCodecInst;
    uint8_t                        _cng_nb_pltype;
    uint8_t                        _cng_wb_pltype;
    uint8_t                        _cng_swb_pltype;
    uint8_t                        _red_pltype;
    bool                           _cng_reg_receiver;
    bool                           _vadEnabled;
    bool                           _dtxEnabled;
    ACMVADMode                     _vadMode;
    ACMGenericCodec*               _codecs[ACMCodecDB::kMaxNumCodecs];
    ACMGenericCodec*               _slaveCodecs[ACMCodecDB::kMaxNumCodecs];
    WebRtc_Word16                  _mirrorCodecIdx[ACMCodecDB::kMaxNumCodecs];
    bool                           _stereoReceive[ACMCodecDB::kMaxNumCodecs];
    bool                           _stereoReceiveRegistered;
    bool                           _stereoSend;
    int                            _prev_received_channel;
    int                            _expected_channels;
    WebRtc_Word32                  _currentSendCodecIdx;
    int                            _current_receive_codec_idx;
    bool                           _sendCodecRegistered;
    ACMResampler                   _inputResampler;
    ACMResampler                   _outputResampler;
    ACMNetEQ                       _netEq;
    CriticalSectionWrapper*        _acmCritSect;
    ACMVADCallback*                _vadCallback;
    WebRtc_UWord8                  _lastRecvAudioCodecPlType;

    
    bool                           _isFirstRED;
    bool                           _fecEnabled;
    WebRtc_UWord8*                 _redBuffer;
    RTPFragmentationHeader*        _fragmentation;
    WebRtc_UWord32                 _lastFECTimestamp;
    
    
    WebRtc_UWord8                  _receiveREDPayloadType;

    
    WebRtc_UWord8                  _previousPayloadType;

    
    
    
    WebRtc_Word16                  _registeredPlTypes[ACMCodecDB::kMaxNumCodecs];

    
    
    
    WebRtcRTPHeader*               _dummyRTPHeader;
    WebRtc_UWord16                 _recvPlFrameSizeSmpls;

    bool                           _receiverInitialized;
    ACMDTMFDetection*              _dtmfDetector;

    AudioCodingFeedback*           _dtmfCallback;
    WebRtc_Word16                  _lastDetectedTone;
    CriticalSectionWrapper*        _callbackCritSect;
#ifdef TIMED_LOGGING
    TimedTrace                     _trace;
#endif

    AudioFrame                     _audioFrame;

#ifdef ACM_QA_TEST
    FILE* _outgoingPL;
    FILE* _incomingPL;
#endif

};

} 

#endif  

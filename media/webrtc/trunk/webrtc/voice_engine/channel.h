









#ifndef WEBRTC_VOICE_ENGINE_CHANNEL_H
#define WEBRTC_VOICE_ENGINE_CHANNEL_H

#include "webrtc/common_audio/resampler/include/resampler.h"
#include "webrtc/common_types.h"
#include "webrtc/modules/audio_coding/main/interface/audio_coding_module.h"
#include "webrtc/modules/audio_conference_mixer/interface/audio_conference_mixer_defines.h"
#include "webrtc/modules/rtp_rtcp/interface/rtp_rtcp.h"
#include "webrtc/modules/utility/interface/file_player.h"
#include "webrtc/modules/utility/interface/file_recorder.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/voice_engine/dtmf_inband.h"
#include "webrtc/voice_engine/dtmf_inband_queue.h"
#include "webrtc/voice_engine/include/voe_audio_processing.h"
#include "webrtc/voice_engine/include/voe_network.h"
#include "webrtc/voice_engine/level_indicator.h"
#include "webrtc/voice_engine/shared_data.h"
#include "webrtc/voice_engine/voice_engine_defines.h"

#ifdef WEBRTC_DTMF_DETECTION
#include "voe_dtmf.h" 
#endif

namespace webrtc
{
class CriticalSectionWrapper;
class ProcessThread;
class AudioDeviceModule;
class RtpRtcp;
class FileWrapper;
class RtpDump;
class VoiceEngineObserver;
class VoEMediaProcess;
class VoERTPObserver;
class VoERTCPObserver;

struct CallStatistics;
struct ReportBlock;
struct SenderInfo;

namespace voe
{
class Statistics;
class TransmitMixer;
class OutputMixer;


class Channel:
    public RtpData,
    public RtpFeedback,
    public RtcpFeedback,
    public FileCallback, 
    public Transport,
    public RtpAudioFeedback,
    public AudioPacketizationCallback, 
    public ACMVADCallback, 
    public MixerParticipant 
{
public:
    enum {KNumSocketThreads = 1};
    enum {KNumberOfSocketBuffers = 8};
public:
    virtual ~Channel();
    static int32_t CreateChannel(Channel*& channel,
                                 const int32_t channelId,
                                 const uint32_t instanceId);
    Channel(const int32_t channelId, const uint32_t instanceId);
    int32_t Init();
    int32_t SetEngineInformation(
        Statistics& engineStatistics,
        OutputMixer& outputMixer,
        TransmitMixer& transmitMixer,
        ProcessThread& moduleProcessThread,
        AudioDeviceModule& audioDeviceModule,
        VoiceEngineObserver* voiceEngineObserver,
        CriticalSectionWrapper* callbackCritSect);
    int32_t UpdateLocalTimeStamp();

public:
    

    
    int32_t StartPlayout();
    int32_t StopPlayout();
    int32_t StartSend();
    int32_t StopSend();
    int32_t StartReceiving();
    int32_t StopReceiving();

    int32_t SetNetEQPlayoutMode(NetEqModes mode);
    int32_t GetNetEQPlayoutMode(NetEqModes& mode);
    int32_t SetOnHoldStatus(bool enable, OnHoldModes mode);
    int32_t GetOnHoldStatus(bool& enabled, OnHoldModes& mode);
    int32_t RegisterVoiceEngineObserver(VoiceEngineObserver& observer);
    int32_t DeRegisterVoiceEngineObserver();

    
    int32_t GetSendCodec(CodecInst& codec);
    int32_t GetRecCodec(CodecInst& codec);
    int32_t SetSendCodec(const CodecInst& codec);
    int32_t SetVADStatus(bool enableVAD, ACMVADMode mode, bool disableDTX);
    int32_t GetVADStatus(bool& enabledVAD, ACMVADMode& mode, bool& disabledDTX);
    int32_t SetRecPayloadType(const CodecInst& codec);
    int32_t GetRecPayloadType(CodecInst& codec);
    int32_t SetAMREncFormat(AmrMode mode);
    int32_t SetAMRDecFormat(AmrMode mode);
    int32_t SetAMRWbEncFormat(AmrMode mode);
    int32_t SetAMRWbDecFormat(AmrMode mode);
    int32_t SetSendCNPayloadType(int type, PayloadFrequencies frequency);
    int32_t SetISACInitTargetRate(int rateBps, bool useFixedFrameSize);
    int32_t SetISACMaxRate(int rateBps);
    int32_t SetISACMaxPayloadSize(int sizeBytes);

    
    int SetSecondarySendCodec(const CodecInst& codec, int red_payload_type);
    void RemoveSecondarySendCodec();
    int GetSecondarySendCodec(CodecInst* codec);

    
    int32_t RegisterExternalTransport(Transport& transport);
    int32_t DeRegisterExternalTransport();
    int32_t ReceivedRTPPacket(const int8_t* data, int32_t length);
    int32_t ReceivedRTCPPacket(const int8_t* data, int32_t length);
    int32_t SetPacketTimeoutNotification(bool enable, int timeoutSeconds);
    int32_t GetPacketTimeoutNotification(bool& enabled, int& timeoutSeconds);
    int32_t RegisterDeadOrAliveObserver(VoEConnectionObserver& observer);
    int32_t DeRegisterDeadOrAliveObserver();
    int32_t SetPeriodicDeadOrAliveStatus(bool enable, int sampleTimeSeconds);
    int32_t GetPeriodicDeadOrAliveStatus(bool& enabled, int& sampleTimeSeconds);

    
    int StartPlayingFileLocally(const char* fileName, const bool loop,
                                const FileFormats format,
                                const int startPosition,
                                const float volumeScaling,
                                const int stopPosition,
                                const CodecInst* codecInst);
    int StartPlayingFileLocally(InStream* stream, const FileFormats format,
                                const int startPosition,
                                const float volumeScaling,
                                const int stopPosition,
                                const CodecInst* codecInst);
    int StopPlayingFileLocally();
    int IsPlayingFileLocally() const;
    int RegisterFilePlayingToMixer();
    int ScaleLocalFilePlayout(const float scale);
    int GetLocalPlayoutPosition(int& positionMs);
    int StartPlayingFileAsMicrophone(const char* fileName, const bool loop,
                                     const FileFormats format,
                                     const int startPosition,
                                     const float volumeScaling,
                                     const int stopPosition,
                                     const CodecInst* codecInst);
    int StartPlayingFileAsMicrophone(InStream* stream,
                                     const FileFormats format,
                                     const int startPosition,
                                     const float volumeScaling,
                                     const int stopPosition,
                                     const CodecInst* codecInst);
    int StopPlayingFileAsMicrophone();
    int IsPlayingFileAsMicrophone() const;
    int ScaleFileAsMicrophonePlayout(const float scale);
    int StartRecordingPlayout(const char* fileName, const CodecInst* codecInst);
    int StartRecordingPlayout(OutStream* stream, const CodecInst* codecInst);
    int StopRecordingPlayout();

    void SetMixWithMicStatus(bool mix);

    
    int RegisterExternalMediaProcessing(ProcessingTypes type,
                                        VoEMediaProcess& processObject);
    int DeRegisterExternalMediaProcessing(ProcessingTypes type);
    int SetExternalMixing(bool enabled);

    
    int GetSpeechOutputLevel(uint32_t& level) const;
    int GetSpeechOutputLevelFullRange(uint32_t& level) const;
    int SetMute(const bool enable);
    bool Mute() const;
    int SetOutputVolumePan(float left, float right);
    int GetOutputVolumePan(float& left, float& right) const;
    int SetChannelOutputVolumeScaling(float scaling);
    int GetChannelOutputVolumeScaling(float& scaling) const;

    
    void ResetDeadOrAliveCounters();
    int ResetRTCPStatistics();
    int GetRoundTripTimeSummary(StatVal& delaysMs) const;
    int GetDeadOrAliveCounters(int& countDead, int& countAlive) const;

    
    int GetNetworkStatistics(NetworkStatistics& stats);

    
    bool GetDelayEstimate(int* jitter_buffer_delay_ms,
                          int* playout_buffer_delay_ms) const;
    int SetInitialPlayoutDelay(int delay_ms);
    int SetMinimumPlayoutDelay(int delayMs);
    int GetPlayoutTimestamp(unsigned int& timestamp);
    void UpdatePlayoutTimestamp(bool rtcp);
    int SetInitTimestamp(unsigned int timestamp);
    int SetInitSequenceNumber(short sequenceNumber);

    
    int GetRtpRtcp(RtpRtcp* &rtpRtcpModule) const;

    
    int RegisterExternalEncryption(Encryption& encryption);
    int DeRegisterExternalEncryption();

    
    int SendTelephoneEventOutband(unsigned char eventCode, int lengthMs,
                                  int attenuationDb, bool playDtmfEvent);
    int SendTelephoneEventInband(unsigned char eventCode, int lengthMs,
                                 int attenuationDb, bool playDtmfEvent);
    int SetDtmfPlayoutStatus(bool enable);
    bool DtmfPlayoutStatus() const;
    int SetSendTelephoneEventPayloadType(unsigned char type);
    int GetSendTelephoneEventPayloadType(unsigned char& type);

    
    int UpdateRxVadDetection(AudioFrame& audioFrame);
    int RegisterRxVadObserver(VoERxVadCallback &observer);
    int DeRegisterRxVadObserver();
    int VoiceActivityIndicator(int &activity);
#ifdef WEBRTC_VOICE_ENGINE_AGC
    int SetRxAgcStatus(const bool enable, const AgcModes mode);
    int GetRxAgcStatus(bool& enabled, AgcModes& mode);
    int SetRxAgcConfig(const AgcConfig config);
    int GetRxAgcConfig(AgcConfig& config);
#endif
#ifdef WEBRTC_VOICE_ENGINE_NR
    int SetRxNsStatus(const bool enable, const NsModes mode);
    int GetRxNsStatus(bool& enabled, NsModes& mode);
#endif

    
    int RegisterRTPObserver(VoERTPObserver& observer);
    int DeRegisterRTPObserver();
    int RegisterRTCPObserver(VoERTCPObserver& observer);
    int DeRegisterRTCPObserver();
    int SetLocalSSRC(unsigned int ssrc);
    int GetLocalSSRC(unsigned int& ssrc);
    int GetRemoteSSRC(unsigned int& ssrc);
    int GetRemoteCSRCs(unsigned int arrCSRC[15]);
    int SetRTPAudioLevelIndicationStatus(bool enable, unsigned char ID);
    int GetRTPAudioLevelIndicationStatus(bool& enable, unsigned char& ID);
    int SetRTCPStatus(bool enable);
    int GetRTCPStatus(bool& enabled);
    int SetRTCP_CNAME(const char cName[256]);
    int GetRTCP_CNAME(char cName[256]);
    int GetRemoteRTCP_CNAME(char cName[256]);
    int GetRemoteRTCPData(unsigned int& NTPHigh, unsigned int& NTPLow,
                          unsigned int& timestamp,
                          unsigned int& playoutTimestamp, unsigned int* jitter,
                          unsigned short* fractionLost);
    int SendApplicationDefinedRTCPPacket(const unsigned char subType,
                                         unsigned int name, const char* data,
                                         unsigned short dataLengthInBytes);
    int GetRTPStatistics(unsigned int& averageJitterMs,
                         unsigned int& maxJitterMs,
                         unsigned int& discardedPackets);
    int GetRemoteRTCPSenderInfo(SenderInfo* sender_info);
    int GetRemoteRTCPReportBlocks(std::vector<ReportBlock>* report_blocks);
    int GetRTPStatistics(CallStatistics& stats);
    int SetFECStatus(bool enable, int redPayloadtype);
    int GetFECStatus(bool& enabled, int& redPayloadtype);
    int StartRTPDump(const char fileNameUTF8[1024], RTPDirections direction);
    int StopRTPDump(RTPDirections direction);
    bool RTPDumpIsActive(RTPDirections direction);
    int InsertExtraRTPPacket(unsigned char payloadType, bool markerBit,
                             const char* payloadData,
                             unsigned short payloadSize);
    uint32_t LastRemoteTimeStamp() { return _lastRemoteTimeStamp; }

public:
    
    int32_t SendData(FrameType frameType,
                     uint8_t payloadType,
                     uint32_t timeStamp,
                     const uint8_t* payloadData,
                     uint16_t payloadSize,
                     const RTPFragmentationHeader* fragmentation);
    
    int32_t InFrameType(int16_t frameType);

public:
    int32_t OnRxVadDetected(const int vadDecision);

public:
    
    int32_t OnReceivedPayloadData(const uint8_t* payloadData,
                                  const uint16_t payloadSize,
                                  const WebRtcRTPHeader* rtpHeader);

public:
    
    int32_t OnInitializeDecoder(
            const int32_t id,
            const int8_t payloadType,
            const char payloadName[RTP_PAYLOAD_NAME_SIZE],
            const int frequency,
            const uint8_t channels,
            const uint32_t rate);

    void OnPacketTimeout(const int32_t id);

    void OnReceivedPacket(const int32_t id, const RtpRtcpPacketType packetType);

    void OnPeriodicDeadOrAlive(const int32_t id,
                               const RTPAliveType alive);

    void OnIncomingSSRCChanged(const int32_t id,
                               const uint32_t SSRC);

    void OnIncomingCSRCChanged(const int32_t id,
                               const uint32_t CSRC, const bool added);

public:
    
    void OnApplicationDataReceived(const int32_t id,
                                   const uint8_t subType,
                                   const uint32_t name,
                                   const uint16_t length,
                                   const uint8_t* data);

public:
    
    void OnReceivedTelephoneEvent(const int32_t id,
                                  const uint8_t event,
                                  const bool endOfEvent);

    void OnPlayTelephoneEvent(const int32_t id,
                              const uint8_t event,
                              const uint16_t lengthMs,
                              const uint8_t volume);

public:
    
    int SendPacket(int , const void *data, int len);
    int SendRTCPPacket(int , const void *data, int len);

public:
    
    int32_t GetAudioFrame(const int32_t id, AudioFrame& audioFrame);
    int32_t NeededFrequency(const int32_t id);

public:
    
    void OnPeriodicProcess();

public:
    
    void PlayNotification(const int32_t id,
                          const uint32_t durationMs);
    void RecordNotification(const int32_t id,
                            const uint32_t durationMs);
    void PlayFileEnded(const int32_t id);
    void RecordFileEnded(const int32_t id);

public:
    uint32_t InstanceId() const
    {
        return _instanceId;
    }
    int32_t ChannelId() const
    {
        return _channelId;
    }
    bool Playing() const
    {
        return _playing;
    }
    bool Sending() const
    {
        
        
        
        CriticalSectionScoped cs(&_callbackCritSect);
        return _sending;
    }
    bool Receiving() const
    {
        return _receiving;
    }
    bool ExternalTransport() const
    {
        return _externalTransport;
    }
    bool ExternalMixing() const
    {
        return _externalMixing;
    }
    bool OutputIsOnHold() const
    {
        return _outputIsOnHold;
    }
    bool InputIsOnHold() const
    {
        return _inputIsOnHold;
    }
    RtpRtcp* RtpRtcpModulePtr() const
    {
        return _rtpRtcpModule.get();
    }
    int8_t OutputEnergyLevel() const
    {
        return _outputAudioLevel.Level();
    }
    uint32_t Demultiplex(const AudioFrame& audioFrame);
    uint32_t PrepareEncodeAndSend(int mixingFrequency);
    uint32_t EncodeAndSend();

private:
    int InsertInbandDtmfTone();
    int32_t MixOrReplaceAudioWithFile(const int mixingFrequency);
    int32_t MixAudioWithFile(AudioFrame& audioFrame, const int mixingFrequency);
    void UpdateDeadOrAliveCounters(bool alive);
    int32_t SendPacketRaw(const void *data, int len, bool RTCP);
    void UpdatePacketDelay(uint32_t timestamp,
                           uint16_t sequenceNumber);
    void RegisterReceiveCodecsToRTPModule();
    int ApmProcessRx(AudioFrame& audioFrame);

    int SetRedPayloadType(int red_payload_type);
private:
    CriticalSectionWrapper& _fileCritSect;
    CriticalSectionWrapper& _callbackCritSect;
    uint32_t _instanceId;
    int32_t _channelId;

private:
    scoped_ptr<RtpRtcp> _rtpRtcpModule;
    AudioCodingModule& _audioCodingModule;
    RtpDump& _rtpDumpIn;
    RtpDump& _rtpDumpOut;
private:
    AudioLevel _outputAudioLevel;
    bool _externalTransport;
    AudioFrame _audioFrame;
    uint8_t _audioLevel_dBov;
    FilePlayer* _inputFilePlayerPtr;
    FilePlayer* _outputFilePlayerPtr;
    FileRecorder* _outputFileRecorderPtr;
    int _inputFilePlayerId;
    int _outputFilePlayerId;
    int _outputFileRecorderId;
    bool _inputFilePlaying;
    bool _outputFilePlaying;
    bool _outputFileRecording;
    DtmfInbandQueue _inbandDtmfQueue;
    DtmfInband _inbandDtmfGenerator;
    bool _inputExternalMedia;
    bool _outputExternalMedia;
    VoEMediaProcess* _inputExternalMediaCallbackPtr;
    VoEMediaProcess* _outputExternalMediaCallbackPtr;
    uint8_t* _encryptionRTPBufferPtr;
    uint8_t* _decryptionRTPBufferPtr;
    uint8_t* _encryptionRTCPBufferPtr;
    uint8_t* _decryptionRTCPBufferPtr;
    uint32_t _timeStamp;
    uint8_t _sendTelephoneEventPayloadType;
    uint32_t playout_timestamp_rtp_;
    uint32_t playout_timestamp_rtcp_;
    uint32_t playout_delay_ms_;
    uint32_t _numberOfDiscardedPackets;

 private:
    
    Statistics* _engineStatisticsPtr;
    OutputMixer* _outputMixerPtr;
    TransmitMixer* _transmitMixerPtr;
    ProcessThread* _moduleProcessThreadPtr;
    AudioDeviceModule* _audioDeviceModulePtr;
    VoiceEngineObserver* _voiceEngineObserverPtr; 
    CriticalSectionWrapper* _callbackCritSectPtr; 
    Transport* _transportPtr; 
    Encryption* _encryptionPtr; 
    scoped_ptr<AudioProcessing> _rtpAudioProc;
    AudioProcessing* _rxAudioProcessingModulePtr; 
    VoERxVadCallback* _rxVadObserverPtr;
    int32_t _oldVadDecision;
    int32_t _sendFrameType; 
    VoERTPObserver* _rtpObserverPtr;
    VoERTCPObserver* _rtcpObserverPtr;
private:
    
    bool _outputIsOnHold;
    bool _externalPlayout;
    bool _externalMixing;
    bool _inputIsOnHold;
    bool _playing;
    bool _sending;
    bool _receiving;
    bool _mixFileWithMicrophone;
    bool _rtpObserver;
    bool _rtcpObserver;
    
    bool _mute;
    float _panLeft;
    float _panRight;
    float _outputGain;
    
    bool _encrypting;
    bool _decrypting;
    
    bool _playOutbandDtmfEvent;
    bool _playInbandDtmfEvent;
    
    uint8_t _extraPayloadType;
    bool _insertExtraRTPPacket;
    bool _extraMarkerBit;
    uint32_t _lastLocalTimeStamp;
    uint32_t _lastRemoteTimeStamp;
    int8_t _lastPayloadType;
    bool _includeAudioLevelIndication;
    
    bool _rtpPacketTimedOut;
    bool _rtpPacketTimeOutIsEnabled;
    uint32_t _rtpTimeOutSeconds;
    bool _connectionObserver;
    VoEConnectionObserver* _connectionObserverPtr;
    uint32_t _countAliveDetections;
    uint32_t _countDeadDetections;
    AudioFrame::SpeechType _outputSpeechType;
    
    uint32_t _average_jitter_buffer_delay_us;
    uint32_t _previousTimestamp;
    uint16_t _recPacketDelayMs;
    
    bool _RxVadDetection;
    bool _rxApmIsEnabled;
    bool _rxAgcIsEnabled;
    bool _rxNsIsEnabled;
};

} 

} 

#endif

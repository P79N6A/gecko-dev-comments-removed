









#ifndef WEBRTC_VOICE_ENGINE_TRANSMIT_MIXER_H
#define WEBRTC_VOICE_ENGINE_TRANSMIT_MIXER_H

#include "common_types.h"
#include "voe_base.h"
#include "file_player.h"
#include "file_recorder.h"
#include "level_indicator.h"
#include "module_common_types.h"
#include "monitor_module.h"
#include "resampler.h"
#include "voice_engine_defines.h"


namespace webrtc {

class AudioProcessing;
class ProcessThread;
class VoEExternalMedia;
class VoEMediaProcess;

namespace voe {

class ChannelManager;
class MixedAudio;
class Statistics;

class TransmitMixer : public MonitorObserver,
                      public FileCallback

{
public:
    static WebRtc_Word32 Create(TransmitMixer*& mixer,
                                const WebRtc_UWord32 instanceId);

    static void Destroy(TransmitMixer*& mixer);

    WebRtc_Word32 SetEngineInformation(ProcessThread& processThread,
                                       Statistics& engineStatistics,
                                       ChannelManager& channelManager);

    WebRtc_Word32 SetAudioProcessingModule(
        AudioProcessing* audioProcessingModule);

    WebRtc_Word32 PrepareDemux(const void* audioSamples,
                               const WebRtc_UWord32 nSamples,
                               const WebRtc_UWord8  nChannels,
                               const WebRtc_UWord32 samplesPerSec,
                               const WebRtc_UWord16 totalDelayMS,
                               const WebRtc_Word32  clockDrift,
                               const WebRtc_UWord16 currentMicLevel);


    WebRtc_Word32 DemuxAndMix();

    WebRtc_Word32 EncodeAndSend();

    WebRtc_UWord32 CaptureLevel() const;

    WebRtc_Word32 StopSend();

    
    void UpdateMuteMicrophoneTime(const WebRtc_UWord32 lengthMs);

    
    int RegisterExternalMediaProcessing(VoEMediaProcess& proccess_object);

    int DeRegisterExternalMediaProcessing();

    int GetMixingFrequency();

    
    int SetMute(const bool enable);

    bool Mute() const;

    WebRtc_Word8 AudioLevel() const;

    WebRtc_Word16 AudioLevelFullRange() const;

    bool IsRecordingCall();

    bool IsRecordingMic();

    int StartPlayingFileAsMicrophone(const char* fileName,
                                     const bool loop,
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

    int StartRecordingMicrophone(const char* fileName,
                                 const CodecInst* codecInst);

    int StartRecordingMicrophone(OutStream* stream,
                                 const CodecInst* codecInst);

    int StopRecordingMicrophone();

    int StartRecordingCall(const char* fileName, const CodecInst* codecInst);

    int StartRecordingCall(OutStream* stream, const CodecInst* codecInst);

    int StopRecordingCall();

    void SetMixWithMicStatus(bool mix);

    WebRtc_Word32 RegisterVoiceEngineObserver(VoiceEngineObserver& observer);

    virtual ~TransmitMixer();

public:	
    void OnPeriodicProcess();


public: 
    void PlayNotification(const WebRtc_Word32 id,
                          const WebRtc_UWord32 durationMs);

    void RecordNotification(const WebRtc_Word32 id,
                            const WebRtc_UWord32 durationMs);

    void PlayFileEnded(const WebRtc_Word32 id);

    void RecordFileEnded(const WebRtc_Word32 id);

#ifdef WEBRTC_VOICE_ENGINE_TYPING_DETECTION
public:  
    int TimeSinceLastTyping(int &seconds);
    int SetTypingDetectionParameters(int timeWindow,
                                     int costPerTyping,
                                     int reportingThreshold,
                                     int penaltyDecay);
#endif

private:
    TransmitMixer(const WebRtc_UWord32 instanceId);

private:
    WebRtc_Word32 GenerateAudioFrame(const WebRtc_Word16 audioSamples[],
                                     const WebRtc_UWord32 nSamples,
                                     const WebRtc_UWord8 nChannels,
                                     const WebRtc_UWord32 samplesPerSec,
                                     const int mixingFrequency);
    WebRtc_Word32 RecordAudioToFile(const WebRtc_UWord32 mixingFrequency);

    WebRtc_Word32 MixOrReplaceAudioWithFile(
        const int mixingFrequency);

    WebRtc_Word32 APMProcessStream(const WebRtc_UWord16 totalDelayMS,
                                   const WebRtc_Word32 clockDrift,
                                   const WebRtc_UWord16 currentMicLevel);

#ifdef WEBRTC_VOICE_ENGINE_TYPING_DETECTION
    int TypingDetection();
#endif

private:  
    Statistics* _engineStatisticsPtr;
    ChannelManager* _channelManagerPtr;
    AudioProcessing* _audioProcessingModulePtr;
    VoiceEngineObserver* _voiceEngineObserverPtr;
    ProcessThread* _processThreadPtr;

private:  
    MonitorModule _monitorModule;
    AudioFrame _audioFrame;
    Resampler _audioResampler;		
    FilePlayer*	_filePlayerPtr;
    FileRecorder* _fileRecorderPtr;
    FileRecorder* _fileCallRecorderPtr;
    int _filePlayerId;
    int _fileRecorderId;
    int _fileCallRecorderId;
    bool _filePlaying;
    bool _fileRecording;
    bool _fileCallRecording;
    voe::AudioLevel _audioLevel;
    
    CriticalSectionWrapper& _critSect;
    CriticalSectionWrapper& _callbackCritSect;

#ifdef WEBRTC_VOICE_ENGINE_TYPING_DETECTION
    WebRtc_Word32 _timeActive;
    WebRtc_Word32 _timeSinceLastTyping;
    WebRtc_Word32 _penaltyCounter;
    WebRtc_UWord32 _typingNoiseWarning;

    
    int _timeWindow; 
    int _costPerTyping; 
    int _reportingThreshold; 
    int _penaltyDecay; 

#endif
    WebRtc_UWord32 _saturationWarning;
    WebRtc_UWord32 _noiseWarning;

private:
    int _instanceId;
    bool _mixFileWithMicrophone;
    WebRtc_UWord32 _captureLevel;
    bool _externalMedia;
    VoEMediaProcess* _externalMediaCallbackPtr;
    bool _mute;
    WebRtc_Word32 _remainingMuteMicTimeMs;
    int _mixingFrequency;
    bool _includeAudioLevelIndication;
};

#endif

}  

}  

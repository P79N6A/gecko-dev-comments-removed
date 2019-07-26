









#ifndef WEBRTC_VOICE_ENGINE_TRANSMIT_MIXER_H
#define WEBRTC_VOICE_ENGINE_TRANSMIT_MIXER_H

#include "webrtc/common_audio/resampler/include/push_resampler.h"
#include "webrtc/common_types.h"
#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/modules/utility/interface/file_player.h"
#include "webrtc/modules/utility/interface/file_recorder.h"
#include "webrtc/voice_engine/include/voe_base.h"
#include "webrtc/voice_engine/level_indicator.h"
#include "webrtc/voice_engine/monitor_module.h"
#include "webrtc/voice_engine/voice_engine_defines.h"

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
    static int32_t Create(TransmitMixer*& mixer, uint32_t instanceId);

    static void Destroy(TransmitMixer*& mixer);

    int32_t SetEngineInformation(ProcessThread& processThread,
                                 Statistics& engineStatistics,
                                 ChannelManager& channelManager);

    int32_t SetAudioProcessingModule(
        AudioProcessing* audioProcessingModule);

    int32_t PrepareDemux(const void* audioSamples,
                         uint32_t nSamples,
                         uint8_t  nChannels,
                         uint32_t samplesPerSec,
                         uint16_t totalDelayMS,
                         int32_t  clockDrift,
                         uint16_t currentMicLevel,
                         bool keyPressed);


    int32_t DemuxAndMix();
    
    
    void DemuxAndMix(const int voe_channels[], int number_of_voe_channels);

    int32_t EncodeAndSend();
    
    
    void EncodeAndSend(const int voe_channels[], int number_of_voe_channels);

    uint32_t CaptureLevel() const;

    int32_t StopSend();

    
    void UpdateMuteMicrophoneTime(uint32_t lengthMs);

    
    int RegisterExternalMediaProcessing(VoEMediaProcess* object,
                                        ProcessingTypes type);
    int DeRegisterExternalMediaProcessing(ProcessingTypes type);

    int GetMixingFrequency();

    
    int SetMute(bool enable);

    bool Mute() const;

    int8_t AudioLevel() const;

    int16_t AudioLevelFullRange() const;

    bool IsRecordingCall();

    bool IsRecordingMic();

    int StartPlayingFileAsMicrophone(const char* fileName,
                                     bool loop,
                                     FileFormats format,
                                     int startPosition,
                                     float volumeScaling,
                                     int stopPosition,
                                     const CodecInst* codecInst);

    int StartPlayingFileAsMicrophone(InStream* stream,
                                     FileFormats format,
                                     int startPosition,
                                     float volumeScaling,
                                     int stopPosition,
                                     const CodecInst* codecInst);

    int StopPlayingFileAsMicrophone();

    int IsPlayingFileAsMicrophone() const;

    int ScaleFileAsMicrophonePlayout(float scale);

    int StartRecordingMicrophone(const char* fileName,
                                 const CodecInst* codecInst);

    int StartRecordingMicrophone(OutStream* stream,
                                 const CodecInst* codecInst);

    int StopRecordingMicrophone();

    int StartRecordingCall(const char* fileName, const CodecInst* codecInst);

    int StartRecordingCall(OutStream* stream, const CodecInst* codecInst);

    int StopRecordingCall();

    void SetMixWithMicStatus(bool mix);

    int32_t RegisterVoiceEngineObserver(VoiceEngineObserver& observer);

    virtual ~TransmitMixer();

    
    void OnPeriodicProcess();


    
    void PlayNotification(int32_t id,
                          uint32_t durationMs);

    void RecordNotification(int32_t id,
                            uint32_t durationMs);

    void PlayFileEnded(int32_t id);

    void RecordFileEnded(int32_t id);

#ifdef WEBRTC_VOICE_ENGINE_TYPING_DETECTION
    
    int TimeSinceLastTyping(int &seconds);
    int SetTypingDetectionParameters(int timeWindow,
                                     int costPerTyping,
                                     int reportingThreshold,
                                     int penaltyDecay,
                                     int typeEventDelay);
#endif

  void EnableStereoChannelSwapping(bool enable);
  bool IsStereoChannelSwappingEnabled();

private:
    TransmitMixer(uint32_t instanceId);

    
    
    void GetSendCodecInfo(int* max_sample_rate, int* max_channels);

    int GenerateAudioFrame(const int16_t audioSamples[],
                           int nSamples,
                           int nChannels,
                           int samplesPerSec);
    int32_t RecordAudioToFile(uint32_t mixingFrequency);

    int32_t MixOrReplaceAudioWithFile(
        int mixingFrequency);

    void ProcessAudio(int delay_ms, int clock_drift, int current_mic_level);

#ifdef WEBRTC_VOICE_ENGINE_TYPING_DETECTION
    int TypingDetection(bool keyPressed);
#endif

    
    Statistics* _engineStatisticsPtr;
    ChannelManager* _channelManagerPtr;
    AudioProcessing* audioproc_;
    VoiceEngineObserver* _voiceEngineObserverPtr;
    ProcessThread* _processThreadPtr;

    
    MonitorModule _monitorModule;
    AudioFrame _audioFrame;
    PushResampler resampler_;  
    FilePlayer* _filePlayerPtr;
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
    int32_t _timeActive;
    int32_t _timeSinceLastTyping;
    int32_t _penaltyCounter;
    bool _typingNoiseWarningPending;
    bool _typingNoiseDetected;

    
    int _timeWindow; 
    int _costPerTyping; 
    int _reportingThreshold; 
    int _penaltyDecay; 
    int _typeEventDelay; 

#endif
    bool _saturationWarning;

    int _instanceId;
    bool _mixFileWithMicrophone;
    uint32_t _captureLevel;
    VoEMediaProcess* external_postproc_ptr_;
    VoEMediaProcess* external_preproc_ptr_;
    bool _mute;
    int32_t _remainingMuteMicTimeMs;
    bool stereo_codec_;
    bool swap_stereo_channels_;
};

}  

}  

#endif 

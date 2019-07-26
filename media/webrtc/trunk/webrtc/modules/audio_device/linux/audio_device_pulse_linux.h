









#ifndef WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_PULSE_LINUX_H
#define WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_PULSE_LINUX_H

#include "webrtc/modules/audio_device/audio_device_generic.h"
#include "webrtc/modules/audio_device/linux/audio_mixer_manager_pulse_linux.h"
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"

#include <X11/Xlib.h>
#include <pulse/pulseaudio.h>




#ifndef PA_STREAM_ADJUST_LATENCY
#define PA_STREAM_ADJUST_LATENCY 0x2000U
#endif
#ifndef PA_STREAM_START_MUTED
#define PA_STREAM_START_MUTED 0x1000U
#endif


const uint32_t WEBRTC_PA_REPORT_LATENCY = 1;




const uint32_t WEBRTC_PA_ADJUST_LATENCY_PROTOCOL_VERSION = 13;













const uint32_t WEBRTC_PA_PLAYBACK_LATENCY_MINIMUM_MSECS = 20;



const uint32_t WEBRTC_PA_PLAYBACK_LATENCY_INCREMENT_MSECS = 20;





const uint32_t WEBRTC_PA_PLAYBACK_REQUEST_FACTOR = 2;








const uint32_t WEBRTC_PA_LOW_CAPTURE_LATENCY_MSECS = 10;






const uint32_t WEBRTC_PA_CAPTURE_BUFFER_EXTRA_MSECS = 750;

const uint32_t WEBRTC_PA_MSECS_PER_SEC = 1000;


const int32_t WEBRTC_PA_NO_LATENCY_REQUIREMENTS = -1;


const uint32_t WEBRTC_PA_CAPTURE_BUFFER_LATENCY_ADJUSTMENT = 0;

namespace webrtc
{
class EventWrapper;
class ThreadWrapper;

class AudioDeviceLinuxPulse: public AudioDeviceGeneric
{
public:
    AudioDeviceLinuxPulse(const int32_t id);
    virtual ~AudioDeviceLinuxPulse();

    
    virtual int32_t ActiveAudioLayer(
        AudioDeviceModule::AudioLayer& audioLayer) const OVERRIDE;

    
    virtual int32_t Init() OVERRIDE;
    virtual int32_t Terminate() OVERRIDE;
    virtual bool Initialized() const OVERRIDE;

    
    virtual int16_t PlayoutDevices() OVERRIDE;
    virtual int16_t RecordingDevices() OVERRIDE;
    virtual int32_t PlayoutDeviceName(
        uint16_t index,
        char name[kAdmMaxDeviceNameSize],
        char guid[kAdmMaxGuidSize]) OVERRIDE;
    virtual int32_t RecordingDeviceName(
        uint16_t index,
        char name[kAdmMaxDeviceNameSize],
        char guid[kAdmMaxGuidSize]) OVERRIDE;

    
    virtual int32_t SetPlayoutDevice(uint16_t index) OVERRIDE;
    virtual int32_t SetPlayoutDevice(
        AudioDeviceModule::WindowsDeviceType device) OVERRIDE;
    virtual int32_t SetRecordingDevice(uint16_t index) OVERRIDE;
    virtual int32_t SetRecordingDevice(
        AudioDeviceModule::WindowsDeviceType device) OVERRIDE;

    
    virtual int32_t PlayoutIsAvailable(bool& available) OVERRIDE;
    virtual int32_t InitPlayout() OVERRIDE;
    virtual bool PlayoutIsInitialized() const OVERRIDE;
    virtual int32_t RecordingIsAvailable(bool& available) OVERRIDE;
    virtual int32_t InitRecording() OVERRIDE;
    virtual bool RecordingIsInitialized() const OVERRIDE;

    
    virtual int32_t StartPlayout() OVERRIDE;
    virtual int32_t StopPlayout() OVERRIDE;
    virtual bool Playing() const OVERRIDE;
    virtual int32_t StartRecording() OVERRIDE;
    virtual int32_t StopRecording() OVERRIDE;
    virtual bool Recording() const OVERRIDE;

    
    virtual int32_t SetAGC(bool enable) OVERRIDE;
    virtual bool AGC() const OVERRIDE;

    
    virtual int32_t SetWaveOutVolume(uint16_t volumeLeft,
                                     uint16_t volumeRight) OVERRIDE;
    virtual int32_t WaveOutVolume(uint16_t& volumeLeft,
                                  uint16_t& volumeRight) const OVERRIDE;

    
    virtual int32_t SpeakerIsAvailable(bool& available) OVERRIDE;
    virtual int32_t InitSpeaker() OVERRIDE;
    virtual bool SpeakerIsInitialized() const OVERRIDE;
    virtual int32_t MicrophoneIsAvailable(bool& available) OVERRIDE;
    virtual int32_t InitMicrophone() OVERRIDE;
    virtual bool MicrophoneIsInitialized() const OVERRIDE;

    
    virtual int32_t SpeakerVolumeIsAvailable(bool& available) OVERRIDE;
    virtual int32_t SetSpeakerVolume(uint32_t volume) OVERRIDE;
    virtual int32_t SpeakerVolume(uint32_t& volume) const OVERRIDE;
    virtual int32_t MaxSpeakerVolume(uint32_t& maxVolume) const OVERRIDE;
    virtual int32_t MinSpeakerVolume(uint32_t& minVolume) const OVERRIDE;
    virtual int32_t SpeakerVolumeStepSize(uint16_t& stepSize) const OVERRIDE;

    
    virtual int32_t MicrophoneVolumeIsAvailable(bool& available) OVERRIDE;
    virtual int32_t SetMicrophoneVolume(uint32_t volume) OVERRIDE;
    virtual int32_t MicrophoneVolume(uint32_t& volume) const OVERRIDE;
    virtual int32_t MaxMicrophoneVolume(uint32_t& maxVolume) const OVERRIDE;
    virtual int32_t MinMicrophoneVolume(uint32_t& minVolume) const OVERRIDE;
    virtual int32_t MicrophoneVolumeStepSize(
        uint16_t& stepSize) const OVERRIDE;

    
    virtual int32_t SpeakerMuteIsAvailable(bool& available) OVERRIDE;
    virtual int32_t SetSpeakerMute(bool enable) OVERRIDE;
    virtual int32_t SpeakerMute(bool& enabled) const OVERRIDE;

    
    virtual int32_t MicrophoneMuteIsAvailable(bool& available) OVERRIDE;
    virtual int32_t SetMicrophoneMute(bool enable) OVERRIDE;
    virtual int32_t MicrophoneMute(bool& enabled) const OVERRIDE;

    
    virtual int32_t MicrophoneBoostIsAvailable(bool& available) OVERRIDE;
    virtual int32_t SetMicrophoneBoost(bool enable) OVERRIDE;
    virtual int32_t MicrophoneBoost(bool& enabled) const OVERRIDE;

    
    virtual int32_t StereoPlayoutIsAvailable(bool& available) OVERRIDE;
    virtual int32_t SetStereoPlayout(bool enable) OVERRIDE;
    virtual int32_t StereoPlayout(bool& enabled) const OVERRIDE;
    virtual int32_t StereoRecordingIsAvailable(bool& available) OVERRIDE;
    virtual int32_t SetStereoRecording(bool enable) OVERRIDE;
    virtual int32_t StereoRecording(bool& enabled) const OVERRIDE;

    
    virtual int32_t
        SetPlayoutBuffer(const AudioDeviceModule::BufferType type,
                         uint16_t sizeMS) OVERRIDE;
    virtual int32_t PlayoutBuffer(AudioDeviceModule::BufferType& type,
                                  uint16_t& sizeMS) const OVERRIDE;
    virtual int32_t PlayoutDelay(uint16_t& delayMS) const OVERRIDE;
    virtual int32_t RecordingDelay(uint16_t& delayMS) const OVERRIDE;

    
    virtual int32_t CPULoad(uint16_t& load) const OVERRIDE;

public:
    virtual bool PlayoutWarning() const OVERRIDE;
    virtual bool PlayoutError() const OVERRIDE;
    virtual bool RecordingWarning() const OVERRIDE;
    virtual bool RecordingError() const OVERRIDE;
    virtual void ClearPlayoutWarning() OVERRIDE;
    virtual void ClearPlayoutError() OVERRIDE;
    virtual void ClearRecordingWarning() OVERRIDE;
    virtual void ClearRecordingError() OVERRIDE;

public:
    virtual void AttachAudioBuffer(AudioDeviceBuffer* audioBuffer) OVERRIDE;

private:
    void Lock() EXCLUSIVE_LOCK_FUNCTION(_critSect) {
        _critSect.Enter();
    }
    void UnLock() UNLOCK_FUNCTION(_critSect) {
        _critSect.Leave();
    }
    void WaitForOperationCompletion(pa_operation* paOperation) const;
    void WaitForSuccess(pa_operation* paOperation) const;

private:
    bool KeyPressed() const;

private:
    static void PaContextStateCallback(pa_context *c, void *pThis);
    static void PaSinkInfoCallback(pa_context *c, const pa_sink_info *i,
                                   int eol, void *pThis);
    static void PaSourceInfoCallback(pa_context *c, const pa_source_info *i,
                                     int eol, void *pThis);
    static void PaServerInfoCallback(pa_context *c, const pa_server_info *i,
                                     void *pThis);
    static void PaStreamStateCallback(pa_stream *p, void *pThis);
    void PaContextStateCallbackHandler(pa_context *c);
    void PaSinkInfoCallbackHandler(const pa_sink_info *i, int eol);
    void PaSourceInfoCallbackHandler(const pa_source_info *i, int eol);
    void PaServerInfoCallbackHandler(const pa_server_info *i);
    void PaStreamStateCallbackHandler(pa_stream *p);

    void EnableWriteCallback();
    void DisableWriteCallback();
    static void PaStreamWriteCallback(pa_stream *unused, size_t buffer_space,
                                      void *pThis);
    void PaStreamWriteCallbackHandler(size_t buffer_space);
    static void PaStreamUnderflowCallback(pa_stream *unused, void *pThis);
    void PaStreamUnderflowCallbackHandler();
    void EnableReadCallback();
    void DisableReadCallback();
    static void PaStreamReadCallback(pa_stream *unused1, size_t unused2,
                                     void *pThis);
    void PaStreamReadCallbackHandler();
    static void PaStreamOverflowCallback(pa_stream *unused, void *pThis);
    void PaStreamOverflowCallbackHandler();
    int32_t LatencyUsecs(pa_stream *stream);
    int32_t ReadRecordedData(const void* bufferData, size_t bufferSize);
    int32_t ProcessRecordedData(int8_t *bufferData,
                                uint32_t bufferSizeInSamples,
                                uint32_t recDelay);

    int32_t CheckPulseAudioVersion();
    int32_t InitSamplingFrequency();
    int32_t GetDefaultDeviceInfo(bool recDevice, char* name, uint16_t& index);
    int32_t InitPulseAudio();
    int32_t TerminatePulseAudio();

    void PaLock();
    void PaUnLock();

    static bool RecThreadFunc(void*);
    static bool PlayThreadFunc(void*);
    bool RecThreadProcess();
    bool PlayThreadProcess();

private:
    AudioDeviceBuffer* _ptrAudioBuffer;

    CriticalSectionWrapper& _critSect;
    EventWrapper& _timeEventRec;
    EventWrapper& _timeEventPlay;
    EventWrapper& _recStartEvent;
    EventWrapper& _playStartEvent;

    ThreadWrapper* _ptrThreadPlay;
    ThreadWrapper* _ptrThreadRec;
    uint32_t _recThreadID;
    uint32_t _playThreadID;
    int32_t _id;

    AudioMixerManagerLinuxPulse _mixerManager;

    uint16_t _inputDeviceIndex;
    uint16_t _outputDeviceIndex;
    bool _inputDeviceIsSpecified;
    bool _outputDeviceIsSpecified;

    int sample_rate_hz_;
    uint8_t _recChannels;
    uint8_t _playChannels;

    AudioDeviceModule::BufferType _playBufType;

private:
    bool _initialized;
    bool _recording;
    bool _playing;
    bool _recIsInitialized;
    bool _playIsInitialized;
    bool _startRec;
    bool _stopRec;
    bool _startPlay;
    bool _stopPlay;
    bool _AGC;
    bool update_speaker_volume_at_startup_;

private:
    uint16_t _playBufDelayFixed; 

    uint32_t _sndCardPlayDelay;
    uint32_t _sndCardRecDelay;

    int32_t _writeErrors;
    uint16_t _playWarning;
    uint16_t _playError;
    uint16_t _recWarning;
    uint16_t _recError;

    uint16_t _deviceIndex;
    int16_t _numPlayDevices;
    int16_t _numRecDevices;
    char* _playDeviceName;
    char* _recDeviceName;
    char* _playDisplayDeviceName;
    char* _recDisplayDeviceName;
    char _paServerVersion[32];

    int8_t* _playBuffer;
    size_t _playbackBufferSize;
    size_t _playbackBufferUnused;
    size_t _tempBufferSpace;
    int8_t* _recBuffer;
    size_t _recordBufferSize;
    size_t _recordBufferUsed;
    const void* _tempSampleData;
    size_t _tempSampleDataSize;
    int32_t _configuredLatencyPlay;
    int32_t _configuredLatencyRec;

    
    uint16_t _paDeviceIndex;
    bool _paStateChanged;

    pa_threaded_mainloop* _paMainloop;
    pa_mainloop_api* _paMainloopApi;
    pa_context* _paContext;

    pa_stream* _recStream;
    pa_stream* _playStream;
    uint32_t _recStreamFlags;
    uint32_t _playStreamFlags;
    pa_buffer_attr _playBufferAttr;
    pa_buffer_attr _recBufferAttr;

    char _oldKeyState[32];
    Display* _XDisplay;
};

}

#endif  

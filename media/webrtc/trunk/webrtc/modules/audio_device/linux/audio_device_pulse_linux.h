









#ifndef WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_PULSE_LINUX_H
#define WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_PULSE_LINUX_H

#include "audio_device_generic.h"
#include "audio_mixer_manager_pulse_linux.h"
#include "critical_section_wrapper.h"

#include <pulse/pulseaudio.h>







#ifndef PA_STREAM_ADJUST_LATENCY
#define PA_STREAM_ADJUST_LATENCY 0x2000U
#endif
#ifndef PA_STREAM_START_MUTED
#define PA_STREAM_START_MUTED 0x1000U
#endif


const WebRtc_UWord32 WEBRTC_PA_REPORT_LATENCY = 1;




const WebRtc_UWord32 WEBRTC_PA_ADJUST_LATENCY_PROTOCOL_VERSION = 13;













const WebRtc_UWord32 WEBRTC_PA_PLAYBACK_LATENCY_MINIMUM_MSECS = 20;



const WebRtc_UWord32 WEBRTC_PA_PLAYBACK_LATENCY_INCREMENT_MSECS = 20;





const WebRtc_UWord32 WEBRTC_PA_PLAYBACK_REQUEST_FACTOR = 2;








const WebRtc_UWord32 WEBRTC_PA_LOW_CAPTURE_LATENCY_MSECS = 10;






const WebRtc_UWord32 WEBRTC_PA_CAPTURE_BUFFER_EXTRA_MSECS = 750;

const WebRtc_UWord32 WEBRTC_PA_MSECS_PER_SEC = 1000;


const WebRtc_Word32 WEBRTC_PA_NO_LATENCY_REQUIREMENTS = -1;


const WebRtc_UWord32 WEBRTC_PA_CAPTURE_BUFFER_LATENCY_ADJUSTMENT = 0;

namespace webrtc
{
class EventWrapper;
class ThreadWrapper;

class AudioDeviceLinuxPulse: public AudioDeviceGeneric
{
public:
    AudioDeviceLinuxPulse(const WebRtc_Word32 id);
    ~AudioDeviceLinuxPulse();

    static bool PulseAudioIsSupported();

    
    virtual WebRtc_Word32
        ActiveAudioLayer(AudioDeviceModule::AudioLayer& audioLayer) const;

    
    virtual WebRtc_Word32 Init();
    virtual WebRtc_Word32 Terminate();
    virtual bool Initialized() const;

    
    virtual WebRtc_Word16 PlayoutDevices();
    virtual WebRtc_Word16 RecordingDevices();
    virtual WebRtc_Word32 PlayoutDeviceName(
        WebRtc_UWord16 index,
        char name[kAdmMaxDeviceNameSize],
        char guid[kAdmMaxGuidSize]);
    virtual WebRtc_Word32 RecordingDeviceName(
        WebRtc_UWord16 index,
        char name[kAdmMaxDeviceNameSize],
        char guid[kAdmMaxGuidSize]);

    
    virtual WebRtc_Word32 SetPlayoutDevice(WebRtc_UWord16 index);
    virtual WebRtc_Word32 SetPlayoutDevice(
        AudioDeviceModule::WindowsDeviceType device);
    virtual WebRtc_Word32 SetRecordingDevice(WebRtc_UWord16 index);
    virtual WebRtc_Word32 SetRecordingDevice(
        AudioDeviceModule::WindowsDeviceType device);

    
    virtual WebRtc_Word32 PlayoutIsAvailable(bool& available);
    virtual WebRtc_Word32 InitPlayout();
    virtual bool PlayoutIsInitialized() const;
    virtual WebRtc_Word32 RecordingIsAvailable(bool& available);
    virtual WebRtc_Word32 InitRecording();
    virtual bool RecordingIsInitialized() const;

    
    virtual WebRtc_Word32 StartPlayout();
    virtual WebRtc_Word32 StopPlayout();
    virtual bool Playing() const;
    virtual WebRtc_Word32 StartRecording();
    virtual WebRtc_Word32 StopRecording();
    virtual bool Recording() const;

    
    virtual WebRtc_Word32 SetAGC(bool enable);
    virtual bool AGC() const;

    
    virtual WebRtc_Word32 SetWaveOutVolume(WebRtc_UWord16 volumeLeft,
                                           WebRtc_UWord16 volumeRight);
    virtual WebRtc_Word32 WaveOutVolume(WebRtc_UWord16& volumeLeft,
                                        WebRtc_UWord16& volumeRight) const;

    
    virtual WebRtc_Word32 SpeakerIsAvailable(bool& available);
    virtual WebRtc_Word32 InitSpeaker();
    virtual bool SpeakerIsInitialized() const;
    virtual WebRtc_Word32 MicrophoneIsAvailable(bool& available);
    virtual WebRtc_Word32 InitMicrophone();
    virtual bool MicrophoneIsInitialized() const;

    
    virtual WebRtc_Word32 SpeakerVolumeIsAvailable(bool& available);
    virtual WebRtc_Word32 SetSpeakerVolume(WebRtc_UWord32 volume);
    virtual WebRtc_Word32 SpeakerVolume(WebRtc_UWord32& volume) const;
    virtual WebRtc_Word32 MaxSpeakerVolume(WebRtc_UWord32& maxVolume) const;
    virtual WebRtc_Word32 MinSpeakerVolume(WebRtc_UWord32& minVolume) const;
    virtual WebRtc_Word32 SpeakerVolumeStepSize(WebRtc_UWord16& stepSize) const;

    
    virtual WebRtc_Word32 MicrophoneVolumeIsAvailable(bool& available);
    virtual WebRtc_Word32 SetMicrophoneVolume(WebRtc_UWord32 volume);
    virtual WebRtc_Word32 MicrophoneVolume(WebRtc_UWord32& volume) const;
    virtual WebRtc_Word32 MaxMicrophoneVolume(WebRtc_UWord32& maxVolume) const;
    virtual WebRtc_Word32 MinMicrophoneVolume(WebRtc_UWord32& minVolume) const;
    virtual WebRtc_Word32 MicrophoneVolumeStepSize(
        WebRtc_UWord16& stepSize) const;

    
    virtual WebRtc_Word32 SpeakerMuteIsAvailable(bool& available);
    virtual WebRtc_Word32 SetSpeakerMute(bool enable);
    virtual WebRtc_Word32 SpeakerMute(bool& enabled) const;

    
    virtual WebRtc_Word32 MicrophoneMuteIsAvailable(bool& available);
    virtual WebRtc_Word32 SetMicrophoneMute(bool enable);
    virtual WebRtc_Word32 MicrophoneMute(bool& enabled) const;

    
    virtual WebRtc_Word32 MicrophoneBoostIsAvailable(bool& available);
    virtual WebRtc_Word32 SetMicrophoneBoost(bool enable);
    virtual WebRtc_Word32 MicrophoneBoost(bool& enabled) const;

    
    virtual WebRtc_Word32 StereoPlayoutIsAvailable(bool& available);
    virtual WebRtc_Word32 SetStereoPlayout(bool enable);
    virtual WebRtc_Word32 StereoPlayout(bool& enabled) const;
    virtual WebRtc_Word32 StereoRecordingIsAvailable(bool& available);
    virtual WebRtc_Word32 SetStereoRecording(bool enable);
    virtual WebRtc_Word32 StereoRecording(bool& enabled) const;

    
    virtual WebRtc_Word32
        SetPlayoutBuffer(const AudioDeviceModule::BufferType type,
                         WebRtc_UWord16 sizeMS);
    virtual WebRtc_Word32 PlayoutBuffer(AudioDeviceModule::BufferType& type,
                                        WebRtc_UWord16& sizeMS) const;
    virtual WebRtc_Word32 PlayoutDelay(WebRtc_UWord16& delayMS) const;
    virtual WebRtc_Word32 RecordingDelay(WebRtc_UWord16& delayMS) const;

    
    virtual WebRtc_Word32 CPULoad(WebRtc_UWord16& load) const;

public:
    virtual bool PlayoutWarning() const;
    virtual bool PlayoutError() const;
    virtual bool RecordingWarning() const;
    virtual bool RecordingError() const;
    virtual void ClearPlayoutWarning();
    virtual void ClearPlayoutError();
    virtual void ClearRecordingWarning();
    virtual void ClearRecordingError();

public:
    virtual void AttachAudioBuffer(AudioDeviceBuffer* audioBuffer);

private:
    void Lock()
    {
        _critSect.Enter();
    }
    ;
    void UnLock()
    {
        _critSect.Leave();
    }
    ;
    void WaitForOperationCompletion(pa_operation* paOperation) const;
    void WaitForSuccess(pa_operation* paOperation) const;

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
    WebRtc_Word32 LatencyUsecs(pa_stream *stream);
    WebRtc_Word32 ReadRecordedData(const void* bufferData, size_t bufferSize);
    WebRtc_Word32 ProcessRecordedData(WebRtc_Word8 *bufferData,
                                      WebRtc_UWord32 bufferSizeInSamples,
                                      WebRtc_UWord32 recDelay);

    WebRtc_Word32 CheckPulseAudioVersion();
    WebRtc_Word32 InitSamplingFrequency();
    WebRtc_Word32 GetDefaultDeviceInfo(bool recDevice, char* name,
                                       WebRtc_UWord16& index);
    WebRtc_Word32 InitPulseAudio();
    WebRtc_Word32 TerminatePulseAudio();

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
    WebRtc_UWord32 _recThreadID;
    WebRtc_UWord32 _playThreadID;
    WebRtc_Word32 _id;

    AudioMixerManagerLinuxPulse _mixerManager;

    WebRtc_UWord16 _inputDeviceIndex;
    WebRtc_UWord16 _outputDeviceIndex;
    bool _inputDeviceIsSpecified;
    bool _outputDeviceIsSpecified;

    WebRtc_UWord32 _samplingFreq;
    WebRtc_UWord8 _recChannels;
    WebRtc_UWord8 _playChannels;

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
    WebRtc_UWord16 _playBufDelayFixed; 

    WebRtc_UWord32 _sndCardPlayDelay;
    WebRtc_UWord32 _sndCardRecDelay;

    WebRtc_Word32 _writeErrors;
    WebRtc_UWord16 _playWarning;
    WebRtc_UWord16 _playError;
    WebRtc_UWord16 _recWarning;
    WebRtc_UWord16 _recError;

    WebRtc_UWord16 _deviceIndex;
    WebRtc_Word16 _numPlayDevices;
    WebRtc_Word16 _numRecDevices;
    char* _playDeviceName;
    char* _recDeviceName;
    char* _playDisplayDeviceName;
    char* _recDisplayDeviceName;
    char _paServerVersion[32];

    WebRtc_Word8* _playBuffer;
    size_t _playbackBufferSize;
    size_t _playbackBufferUnused;
    size_t _tempBufferSpace;
    WebRtc_Word8* _recBuffer;
    size_t _recordBufferSize;
    size_t _recordBufferUsed;
    const void* _tempSampleData;
    size_t _tempSampleDataSize;
    WebRtc_Word32 _configuredLatencyPlay;
    WebRtc_Word32 _configuredLatencyRec;

    
    WebRtc_UWord16 _paDeviceIndex;
    bool _paStateChanged;

    pa_threaded_mainloop* _paMainloop;
    pa_mainloop_api* _paMainloopApi;
    pa_context* _paContext;

    pa_stream* _recStream;
    pa_stream* _playStream;
    WebRtc_UWord32 _recStreamFlags;
    WebRtc_UWord32 _playStreamFlags;
    pa_buffer_attr _playBufferAttr;
    pa_buffer_attr _recBufferAttr;
};

}

#endif  

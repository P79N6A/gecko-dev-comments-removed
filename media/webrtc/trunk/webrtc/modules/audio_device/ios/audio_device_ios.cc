









#include <AudioToolbox/AudioServices.h>  

#include "audio_device_ios.h"

#include "trace.h"
#include "thread_wrapper.h"

namespace webrtc {
AudioDeviceIPhone::AudioDeviceIPhone(const WebRtc_Word32 id)
    :
    _ptrAudioBuffer(NULL),
    _critSect(*CriticalSectionWrapper::CreateCriticalSection()),
    _captureWorkerThread(NULL),
    _captureWorkerThreadId(0),
    _id(id),
    _auRemoteIO(NULL),
    _initialized(false),
    _isShutDown(false),
    _recording(false),
    _playing(false),
    _recIsInitialized(false),
    _playIsInitialized(false),
    _recordingDeviceIsSpecified(false),
    _playoutDeviceIsSpecified(false),
    _micIsInitialized(false),
    _speakerIsInitialized(false),
    _AGC(false),
    _adbSampFreq(0),
    _recordingDelay(0),
    _playoutDelay(0),
    _playoutDelayMeasurementCounter(9999),
    _recordingDelayHWAndOS(0),
    _recordingDelayMeasurementCounter(9999),
    _playWarning(0),
    _playError(0),
    _recWarning(0),
    _recError(0),
    _playoutBufferUsed(0),
    _recordingCurrentSeq(0),
    _recordingBufferTotalSize(0) {
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, id,
                 "%s created", __FUNCTION__);

    memset(_playoutBuffer, 0, sizeof(_playoutBuffer));
    memset(_recordingBuffer, 0, sizeof(_recordingBuffer));
    memset(_recordingLength, 0, sizeof(_recordingLength));
    memset(_recordingSeqNumber, 0, sizeof(_recordingSeqNumber));
}

AudioDeviceIPhone::~AudioDeviceIPhone() {
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id,
                 "%s destroyed", __FUNCTION__);

    Terminate();

    delete &_critSect;
}






void AudioDeviceIPhone::AttachAudioBuffer(AudioDeviceBuffer* audioBuffer) {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    CriticalSectionScoped lock(&_critSect);

    _ptrAudioBuffer = audioBuffer;

    
    _ptrAudioBuffer->SetRecordingSampleRate(ENGINE_REC_BUF_SIZE_IN_SAMPLES);
    _ptrAudioBuffer->SetPlayoutSampleRate(ENGINE_PLAY_BUF_SIZE_IN_SAMPLES);
    _ptrAudioBuffer->SetRecordingChannels(N_REC_CHANNELS);
    _ptrAudioBuffer->SetPlayoutChannels(N_PLAY_CHANNELS);
}

WebRtc_Word32 AudioDeviceIPhone::ActiveAudioLayer(
    AudioDeviceModule::AudioLayer& audioLayer) const {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);
    audioLayer = AudioDeviceModule::kPlatformDefaultAudio;
    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::Init() {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    CriticalSectionScoped lock(&_critSect);

    if (_initialized) {
        return 0;
    }

    _isShutDown = false;

    
    if (_captureWorkerThread == NULL) {
        _captureWorkerThread
            = ThreadWrapper::CreateThread(RunCapture, this, kRealtimePriority,
                                          "CaptureWorkerThread");

        if (_captureWorkerThread == NULL) {
            WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice,
                         _id, "CreateThread() error");
            return -1;
        }

        unsigned int threadID(0);
        bool res = _captureWorkerThread->Start(threadID);
        _captureWorkerThreadId = static_cast<WebRtc_UWord32>(threadID);
        WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice,
                     _id, "CaptureWorkerThread started (res=%d)", res);
    } else {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice,
                     _id, "Thread already created");
    }

    
    Float64 sampleRate(16000.0);
    OSStatus result = AudioSessionSetProperty(
        kAudioSessionProperty_PreferredHardwareSampleRate,
        sizeof(sampleRate), &sampleRate);
    if (0 != result) {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     "Could not set preferred sample rate (result=%d)", result);
    }

    _playWarning = 0;
    _playError = 0;
    _recWarning = 0;
    _recError = 0;

    _initialized = true;

    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::Terminate() {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    if (!_initialized) {
        return 0;
    }


    
    if (_captureWorkerThread != NULL) {
        WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice,
                     _id, "Stopping CaptureWorkerThread");
        bool res = _captureWorkerThread->Stop();
        WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice,
                     _id, "CaptureWorkerThread stopped (res=%d)", res);
        delete _captureWorkerThread;
        _captureWorkerThread = NULL;
    }

    
    ShutdownPlayOrRecord();

    _isShutDown = true;
    _initialized = false;
    _speakerIsInitialized = false;
    _micIsInitialized = false;
    _playoutDeviceIsSpecified = false;
    _recordingDeviceIsSpecified = false;
    return 0;
}

bool AudioDeviceIPhone::Initialized() const {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);
    return (_initialized);
}

WebRtc_Word32 AudioDeviceIPhone::SpeakerIsAvailable(bool& available) {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    
    available = true;
    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::InitSpeaker() {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    CriticalSectionScoped lock(&_critSect);

    if (!_initialized) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice,
                     _id, "  Not initialized");
        return -1;
    }

    if (_playing) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice,
                     _id, "  Cannot init speaker when playing");
        return -1;
    }

    if (!_playoutDeviceIsSpecified) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice,
                     _id, "  Playout device is not specified");
        return -1;
    }

    
    _speakerIsInitialized = true;

    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::MicrophoneIsAvailable(bool& available) {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    available = false;

    OSStatus result = -1;
    UInt32 channel = 0;
    UInt32 size = sizeof(channel);
    result = AudioSessionGetProperty(kAudioSessionProperty_AudioInputAvailable,
                                     &size, &channel);
    if (channel != 0) {
        
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice,
                     _id, "  API call not supported on this version");
        available = true;
        return 0;
    }

    available = (channel == 0) ? false : true;

    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::InitMicrophone() {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    CriticalSectionScoped lock(&_critSect);

    if (!_initialized) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice,
                     _id, "  Not initialized");
        return -1;
    }

    if (_recording) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice,
                     _id, "  Cannot init mic when recording");
        return -1;
    }

    if (!_recordingDeviceIsSpecified) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice,
                     _id, "  Recording device is not specified");
        return -1;
    }

    

    _micIsInitialized = true;

    return 0;
}

bool AudioDeviceIPhone::SpeakerIsInitialized() const {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);
    return _speakerIsInitialized;
}

bool AudioDeviceIPhone::MicrophoneIsInitialized() const {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);
    return _micIsInitialized;
}

WebRtc_Word32 AudioDeviceIPhone::SpeakerVolumeIsAvailable(bool& available) {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    available = false;  

    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::SetSpeakerVolume(WebRtc_UWord32 volume) {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "AudioDeviceIPhone::SetSpeakerVolume(volume=%u)", volume);

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

WebRtc_Word32 AudioDeviceIPhone::SpeakerVolume(WebRtc_UWord32& volume) const {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

WebRtc_Word32
    AudioDeviceIPhone::SetWaveOutVolume(WebRtc_UWord16 volumeLeft,
                                        WebRtc_UWord16 volumeRight) {
    WEBRTC_TRACE(
        kTraceModuleCall,
        kTraceAudioDevice,
        _id,
        "AudioDeviceIPhone::SetWaveOutVolume(volumeLeft=%u, volumeRight=%u)",
        volumeLeft, volumeRight);

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");

    return -1;
}

WebRtc_Word32
AudioDeviceIPhone::WaveOutVolume(WebRtc_UWord16& ,
                                 WebRtc_UWord16& ) const {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

WebRtc_Word32
    AudioDeviceIPhone::MaxSpeakerVolume(WebRtc_UWord32& maxVolume) const {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

WebRtc_Word32 AudioDeviceIPhone::MinSpeakerVolume(
    WebRtc_UWord32& minVolume) const {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

WebRtc_Word32
    AudioDeviceIPhone::SpeakerVolumeStepSize(WebRtc_UWord16& stepSize) const {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

WebRtc_Word32 AudioDeviceIPhone::SpeakerMuteIsAvailable(bool& available) {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    available = false;  

    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::SetSpeakerMute(bool enable) {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

WebRtc_Word32 AudioDeviceIPhone::SpeakerMute(bool& enabled) const {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

WebRtc_Word32 AudioDeviceIPhone::MicrophoneMuteIsAvailable(bool& available) {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    available = false;  

    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::SetMicrophoneMute(bool enable) {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

WebRtc_Word32 AudioDeviceIPhone::MicrophoneMute(bool& enabled) const {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

WebRtc_Word32 AudioDeviceIPhone::MicrophoneBoostIsAvailable(bool& available) {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    available = false;  

    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::SetMicrophoneBoost(bool enable) {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "AudioDeviceIPhone::SetMicrophoneBoost(enable=%u)", enable);

    if (!_micIsInitialized) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Microphone not initialized");
        return -1;
    }

    if (enable) {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  SetMicrophoneBoost cannot be enabled on this platform");
        return -1;
    }

    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::MicrophoneBoost(bool& enabled) const {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);
    if (!_micIsInitialized) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Microphone not initialized");
        return -1;
    }

    enabled = false;

    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::StereoRecordingIsAvailable(bool& available) {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    available = false;  

    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::SetStereoRecording(bool enable) {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "AudioDeviceIPhone::SetStereoRecording(enable=%u)", enable);

    if (enable) {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     " Stereo recording is not supported on this platform");
        return -1;
    }
    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::StereoRecording(bool& enabled) const {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    enabled = false;
    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::StereoPlayoutIsAvailable(bool& available) {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    available = false;  

    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::SetStereoPlayout(bool enable) {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "AudioDeviceIPhone::SetStereoPlayout(enable=%u)", enable);

    if (enable) {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     " Stereo playout is not supported on this platform");
        return -1;
    }
    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::StereoPlayout(bool& enabled) const {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    enabled = false;
    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::SetAGC(bool enable) {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "AudioDeviceIPhone::SetAGC(enable=%d)", enable);

    _AGC = enable;

    return 0;
}

bool AudioDeviceIPhone::AGC() const {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    return _AGC;
}

WebRtc_Word32 AudioDeviceIPhone::MicrophoneVolumeIsAvailable(bool& available) {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    available = false;  

    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::SetMicrophoneVolume(WebRtc_UWord32 volume) {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "AudioDeviceIPhone::SetMicrophoneVolume(volume=%u)", volume);

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

WebRtc_Word32
    AudioDeviceIPhone::MicrophoneVolume(WebRtc_UWord32& volume) const {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

WebRtc_Word32
    AudioDeviceIPhone::MaxMicrophoneVolume(WebRtc_UWord32& maxVolume) const {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

WebRtc_Word32
    AudioDeviceIPhone::MinMicrophoneVolume(WebRtc_UWord32& minVolume) const {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

WebRtc_Word32
    AudioDeviceIPhone::MicrophoneVolumeStepSize(
                                            WebRtc_UWord16& stepSize) const {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

WebRtc_Word16 AudioDeviceIPhone::PlayoutDevices() {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    return (WebRtc_Word16)1;
}

WebRtc_Word32 AudioDeviceIPhone::SetPlayoutDevice(WebRtc_UWord16 index) {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "AudioDeviceIPhone::SetPlayoutDevice(index=%u)", index);

    if (_playIsInitialized) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Playout already initialized");
        return -1;
    }

    if (index !=0) {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  SetPlayoutDevice invalid index");
        return -1;
    }
    _playoutDeviceIsSpecified = true;

    return 0;
}

WebRtc_Word32
    AudioDeviceIPhone::SetPlayoutDevice(AudioDeviceModule::WindowsDeviceType) {
    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "WindowsDeviceType not supported");
    return -1;
}

WebRtc_Word32
    AudioDeviceIPhone::PlayoutDeviceName(WebRtc_UWord16 index,
                                         char name[kAdmMaxDeviceNameSize],
                                         char guid[kAdmMaxGuidSize]) {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "AudioDeviceIPhone::PlayoutDeviceName(index=%u)", index);

    if (index != 0) {
        return -1;
    }
    
    memset(name, 0, kAdmMaxDeviceNameSize);
    if (guid != NULL) {
        memset(guid, 0, kAdmMaxGuidSize);
    }

    return 0;
}

WebRtc_Word32
    AudioDeviceIPhone::RecordingDeviceName(WebRtc_UWord16 index,
                                           char name[kAdmMaxDeviceNameSize],
                                           char guid[kAdmMaxGuidSize]) {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "AudioDeviceIPhone::RecordingDeviceName(index=%u)", index);

    if (index != 0) {
        return -1;
    }
    
    memset(name, 0, kAdmMaxDeviceNameSize);
    if (guid != NULL) {
        memset(guid, 0, kAdmMaxGuidSize);
    }

    return 0;
}

WebRtc_Word16 AudioDeviceIPhone::RecordingDevices() {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    return (WebRtc_Word16)1;
}

WebRtc_Word32 AudioDeviceIPhone::SetRecordingDevice(WebRtc_UWord16 index) {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "AudioDeviceIPhone::SetRecordingDevice(index=%u)", index);

    if (_recIsInitialized) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Recording already initialized");
        return -1;
    }

    if (index !=0) {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  SetRecordingDevice invalid index");
        return -1;
    }

    _recordingDeviceIsSpecified = true;

    return 0;
}

WebRtc_Word32
    AudioDeviceIPhone::SetRecordingDevice(
                                        AudioDeviceModule::WindowsDeviceType) {
    WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                 "WindowsDeviceType not supported");
    return -1;
}









WebRtc_Word32 AudioDeviceIPhone::SetLoudspeakerStatus(bool enable) {
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "AudioDeviceIPhone::SetLoudspeakerStatus(enable=%d)", enable);

    UInt32 doChangeDefaultRoute = enable ? 1 : 0;
    OSStatus err = AudioSessionSetProperty(
        kAudioSessionProperty_OverrideCategoryDefaultToSpeaker,
        sizeof(doChangeDefaultRoute), &doChangeDefaultRoute);

    if (err != noErr) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
            "Error changing default output route " \
            "(only available on iOS 3.1 or later)");
        return -1;
    }

    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::GetLoudspeakerStatus(bool &enabled) const {
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "AudioDeviceIPhone::SetLoudspeakerStatus(enabled=?)");

    UInt32 route(0);
    UInt32 size = sizeof(route);
    OSStatus err = AudioSessionGetProperty(
        kAudioSessionProperty_OverrideCategoryDefaultToSpeaker,
        &size, &route);
    if (err != noErr) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
            "Error changing default output route " \
            "(only available on iOS 3.1 or later)");
        return -1;
    }

    enabled = route == 1 ? true: false;

    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::PlayoutIsAvailable(bool& available) {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    available = false;

    
    WebRtc_Word32 res = InitPlayout();

    
    StopPlayout();

    if (res != -1) {
        available = true;
    }

    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::RecordingIsAvailable(bool& available) {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    available = false;

    
    WebRtc_Word32 res = InitRecording();

    
    StopRecording();

    if (res != -1) {
        available = true;
    }

    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::InitPlayout() {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    CriticalSectionScoped lock(&_critSect);

    if (!_initialized) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "  Not initialized");
        return -1;
    }

    if (_playing) {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  Playout already started");
        return -1;
    }

    if (_playIsInitialized) {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     "  Playout already initialized");
        return 0;
    }

    if (!_playoutDeviceIsSpecified) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Playout device is not specified");
        return -1;
    }

    
    if (InitSpeaker() == -1) {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  InitSpeaker() failed");
    }

    _playIsInitialized = true;

    if (!_recIsInitialized) {
        
        if (InitPlayOrRecord() == -1) {
            
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "  InitPlayOrRecord() failed");
        }
    } else {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
        "  Recording already initialized - InitPlayOrRecord() not called");
    }

    return 0;
}

bool AudioDeviceIPhone::PlayoutIsInitialized() const {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id, "%s", __FUNCTION__);
    return (_playIsInitialized);
}

WebRtc_Word32 AudioDeviceIPhone::InitRecording() {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    CriticalSectionScoped lock(&_critSect);

    if (!_initialized) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Not initialized");
        return -1;
    }

    if (_recording) {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  Recording already started");
        return -1;
    }

    if (_recIsInitialized) {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     "  Recording already initialized");
        return 0;
    }

    if (!_recordingDeviceIsSpecified) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Recording device is not specified");
        return -1;
    }

    
    if (InitMicrophone() == -1) {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  InitMicrophone() failed");
    }

    _recIsInitialized = true;

    if (!_playIsInitialized) {
        
        if (InitPlayOrRecord() == -1) {
            
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "  InitPlayOrRecord() failed");
        }
    } else {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     "  Playout already initialized - InitPlayOrRecord() " \
                     "not called");
    }

    return 0;
}

bool AudioDeviceIPhone::RecordingIsInitialized() const {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id, "%s", __FUNCTION__);
    return (_recIsInitialized);
}

WebRtc_Word32 AudioDeviceIPhone::StartRecording() {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    CriticalSectionScoped lock(&_critSect);

    if (!_recIsInitialized) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Recording not initialized");
        return -1;
    }

    if (_recording) {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     "  Recording already started");
        return 0;
    }

    
    memset(_recordingBuffer, 0, sizeof(_recordingBuffer));
    memset(_recordingLength, 0, sizeof(_recordingLength));
    memset(_recordingSeqNumber, 0, sizeof(_recordingSeqNumber));
    _recordingCurrentSeq = 0;
    _recordingBufferTotalSize = 0;
    _recordingDelay = 0;
    _recordingDelayHWAndOS = 0;
    
    _recordingDelayMeasurementCounter = 9999;
    _recWarning = 0;
    _recError = 0;

    if (!_playing) {
        
        WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                     "  Starting AU Remote IO");
        OSStatus result = AudioOutputUnitStart(_auRemoteIO);
        if (0 != result) {
            WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                         "  Error starting AU Remote IO (result=%d)", result);
            return -1;
        }
    }

    _recording = true;

    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::StopRecording() {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    CriticalSectionScoped lock(&_critSect);

    if (!_recIsInitialized) {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     "  Recording is not initialized");
        return 0;
    }

    _recording = false;

    if (!_playing) {
        
        ShutdownPlayOrRecord();
    }

    _recIsInitialized = false;
    _micIsInitialized = false;

    return 0;
}

bool AudioDeviceIPhone::Recording() const {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id, "%s", __FUNCTION__);
    return (_recording);
}

WebRtc_Word32 AudioDeviceIPhone::StartPlayout() {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    
    
    
    CriticalSectionScoped lock(&_critSect);

    if (!_playIsInitialized) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Playout not initialized");
        return -1;
    }

    if (_playing) {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     "  Playing already started");
        return 0;
    }

    
    memset(_playoutBuffer, 0, sizeof(_playoutBuffer));
    _playoutBufferUsed = 0;
    _playoutDelay = 0;
    
    _playoutDelayMeasurementCounter = 9999;
    _playWarning = 0;
    _playError = 0;

    if (!_recording) {
        
        WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                     "  Starting AU Remote IO");
        OSStatus result = AudioOutputUnitStart(_auRemoteIO);
        if (0 != result) {
            WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                         "  Error starting AU Remote IO (result=%d)", result);
            return -1;
        }
    }

    _playing = true;

    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::StopPlayout() {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    CriticalSectionScoped lock(&_critSect);

    if (!_playIsInitialized) {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     "  Playout is not initialized");
        return 0;
    }

    _playing = false;

    if (!_recording) {
        
        ShutdownPlayOrRecord();
    }

    _playIsInitialized = false;
    _speakerIsInitialized = false;

    return 0;
}

bool AudioDeviceIPhone::Playing() const {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);
    return (_playing);
}








WebRtc_Word32 AudioDeviceIPhone::ResetAudioDevice() {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    CriticalSectionScoped lock(&_critSect);

    if (!_playIsInitialized && !_recIsInitialized) {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     "  Playout or recording not initialized, doing nothing");
        return 0;  
    }

    
    bool initPlay = _playIsInitialized;
    bool play = _playing;
    bool initRec = _recIsInitialized;
    bool rec = _recording;

    int res(0);

    
    WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                 "  Stopping playout and recording");
    res += StopPlayout();
    res += StopRecording();

    
    WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                 "  Restarting playout and recording (%d, %d, %d, %d)",
                 initPlay, play, initRec, rec);
    if (initPlay) res += InitPlayout();
    if (initRec)  res += InitRecording();
    if (play)     res += StartPlayout();
    if (rec)      res += StartRecording();

    if (0 != res) {
        
        return -1;
    }

    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::PlayoutDelay(WebRtc_UWord16& delayMS) const {
    delayMS = _playoutDelay;
    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::RecordingDelay(WebRtc_UWord16& delayMS) const {
    delayMS = _recordingDelay;
    return 0;
}

WebRtc_Word32
    AudioDeviceIPhone::SetPlayoutBuffer(
                                    const AudioDeviceModule::BufferType type,
                                    WebRtc_UWord16 sizeMS) {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "AudioDeviceIPhone::SetPlayoutBuffer(type=%u, sizeMS=%u)",
                 type, sizeMS);

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

WebRtc_Word32
    AudioDeviceIPhone::PlayoutBuffer(AudioDeviceModule::BufferType& type,
                                     WebRtc_UWord16& sizeMS) const {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    type = AudioDeviceModule::kAdaptiveBufferSize;

    sizeMS = _playoutDelay;

    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::CPULoad(WebRtc_UWord16& ) const {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

bool AudioDeviceIPhone::PlayoutWarning() const {
    return (_playWarning > 0);
}

bool AudioDeviceIPhone::PlayoutError() const {
    return (_playError > 0);
}

bool AudioDeviceIPhone::RecordingWarning() const {
    return (_recWarning > 0);
}

bool AudioDeviceIPhone::RecordingError() const {
    return (_recError > 0);
}

void AudioDeviceIPhone::ClearPlayoutWarning() {
    _playWarning = 0;
}

void AudioDeviceIPhone::ClearPlayoutError() {
    _playError = 0;
}

void AudioDeviceIPhone::ClearRecordingWarning() {
    _recWarning = 0;
}

void AudioDeviceIPhone::ClearRecordingError() {
    _recError = 0;
}





WebRtc_Word32 AudioDeviceIPhone::InitPlayOrRecord() {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    OSStatus result = -1;

    
    if (NULL != _auRemoteIO) {
        
        
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     "  Already initialized");
        
        return 0;
    }

    
    AudioComponentDescription desc;
    AudioComponent comp;

    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_RemoteIO;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;

    comp = AudioComponentFindNext(NULL, &desc);
    if (NULL == comp) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Could not find audio component for AU Remote IO");
        return -1;
    }

    result = AudioComponentInstanceNew(comp, &_auRemoteIO);
    if (0 != result) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Could not create AU Remote IO instance (result=%d)",
                     result);
        return -1;
    }

    
    

    
    

    

    
    
    

    UInt32 enableIO = 1;
    result = AudioUnitSetProperty(_auRemoteIO,
                                  kAudioOutputUnitProperty_EnableIO,
                                  kAudioUnitScope_Input,
                                  1,  
                                  &enableIO,
                                  sizeof(enableIO));
    if (0 != result) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Could not enable IO on input (result=%d)", result);
    }

    result = AudioUnitSetProperty(_auRemoteIO,
                                  kAudioOutputUnitProperty_EnableIO,
                                  kAudioUnitScope_Output,
                                  0,   
                                  &enableIO,
                                  sizeof(enableIO));
    if (0 != result) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Could not enable IO on output (result=%d)", result);
    }

    
    UInt32 flag = 0;
    result = AudioUnitSetProperty(
        _auRemoteIO, kAudioUnitProperty_ShouldAllocateBuffer,
        kAudioUnitScope_Output,  1, &flag, sizeof(flag));
    if (0 != result) {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  Could not disable AU buffer allocation (result=%d)",
                     result);
        
    }

    
    result = AudioUnitInitialize(_auRemoteIO);
    if (0 != result) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Could not init AU Remote IO (result=%d)", result);
    }

    
    AudioStreamBasicDescription playoutDesc;
    UInt32 size = sizeof(playoutDesc);
    result = AudioUnitGetProperty(_auRemoteIO, kAudioUnitProperty_StreamFormat,
                                  kAudioUnitScope_Output, 0, &playoutDesc,
                                  &size);
    if (0 != result) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
            "  Could not get stream format AU Remote IO out/0 (result=%d)",
            result);
    }
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "  AU Remote IO playout opened in sampling rate %f",
                 playoutDesc.mSampleRate);

    
    
    if ((playoutDesc.mSampleRate > 44090.0)
        && (playoutDesc.mSampleRate < 44110.0)) {
        _adbSampFreq = 44000;
    } else if ((playoutDesc.mSampleRate > 15990.0)
               && (playoutDesc.mSampleRate < 16010.0)) {
        _adbSampFreq = 16000;
    } else if ((playoutDesc.mSampleRate > 7990.0)
               && (playoutDesc.mSampleRate < 8010.0)) {
        _adbSampFreq = 8000;
    } else {
        _adbSampFreq = 0;
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
            "  AU Remote IO out/0 opened in unknown sampling rate (%f)",
            playoutDesc.mSampleRate);
        
    }

    
    
    if (_ptrAudioBuffer->SetRecordingSampleRate(_adbSampFreq) < 0) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
            "  Could not set audio device buffer recording sampling rate (%d)",
            _adbSampFreq);
    }

    if (_ptrAudioBuffer->SetPlayoutSampleRate(_adbSampFreq) < 0) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
            "  Could not set audio device buffer playout sampling rate (%d)",
            _adbSampFreq);
    }

    
    playoutDesc.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger
                               | kLinearPCMFormatFlagIsPacked
                               | kLinearPCMFormatFlagIsNonInterleaved;
    playoutDesc.mBytesPerPacket = 2;
    playoutDesc.mFramesPerPacket = 1;
    playoutDesc.mBytesPerFrame = 2;
    playoutDesc.mChannelsPerFrame = 1;
    playoutDesc.mBitsPerChannel = 16;
    result = AudioUnitSetProperty(_auRemoteIO, kAudioUnitProperty_StreamFormat,
                                  kAudioUnitScope_Input, 0, &playoutDesc, size);
    if (0 != result) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
            "  Could not set stream format AU Remote IO in/0 (result=%d)",
            result);
    }

    
    AudioStreamBasicDescription recordingDesc;
    size = sizeof(recordingDesc);
    result = AudioUnitGetProperty(_auRemoteIO, kAudioUnitProperty_StreamFormat,
                                  kAudioUnitScope_Input, 1, &recordingDesc,
                                  &size);
    if (0 != result) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
            "  Could not get stream format AU Remote IO in/1 (result=%d)",
            result);
    }
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "  AU Remote IO recording opened in sampling rate %f",
                 recordingDesc.mSampleRate);

    if (static_cast<int>(playoutDesc.mSampleRate)
        != static_cast<int>(recordingDesc.mSampleRate)) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
            "  AU Remote IO recording and playout opened " \
                     "in different sampling rates");
        
        
    }

    
    recordingDesc.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger
                                 | kLinearPCMFormatFlagIsPacked
                                 | kLinearPCMFormatFlagIsNonInterleaved;

    recordingDesc.mBytesPerPacket = 2;
    recordingDesc.mFramesPerPacket = 1;
    recordingDesc.mBytesPerFrame = 2;
    recordingDesc.mChannelsPerFrame = 1;
    recordingDesc.mBitsPerChannel = 16;
    result = AudioUnitSetProperty(_auRemoteIO, kAudioUnitProperty_StreamFormat,
                                  kAudioUnitScope_Output, 1, &recordingDesc,
                                  size);
    if (0 != result) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
            "  Could not set stream format AU Remote IO out/1 (result=%d)",
            result);
    }

    
    AURenderCallbackStruct auCbS;
    memset(&auCbS, 0, sizeof(auCbS));
    auCbS.inputProc = RecordProcess;
    auCbS.inputProcRefCon = this;
    result = AudioUnitSetProperty(_auRemoteIO,
        kAudioOutputUnitProperty_SetInputCallback, kAudioUnitScope_Global, 1,
        &auCbS, sizeof(auCbS));
    if (0 != result) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
            "  Could not set record callback for AU Remote IO (result=%d)",
            result);
    }

    
    memset(&auCbS, 0, sizeof(auCbS));
    auCbS.inputProc = PlayoutProcess;
    auCbS.inputProcRefCon = this;
    result = AudioUnitSetProperty(_auRemoteIO,
        kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Global, 0,
        &auCbS, sizeof(auCbS));
    if (0 != result) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
            "  Could not set play callback for AU Remote IO (result=%d)",
            result);
    }

    
    Float64 sampleRate(0.0);
    size = sizeof(sampleRate);
    result = AudioSessionGetProperty(
        kAudioSessionProperty_CurrentHardwareSampleRate, &size, &sampleRate);
    if (0 != result) {
        WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
            "  Could not get current HW sample rate (result=%d)", result);
    }
    WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                 "  Current HW sample rate is %f, ADB sample rate is %d",
             sampleRate, _adbSampFreq);

    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::ShutdownPlayOrRecord() {
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    
    OSStatus result = -1;
    if (NULL != _auRemoteIO) {
        result = AudioOutputUnitStop(_auRemoteIO);
        if (0 != result) {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                "  Error stopping AU Remote IO (result=%d)", result);
        }
        result = AudioUnitUninitialize(_auRemoteIO);
        if (0 != result) {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                "  Error uninitializing AU Remote IO (result=%d)", result);
        }
        result = AudioComponentInstanceDispose(_auRemoteIO);
        if (0 != result) {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                "  Error disposing AU Remote IO (result=%d)", result);
        }
        _auRemoteIO = NULL;
    }

    return 0;
}





OSStatus
    AudioDeviceIPhone::RecordProcess(void *inRefCon,
                                     AudioUnitRenderActionFlags *ioActionFlags,
                                     const AudioTimeStamp *inTimeStamp,
                                     UInt32 inBusNumber,
                                     UInt32 inNumberFrames,
                                     AudioBufferList *ioData) {
    AudioDeviceIPhone* ptrThis = static_cast<AudioDeviceIPhone*>(inRefCon);

    return ptrThis->RecordProcessImpl(ioActionFlags,
                                      inTimeStamp,
                                      inBusNumber,
                                      inNumberFrames);
}


OSStatus
    AudioDeviceIPhone::RecordProcessImpl(
                                    AudioUnitRenderActionFlags *ioActionFlags,
                                    const AudioTimeStamp *inTimeStamp,
                                    WebRtc_UWord32 inBusNumber,
                                    WebRtc_UWord32 inNumberFrames) {
    
    
    
    
    WebRtc_Word16* dataTmp = new WebRtc_Word16[inNumberFrames];
    memset(dataTmp, 0, 2*inNumberFrames);

    AudioBufferList abList;
    abList.mNumberBuffers = 1;
    abList.mBuffers[0].mData = dataTmp;
    abList.mBuffers[0].mDataByteSize = 2*inNumberFrames;  
    abList.mBuffers[0].mNumberChannels = 1;

    
    OSStatus res = AudioUnitRender(_auRemoteIO, ioActionFlags, inTimeStamp,
                                   inBusNumber, inNumberFrames, &abList);
    if (res != 0) {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  Error getting rec data, error = %d", res);

        if (_recWarning > 0) {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "  Pending rec warning exists");
        }
        _recWarning = 1;

        delete [] dataTmp;
        return 0;
    }

    if (_recording) {
        
        
        
        

        const unsigned int noSamp10ms = _adbSampFreq / 100;
        unsigned int dataPos = 0;
        WebRtc_UWord16 bufPos = 0;
        WebRtc_Word16 insertPos = -1;
        unsigned int nCopy = 0;  

        while (dataPos < inNumberFrames) {
            
            
            
            
            bufPos = 0;
            insertPos = -1;
            nCopy = 0;
            while (bufPos < N_REC_BUFFERS) {
                if ((_recordingLength[bufPos] > 0)
                    && (_recordingLength[bufPos] < noSamp10ms)) {
                    
                    insertPos = static_cast<WebRtc_Word16>(bufPos);
                    
                    bufPos = N_REC_BUFFERS;
                } else if ((-1 == insertPos)
                           && (0 == _recordingLength[bufPos])) {
                    
                    insertPos = static_cast<WebRtc_Word16>(bufPos);
                }
                ++bufPos;
            }

            
            if (insertPos > -1) {
                
                unsigned int dataToCopy = inNumberFrames - dataPos;
                unsigned int currentRecLen = _recordingLength[insertPos];
                unsigned int roomInBuffer = noSamp10ms - currentRecLen;
                nCopy = (dataToCopy < roomInBuffer ? dataToCopy : roomInBuffer);

                memcpy(&_recordingBuffer[insertPos][currentRecLen],
                       &dataTmp[dataPos], nCopy*sizeof(WebRtc_Word16));
                if (0 == currentRecLen) {
                    _recordingSeqNumber[insertPos] = _recordingCurrentSeq;
                    ++_recordingCurrentSeq;
                }
                _recordingBufferTotalSize += nCopy;
                
                
                _recordingLength[insertPos] += nCopy;
                dataPos += nCopy;
            } else {
                
                WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                             "  Could not insert into recording buffer");
                if (_recWarning > 0) {
                    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                                 "  Pending rec warning exists");
                }
                _recWarning = 1;
                dataPos = inNumberFrames;  
            }
        }
    }

    delete [] dataTmp;

    return 0;
}

OSStatus
    AudioDeviceIPhone::PlayoutProcess(void *inRefCon,
                                      AudioUnitRenderActionFlags *ioActionFlags,
                                      const AudioTimeStamp *inTimeStamp,
                                      UInt32 inBusNumber,
                                      UInt32 inNumberFrames,
                                      AudioBufferList *ioData) {
    AudioDeviceIPhone* ptrThis = static_cast<AudioDeviceIPhone*>(inRefCon);

    return ptrThis->PlayoutProcessImpl(inNumberFrames, ioData);
}

OSStatus
    AudioDeviceIPhone::PlayoutProcessImpl(WebRtc_UWord32 inNumberFrames,
                                          AudioBufferList *ioData) {
    


    WebRtc_Word16* data =
        static_cast<WebRtc_Word16*>(ioData->mBuffers[0].mData);
    unsigned int dataSizeBytes = ioData->mBuffers[0].mDataByteSize;
    unsigned int dataSize = dataSizeBytes/2;  
        if (dataSize != inNumberFrames) {  
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "dataSize (%u) != inNumberFrames (%u)",
                     dataSize, (unsigned int)inNumberFrames);
        if (_playWarning > 0) {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "  Pending play warning exists");
        }
        _playWarning = 1;
    }
    memset(data, 0, dataSizeBytes);  


    

    if (_playing) {
        unsigned int noSamp10ms = _adbSampFreq / 100;
        
        WebRtc_Word16* dataTmp = new WebRtc_Word16[noSamp10ms];
        memset(dataTmp, 0, 2*noSamp10ms);
        unsigned int dataPos = 0;
        int noSamplesOut = 0;
        unsigned int nCopy = 0;

        
        if (_playoutBufferUsed > 0) {
            nCopy = (dataSize < _playoutBufferUsed) ?
                    dataSize : _playoutBufferUsed;
            if (nCopy != _playoutBufferUsed) {
                
                
                
                WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                             "nCopy (%u) != _playoutBufferUsed (%u)",
                             nCopy, _playoutBufferUsed);
                if (_playWarning > 0) {
                    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                                 "  Pending play warning exists");
                }
                _playWarning = 1;
            }
            memcpy(data, _playoutBuffer, 2*nCopy);
            dataPos = nCopy;
            memset(_playoutBuffer, 0, sizeof(_playoutBuffer));
            _playoutBufferUsed = 0;
        }

        
        while (dataPos < dataSize) {
            
            UpdatePlayoutDelay();

            
            noSamplesOut = _ptrAudioBuffer->RequestPlayoutData(noSamp10ms);

            
            noSamplesOut =
                _ptrAudioBuffer->GetPlayoutData(
                    reinterpret_cast<WebRtc_Word8*>(dataTmp));
            
            if (noSamp10ms != (unsigned int)noSamplesOut) {
                
                WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                             "noSamp10ms (%u) != noSamplesOut (%d)",
                             noSamp10ms, noSamplesOut);

                if (_playWarning > 0) {
                    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                                 "  Pending play warning exists");
                }
                _playWarning = 1;
            }

            
            nCopy = (dataSize-dataPos) > noSamp10ms ?
                    noSamp10ms : (dataSize-dataPos);
            memcpy(&data[dataPos], dataTmp, 2*nCopy);

            
            if (nCopy < noSamp10ms) {
                memcpy(_playoutBuffer, &dataTmp[nCopy], 2*(noSamp10ms-nCopy));
                _playoutBufferUsed = noSamp10ms - nCopy;
            }

            
            
            dataPos += noSamp10ms;
        }

        delete [] dataTmp;
    }

    return 0;
}

void AudioDeviceIPhone::UpdatePlayoutDelay() {
    ++_playoutDelayMeasurementCounter;

    if (_playoutDelayMeasurementCounter >= 100) {
        

        _playoutDelay = 0;

        
        Float32 f32(0);
        UInt32 size = sizeof(f32);
        OSStatus result = AudioSessionGetProperty(
            kAudioSessionProperty_CurrentHardwareOutputLatency, &size, &f32);
        if (0 != result) {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "error HW latency (result=%d)", result);
        }
        _playoutDelay += static_cast<int>(f32 * 1000000);

        
        f32 = 0;
        result = AudioSessionGetProperty(
            kAudioSessionProperty_CurrentHardwareIOBufferDuration, &size, &f32);
        if (0 != result) {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "error HW buffer duration (result=%d)", result);
        }
        _playoutDelay += static_cast<int>(f32 * 1000000);

        
        Float64 f64(0);
        size = sizeof(f64);
        result = AudioUnitGetProperty(_auRemoteIO,
            kAudioUnitProperty_Latency, kAudioUnitScope_Global, 0, &f64, &size);
        if (0 != result) {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "error AU latency (result=%d)", result);
        }
        _playoutDelay += static_cast<int>(f64 * 1000000);

        
        _playoutDelay = (_playoutDelay - 500) / 1000;

        
        _playoutDelayMeasurementCounter = 0;
    }

    
}

void AudioDeviceIPhone::UpdateRecordingDelay() {
    ++_recordingDelayMeasurementCounter;

    if (_recordingDelayMeasurementCounter >= 100) {
        

        _recordingDelayHWAndOS = 0;

        
        Float32 f32(0);
        UInt32 size = sizeof(f32);
        OSStatus result = AudioSessionGetProperty(
            kAudioSessionProperty_CurrentHardwareInputLatency, &size, &f32);
        if (0 != result) {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "error HW latency (result=%d)", result);
        }
        _recordingDelayHWAndOS += static_cast<int>(f32 * 1000000);

        
        f32 = 0;
        result = AudioSessionGetProperty(
            kAudioSessionProperty_CurrentHardwareIOBufferDuration, &size, &f32);
        if (0 != result) {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "error HW buffer duration (result=%d)", result);
        }
        _recordingDelayHWAndOS += static_cast<int>(f32 * 1000000);

        
        Float64 f64(0);
        size = sizeof(f64);
        result = AudioUnitGetProperty(_auRemoteIO, kAudioUnitProperty_Latency,
                                      kAudioUnitScope_Global, 0, &f64, &size);
        if (0 != result) {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "error AU latency (result=%d)", result);
        }
        _recordingDelayHWAndOS += static_cast<int>(f64 * 1000000);

        
        _recordingDelayHWAndOS = (_recordingDelayHWAndOS - 500) / 1000;

        
        _recordingDelayMeasurementCounter = 0;
    }

    _recordingDelay = _recordingDelayHWAndOS;

    
    
    const WebRtc_UWord32 noSamp10ms = _adbSampFreq / 100;
    if (_recordingBufferTotalSize > noSamp10ms) {
        _recordingDelay +=
            (_recordingBufferTotalSize - noSamp10ms) / (_adbSampFreq / 1000);
    }
}

bool AudioDeviceIPhone::RunCapture(void* ptrThis) {
    return static_cast<AudioDeviceIPhone*>(ptrThis)->CaptureWorkerThread();
}

bool AudioDeviceIPhone::CaptureWorkerThread() {
    if (_recording) {
        int bufPos = 0;
        unsigned int lowestSeq = 0;
        int lowestSeqBufPos = 0;
        bool foundBuf = true;
        const unsigned int noSamp10ms = _adbSampFreq / 100;

        while (foundBuf) {
            
            
            
            foundBuf = false;
            for (bufPos = 0; bufPos < N_REC_BUFFERS; ++bufPos) {
                if (noSamp10ms == _recordingLength[bufPos]) {
                    if (!foundBuf) {
                        lowestSeq = _recordingSeqNumber[bufPos];
                        lowestSeqBufPos = bufPos;
                        foundBuf = true;
                    } else if (_recordingSeqNumber[bufPos] < lowestSeq) {
                        lowestSeq = _recordingSeqNumber[bufPos];
                        lowestSeqBufPos = bufPos;
                    }
                }
            }  

            
            if (foundBuf) {
                
                UpdateRecordingDelay();

                
                _ptrAudioBuffer->SetRecordedBuffer(
                    reinterpret_cast<WebRtc_Word8*>(
                        _recordingBuffer[lowestSeqBufPos]),
                        _recordingLength[lowestSeqBufPos]);

                
                
                

                
                _ptrAudioBuffer->SetVQEData(_playoutDelay, _recordingDelay, 0);

                
                
                _ptrAudioBuffer->DeliverRecordedData();

                
                _recordingSeqNumber[lowestSeqBufPos] = 0;
                _recordingBufferTotalSize -= _recordingLength[lowestSeqBufPos];
                
                _recordingLength[lowestSeqBufPos] = 0;
            }
        }  
    }  

    {
        
        
        
        
        timespec t;
        t.tv_sec = 0;
        t.tv_nsec = 5*1000*1000;
        nanosleep(&t, NULL);
    }

    return true;
}

}  


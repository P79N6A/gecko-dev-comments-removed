









#include <AudioToolbox/AudioServices.h>  

#include "webrtc/modules/audio_device/ios/audio_device_ios.h"

#include "webrtc/system_wrappers/interface/thread_wrapper.h"
#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc {
AudioDeviceIPhone::AudioDeviceIPhone(const int32_t id)
    :
    _ptrAudioBuffer(NULL),
    _critSect(*CriticalSectionWrapper::CreateCriticalSection()),
    _captureWorkerThread(NULL),
    _captureWorkerThreadId(0),
    _id(id),
    _auVoiceProcessing(NULL),
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

int32_t AudioDeviceIPhone::ActiveAudioLayer(
    AudioDeviceModule::AudioLayer& audioLayer) const {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);
    audioLayer = AudioDeviceModule::kPlatformDefaultAudio;
    return 0;
}

int32_t AudioDeviceIPhone::Init() {
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
        _captureWorkerThreadId = static_cast<uint32_t>(threadID);
        WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice,
                     _id, "CaptureWorkerThread started (res=%d)", res);
    } else {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice,
                     _id, "Thread already created");
    }
    _playWarning = 0;
    _playError = 0;
    _recWarning = 0;
    _recError = 0;

    _initialized = true;

    return 0;
}

int32_t AudioDeviceIPhone::Terminate() {
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

int32_t AudioDeviceIPhone::SpeakerIsAvailable(bool& available) {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    
    available = true;
    return 0;
}

int32_t AudioDeviceIPhone::InitSpeaker() {
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

int32_t AudioDeviceIPhone::MicrophoneIsAvailable(bool& available) {
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

int32_t AudioDeviceIPhone::InitMicrophone() {
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

int32_t AudioDeviceIPhone::SpeakerVolumeIsAvailable(bool& available) {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    available = false;  

    return 0;
}

int32_t AudioDeviceIPhone::SetSpeakerVolume(uint32_t volume) {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "AudioDeviceIPhone::SetSpeakerVolume(volume=%u)", volume);

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

int32_t AudioDeviceIPhone::SpeakerVolume(uint32_t& volume) const {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

int32_t
    AudioDeviceIPhone::SetWaveOutVolume(uint16_t volumeLeft,
                                        uint16_t volumeRight) {
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

int32_t
AudioDeviceIPhone::WaveOutVolume(uint16_t& ,
                                 uint16_t& ) const {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

int32_t
    AudioDeviceIPhone::MaxSpeakerVolume(uint32_t& maxVolume) const {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

int32_t AudioDeviceIPhone::MinSpeakerVolume(
    uint32_t& minVolume) const {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

int32_t
    AudioDeviceIPhone::SpeakerVolumeStepSize(uint16_t& stepSize) const {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

int32_t AudioDeviceIPhone::SpeakerMuteIsAvailable(bool& available) {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    available = false;  

    return 0;
}

int32_t AudioDeviceIPhone::SetSpeakerMute(bool enable) {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

int32_t AudioDeviceIPhone::SpeakerMute(bool& enabled) const {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

int32_t AudioDeviceIPhone::MicrophoneMuteIsAvailable(bool& available) {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    available = false;  

    return 0;
}

int32_t AudioDeviceIPhone::SetMicrophoneMute(bool enable) {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

int32_t AudioDeviceIPhone::MicrophoneMute(bool& enabled) const {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

int32_t AudioDeviceIPhone::MicrophoneBoostIsAvailable(bool& available) {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    available = false;  

    return 0;
}

int32_t AudioDeviceIPhone::SetMicrophoneBoost(bool enable) {
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

int32_t AudioDeviceIPhone::MicrophoneBoost(bool& enabled) const {
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

int32_t AudioDeviceIPhone::StereoRecordingIsAvailable(bool& available) {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    available = false;  

    return 0;
}

int32_t AudioDeviceIPhone::SetStereoRecording(bool enable) {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "AudioDeviceIPhone::SetStereoRecording(enable=%u)", enable);

    if (enable) {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     " Stereo recording is not supported on this platform");
        return -1;
    }
    return 0;
}

int32_t AudioDeviceIPhone::StereoRecording(bool& enabled) const {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    enabled = false;
    return 0;
}

int32_t AudioDeviceIPhone::StereoPlayoutIsAvailable(bool& available) {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    available = false;  

    return 0;
}

int32_t AudioDeviceIPhone::SetStereoPlayout(bool enable) {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "AudioDeviceIPhone::SetStereoPlayout(enable=%u)", enable);

    if (enable) {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     " Stereo playout is not supported on this platform");
        return -1;
    }
    return 0;
}

int32_t AudioDeviceIPhone::StereoPlayout(bool& enabled) const {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    enabled = false;
    return 0;
}

int32_t AudioDeviceIPhone::SetAGC(bool enable) {
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

int32_t AudioDeviceIPhone::MicrophoneVolumeIsAvailable(bool& available) {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    available = false;  

    return 0;
}

int32_t AudioDeviceIPhone::SetMicrophoneVolume(uint32_t volume) {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "AudioDeviceIPhone::SetMicrophoneVolume(volume=%u)", volume);

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

int32_t
    AudioDeviceIPhone::MicrophoneVolume(uint32_t& volume) const {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

int32_t
    AudioDeviceIPhone::MaxMicrophoneVolume(uint32_t& maxVolume) const {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

int32_t
    AudioDeviceIPhone::MinMicrophoneVolume(uint32_t& minVolume) const {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

int32_t
    AudioDeviceIPhone::MicrophoneVolumeStepSize(
                                            uint16_t& stepSize) const {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

int16_t AudioDeviceIPhone::PlayoutDevices() {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    return (int16_t)1;
}

int32_t AudioDeviceIPhone::SetPlayoutDevice(uint16_t index) {
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

int32_t
    AudioDeviceIPhone::SetPlayoutDevice(AudioDeviceModule::WindowsDeviceType) {
    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "WindowsDeviceType not supported");
    return -1;
}

int32_t
    AudioDeviceIPhone::PlayoutDeviceName(uint16_t index,
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

int32_t
    AudioDeviceIPhone::RecordingDeviceName(uint16_t index,
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

int16_t AudioDeviceIPhone::RecordingDevices() {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    return (int16_t)1;
}

int32_t AudioDeviceIPhone::SetRecordingDevice(uint16_t index) {
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

int32_t
    AudioDeviceIPhone::SetRecordingDevice(
                                        AudioDeviceModule::WindowsDeviceType) {
    WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                 "WindowsDeviceType not supported");
    return -1;
}









int32_t AudioDeviceIPhone::SetLoudspeakerStatus(bool enable) {
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

int32_t AudioDeviceIPhone::GetLoudspeakerStatus(bool &enabled) const {
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

int32_t AudioDeviceIPhone::PlayoutIsAvailable(bool& available) {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    available = false;

    
    int32_t res = InitPlayout();

    
    StopPlayout();

    if (res != -1) {
        available = true;
    }

    return 0;
}

int32_t AudioDeviceIPhone::RecordingIsAvailable(bool& available) {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    available = false;

    
    int32_t res = InitRecording();

    
    StopRecording();

    if (res != -1) {
        available = true;
    }

    return 0;
}

int32_t AudioDeviceIPhone::InitPlayout() {
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

int32_t AudioDeviceIPhone::InitRecording() {
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

int32_t AudioDeviceIPhone::StartRecording() {
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
                     "  Starting Audio Unit");
        OSStatus result = AudioOutputUnitStart(_auVoiceProcessing);
        if (0 != result) {
            WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                         "  Error starting Audio Unit (result=%d)", result);
            return -1;
        }
    }

    _recording = true;

    return 0;
}

int32_t AudioDeviceIPhone::StopRecording() {
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

int32_t AudioDeviceIPhone::StartPlayout() {
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
                     "  Starting Audio Unit");
        OSStatus result = AudioOutputUnitStart(_auVoiceProcessing);
        if (0 != result) {
            WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                         "  Error starting Audio Unit (result=%d)", result);
            return -1;
        }
    }

    _playing = true;

    return 0;
}

int32_t AudioDeviceIPhone::StopPlayout() {
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








int32_t AudioDeviceIPhone::ResetAudioDevice() {
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

int32_t AudioDeviceIPhone::PlayoutDelay(uint16_t& delayMS) const {
    delayMS = _playoutDelay;
    return 0;
}

int32_t AudioDeviceIPhone::RecordingDelay(uint16_t& delayMS) const {
    delayMS = _recordingDelay;
    return 0;
}

int32_t
    AudioDeviceIPhone::SetPlayoutBuffer(
                                    const AudioDeviceModule::BufferType type,
                                    uint16_t sizeMS) {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "AudioDeviceIPhone::SetPlayoutBuffer(type=%u, sizeMS=%u)",
                 type, sizeMS);

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

int32_t
    AudioDeviceIPhone::PlayoutBuffer(AudioDeviceModule::BufferType& type,
                                     uint16_t& sizeMS) const {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    type = AudioDeviceModule::kAdaptiveBufferSize;

    sizeMS = _playoutDelay;

    return 0;
}

int32_t AudioDeviceIPhone::CPULoad(uint16_t& ) const {
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





int32_t AudioDeviceIPhone::InitPlayOrRecord() {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    OSStatus result = -1;

    
    if (NULL != _auVoiceProcessing) {
        
        
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     "  Already initialized");
        
        return 0;
    }

    
    AudioComponentDescription desc;
    AudioComponent comp;

    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_VoiceProcessingIO;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;

    comp = AudioComponentFindNext(NULL, &desc);
    if (NULL == comp) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Could not find audio component for Audio Unit");
        return -1;
    }

    result = AudioComponentInstanceNew(comp, &_auVoiceProcessing);
    if (0 != result) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Could not create Audio Unit instance (result=%d)",
                     result);
        return -1;
    }

    
    Float64 sampleRate(16000.0);
    result = AudioSessionSetProperty(
                         kAudioSessionProperty_PreferredHardwareSampleRate,
                         sizeof(sampleRate), &sampleRate);
    if (0 != result) {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     "Could not set preferred sample rate (result=%d)", result);
    }

    uint32_t voiceChat = kAudioSessionMode_VoiceChat;
    AudioSessionSetProperty(kAudioSessionProperty_Mode,
                            sizeof(voiceChat), &voiceChat);

    
    

    
    
    

    

    
    
    

    UInt32 enableIO = 1;
    result = AudioUnitSetProperty(_auVoiceProcessing,
                                  kAudioOutputUnitProperty_EnableIO,
                                  kAudioUnitScope_Input,
                                  1,  
                                  &enableIO,
                                  sizeof(enableIO));
    if (0 != result) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Could not enable IO on input (result=%d)", result);
    }

    result = AudioUnitSetProperty(_auVoiceProcessing,
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
        _auVoiceProcessing, kAudioUnitProperty_ShouldAllocateBuffer,
        kAudioUnitScope_Output,  1, &flag, sizeof(flag));
    if (0 != result) {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  Could not disable AU buffer allocation (result=%d)",
                     result);
        
    }

    
    AURenderCallbackStruct auCbS;
    memset(&auCbS, 0, sizeof(auCbS));
    auCbS.inputProc = RecordProcess;
    auCbS.inputProcRefCon = this;
    result = AudioUnitSetProperty(_auVoiceProcessing,
                                  kAudioOutputUnitProperty_SetInputCallback,
                                  kAudioUnitScope_Global, 1,
                                  &auCbS, sizeof(auCbS));
    if (0 != result) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
            "  Could not set record callback for Audio Unit (result=%d)",
            result);
    }

    
    memset(&auCbS, 0, sizeof(auCbS));
    auCbS.inputProc = PlayoutProcess;
    auCbS.inputProcRefCon = this;
    result = AudioUnitSetProperty(_auVoiceProcessing,
                                  kAudioUnitProperty_SetRenderCallback,
                                  kAudioUnitScope_Global, 0,
                                  &auCbS, sizeof(auCbS));
    if (0 != result) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
            "  Could not set play callback for Audio Unit (result=%d)",
            result);
    }

    
    AudioStreamBasicDescription playoutDesc;
    UInt32 size = sizeof(playoutDesc);
    result = AudioUnitGetProperty(_auVoiceProcessing,
                                  kAudioUnitProperty_StreamFormat,
                                  kAudioUnitScope_Output, 0, &playoutDesc,
                                  &size);
    if (0 != result) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
            "  Could not get stream format Audio Unit out/0 (result=%d)",
            result);
    }
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "  Audio Unit playout opened in sampling rate %f",
                 playoutDesc.mSampleRate);

    playoutDesc.mSampleRate = sampleRate;

    
    
    if ((playoutDesc.mSampleRate > 44090.0)
        && (playoutDesc.mSampleRate < 44110.0)) {
        _adbSampFreq = 44100;
    } else if ((playoutDesc.mSampleRate > 15990.0)
               && (playoutDesc.mSampleRate < 16010.0)) {
        _adbSampFreq = 16000;
    } else if ((playoutDesc.mSampleRate > 7990.0)
               && (playoutDesc.mSampleRate < 8010.0)) {
        _adbSampFreq = 8000;
    } else {
        _adbSampFreq = 0;
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
            "  Audio Unit out/0 opened in unknown sampling rate (%f)",
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
    result = AudioUnitSetProperty(_auVoiceProcessing,
                                  kAudioUnitProperty_StreamFormat,
                                  kAudioUnitScope_Input, 0, &playoutDesc, size);
    if (0 != result) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
            "  Could not set stream format Audio Unit in/0 (result=%d)",
            result);
    }

    
    AudioStreamBasicDescription recordingDesc;
    size = sizeof(recordingDesc);
    result = AudioUnitGetProperty(_auVoiceProcessing,
                                  kAudioUnitProperty_StreamFormat,
                                  kAudioUnitScope_Input, 1, &recordingDesc,
                                  &size);
    if (0 != result) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
            "  Could not get stream format Audio Unit in/1 (result=%d)",
            result);
    }
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "  Audio Unit recording opened in sampling rate %f",
                 recordingDesc.mSampleRate);

    recordingDesc.mSampleRate = sampleRate;

    
    recordingDesc.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger
                                 | kLinearPCMFormatFlagIsPacked
                                 | kLinearPCMFormatFlagIsNonInterleaved;

    recordingDesc.mBytesPerPacket = 2;
    recordingDesc.mFramesPerPacket = 1;
    recordingDesc.mBytesPerFrame = 2;
    recordingDesc.mChannelsPerFrame = 1;
    recordingDesc.mBitsPerChannel = 16;
    result = AudioUnitSetProperty(_auVoiceProcessing,
                                  kAudioUnitProperty_StreamFormat,
                                  kAudioUnitScope_Output, 1, &recordingDesc,
                                  size);
    if (0 != result) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
            "  Could not set stream format Audio Unit out/1 (result=%d)",
            result);
    }

    
    result = AudioUnitInitialize(_auVoiceProcessing);
    if (0 != result) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Could not init Audio Unit (result=%d)", result);
    }

    
    Float64 hardwareSampleRate = 0.0;
    size = sizeof(hardwareSampleRate);
    result = AudioSessionGetProperty(
        kAudioSessionProperty_CurrentHardwareSampleRate, &size,
        &hardwareSampleRate);
    if (0 != result) {
        WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
            "  Could not get current HW sample rate (result=%d)", result);
    }
    WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                 "  Current HW sample rate is %f, ADB sample rate is %d",
             hardwareSampleRate, _adbSampFreq);

    return 0;
}

int32_t AudioDeviceIPhone::ShutdownPlayOrRecord() {
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    
    OSStatus result = -1;
    if (NULL != _auVoiceProcessing) {
        result = AudioOutputUnitStop(_auVoiceProcessing);
        if (0 != result) {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                "  Error stopping Audio Unit (result=%d)", result);
        }
        result = AudioComponentInstanceDispose(_auVoiceProcessing);
        if (0 != result) {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                "  Error disposing Audio Unit (result=%d)", result);
        }
        _auVoiceProcessing = NULL;
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
                                    uint32_t inBusNumber,
                                    uint32_t inNumberFrames) {
    
    
    
    
    int16_t* dataTmp = new int16_t[inNumberFrames];
    memset(dataTmp, 0, 2*inNumberFrames);

    AudioBufferList abList;
    abList.mNumberBuffers = 1;
    abList.mBuffers[0].mData = dataTmp;
    abList.mBuffers[0].mDataByteSize = 2*inNumberFrames;  
    abList.mBuffers[0].mNumberChannels = 1;

    
    OSStatus res = AudioUnitRender(_auVoiceProcessing,
                                   ioActionFlags, inTimeStamp,
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
        uint16_t bufPos = 0;
        int16_t insertPos = -1;
        unsigned int nCopy = 0;  

        while (dataPos < inNumberFrames) {
            
            
            
            
            bufPos = 0;
            insertPos = -1;
            nCopy = 0;
            while (bufPos < N_REC_BUFFERS) {
                if ((_recordingLength[bufPos] > 0)
                    && (_recordingLength[bufPos] < noSamp10ms)) {
                    
                    insertPos = static_cast<int16_t>(bufPos);
                    
                    bufPos = N_REC_BUFFERS;
                } else if ((-1 == insertPos)
                           && (0 == _recordingLength[bufPos])) {
                    
                    insertPos = static_cast<int16_t>(bufPos);
                }
                ++bufPos;
            }

            
            if (insertPos > -1) {
                
                unsigned int dataToCopy = inNumberFrames - dataPos;
                unsigned int currentRecLen = _recordingLength[insertPos];
                unsigned int roomInBuffer = noSamp10ms - currentRecLen;
                nCopy = (dataToCopy < roomInBuffer ? dataToCopy : roomInBuffer);

                memcpy(&_recordingBuffer[insertPos][currentRecLen],
                       &dataTmp[dataPos], nCopy*sizeof(int16_t));
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
    AudioDeviceIPhone::PlayoutProcessImpl(uint32_t inNumberFrames,
                                          AudioBufferList *ioData) {
    


    int16_t* data =
        static_cast<int16_t*>(ioData->mBuffers[0].mData);
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
        
        int16_t* dataTmp = new int16_t[noSamp10ms];
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
                    reinterpret_cast<int8_t*>(dataTmp));
            
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
        result = AudioUnitGetProperty(_auVoiceProcessing,
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
        result = AudioUnitGetProperty(_auVoiceProcessing,
                                      kAudioUnitProperty_Latency,
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

    
    
    const uint32_t noSamp10ms = _adbSampFreq / 100;
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
                    reinterpret_cast<int8_t*>(
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

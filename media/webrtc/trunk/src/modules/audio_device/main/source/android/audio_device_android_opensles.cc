









#include <time.h>
#include <sys/time.h>

#include "audio_device_utility.h"
#include "audio_device_android_opensles.h"
#include "audio_device_config.h"

#include "trace.h"
#include "thread_wrapper.h"
#include "event_wrapper.h"

#ifdef WEBRTC_ANDROID_DEBUG
#include <android/log.h>
#define WEBRTC_TRACE(a,b,c,...)  __android_log_print(                  \
           ANDROID_LOG_DEBUG, "WebRTC ADM OpenSLES", __VA_ARGS__)
#endif

namespace webrtc {









AudioDeviceAndroidOpenSLES::AudioDeviceAndroidOpenSLES(const WebRtc_Word32 id) :
    _ptrAudioBuffer(NULL),
    _critSect(*CriticalSectionWrapper::CreateCriticalSection()),
    _id(id),
    _slEngineObject(NULL),
    _slPlayer(NULL),
    _slEngine(NULL),
    _slPlayerPlay(NULL),
    _slOutputMixObject(NULL),
    _slSpeakerVolume(NULL),
    _slRecorder(NULL),
    _slRecorderRecord(NULL),
    _slAudioIODeviceCapabilities(NULL),
    _slRecorderSimpleBufferQueue(NULL),
    _slMicVolume(NULL),
    _micDeviceId(0),
    _recQueueSeq(0),
    _timeEventRec(*EventWrapper::Create()),
    _ptrThreadRec(NULL),
    _recThreadID(0),
    _playQueueSeq(0),
    _recCurrentSeq(0),
    _recBufferTotalSize(0),
    _recordingDeviceIsSpecified(false),
    _playoutDeviceIsSpecified(false),
    _initialized(false),
    _recording(false),
    _playing(false),
    _recIsInitialized(false),
    _playIsInitialized(false),
    _micIsInitialized(false),
    _speakerIsInitialized(false),
    _playWarning(0),
    _playError(0),
    _recWarning(0),
    _recError(0),
    _playoutDelay(0),
    _recordingDelay(0),
    _AGC(false),
    _adbSampleRate(0),
    _samplingRateIn(SL_SAMPLINGRATE_16),
    _samplingRateOut(SL_SAMPLINGRATE_16),
    _maxSpeakerVolume(0),
    _minSpeakerVolume(0),
    _loudSpeakerOn(false) {
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, id, "%s created",
                 __FUNCTION__);
    memset(_playQueueBuffer, 0, sizeof(_playQueueBuffer));
    memset(_recQueueBuffer, 0, sizeof(_recQueueBuffer));
    memset(_recBuffer, 0, sizeof(_recBuffer));
    memset(_recLength, 0, sizeof(_recLength));
    memset(_recSeqNumber, 0, sizeof(_recSeqNumber));
}

AudioDeviceAndroidOpenSLES::~AudioDeviceAndroidOpenSLES() {
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id, "%s destroyed",
                 __FUNCTION__);

    Terminate();

    delete &_timeEventRec;
    delete &_critSect;
}





void AudioDeviceAndroidOpenSLES::AttachAudioBuffer(
    AudioDeviceBuffer* audioBuffer) {

    CriticalSectionScoped lock(&_critSect);

    _ptrAudioBuffer = audioBuffer;

    
    _ptrAudioBuffer->SetRecordingSampleRate(N_REC_SAMPLES_PER_SEC);
    _ptrAudioBuffer->SetPlayoutSampleRate(N_PLAY_SAMPLES_PER_SEC);
    _ptrAudioBuffer->SetRecordingChannels(N_REC_CHANNELS);
    _ptrAudioBuffer->SetPlayoutChannels(N_PLAY_CHANNELS);
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::ActiveAudioLayer(
    AudioDeviceModule::AudioLayer& audioLayer) const {

    audioLayer = AudioDeviceModule::kPlatformDefaultAudio;

    return 0;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::Init() {

    CriticalSectionScoped lock(&_critSect);

    if (_initialized) {
        return 0;
    }

    _playWarning = 0;
    _playError = 0;
    _recWarning = 0;
    _recError = 0;

    SLEngineOption EngineOption[] = {
      { (SLuint32) SL_ENGINEOPTION_THREADSAFE, (SLuint32) SL_BOOLEAN_TRUE },
    };
    WebRtc_Word32 res = slCreateEngine(&_slEngineObject, 1, EngineOption, 0,
                                       NULL, NULL);
    
    
    if (res != SL_RESULT_SUCCESS) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  failed to create SL Engine Object");
        return -1;
    }
    
    if ((*_slEngineObject)->Realize(_slEngineObject, SL_BOOLEAN_FALSE)
            != SL_RESULT_SUCCESS) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  failed to Realize SL Engine");
        return -1;
    }

    if ((*_slEngineObject)->GetInterface(_slEngineObject, SL_IID_ENGINE,
                                         (void*) &_slEngine)
            != SL_RESULT_SUCCESS) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  failed to get SL Engine interface");
        return -1;
    }

    
    if (InitSampleRate() != 0) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "%s: Failed to init samplerate", __FUNCTION__);
        return -1;
    }

    
    
    if (_ptrAudioBuffer->SetRecordingSampleRate(_adbSampleRate) < 0) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Could not set audio device buffer recording "
                         "sampling rate (%d)", _adbSampleRate);
    }
    if (_ptrAudioBuffer->SetPlayoutSampleRate(_adbSampleRate) < 0) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Could not set audio device buffer playout sampling "
                         "rate (%d)", _adbSampleRate);
    }

    _initialized = true;

    return 0;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::Terminate() {

    CriticalSectionScoped lock(&_critSect);

    if (!_initialized) {
        return 0;
    }

    
    StopRecording();

    _micIsInitialized = false;
    _recordingDeviceIsSpecified = false;

    
    StopPlayout();

    if (_slEngineObject != NULL) {
        (*_slEngineObject)->Destroy(_slEngineObject);
        _slEngineObject = NULL;
        _slEngine = NULL;
    }

    _initialized = false;

    return 0;
}

bool AudioDeviceAndroidOpenSLES::Initialized() const {

    return (_initialized);
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::SpeakerIsAvailable(bool& available) {

    
    available = true;

    return 0;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::InitSpeaker() {

    CriticalSectionScoped lock(&_critSect);

    if (_playing) {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  Playout already started");
        return -1;
    }

    if (!_playoutDeviceIsSpecified) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Playout device is not specified");
        return -1;
    }

    
    
    _speakerIsInitialized = true;

    return 0;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::MicrophoneIsAvailable(
    bool& available) {

    
    available = true;

    return 0;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::InitMicrophone() {

    CriticalSectionScoped lock(&_critSect);

    if (_recording) {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  Recording already started");
        return -1;
    }

    if (!_recordingDeviceIsSpecified) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Recording device is not specified");
        return -1;
    }

    
    
    _micIsInitialized = true;

    return 0;
}

bool AudioDeviceAndroidOpenSLES::SpeakerIsInitialized() const {

    return _speakerIsInitialized;
}

bool AudioDeviceAndroidOpenSLES::MicrophoneIsInitialized() const {

    return _micIsInitialized;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::SpeakerVolumeIsAvailable(
    bool& available) {

    available = true; 

    return 0;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::SetSpeakerVolume(
    WebRtc_UWord32 volume) {

    if (!_speakerIsInitialized) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Speaker not initialized");
        return -1;
    }

    if (_slEngineObject == NULL) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "SetSpeakerVolume, SL Engine object doesnt exist");
        return -1;
    }

    if (_slEngine == NULL) {
        
        if ((*_slEngineObject)->GetInterface(_slEngineObject, SL_IID_ENGINE,
                                             (void*) &_slEngine)
                != SL_RESULT_SUCCESS) {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  failed to GetInterface SL Engine Interface");
            return -1;
        }
    }
    return 0;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::SpeakerVolume(
    WebRtc_UWord32& volume) const {
    return 0;
}





WebRtc_Word32 AudioDeviceAndroidOpenSLES::SetWaveOutVolume(
    WebRtc_UWord16 ,
    WebRtc_UWord16 ) {

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}





WebRtc_Word32 AudioDeviceAndroidOpenSLES::WaveOutVolume(
    WebRtc_UWord16& ,
    WebRtc_UWord16& ) const {

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::MaxSpeakerVolume(
    WebRtc_UWord32& maxVolume) const {

    if (!_speakerIsInitialized) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Speaker not initialized");
        return -1;
    }

    maxVolume = _maxSpeakerVolume;

    return 0;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::MinSpeakerVolume(
    WebRtc_UWord32& minVolume) const {

    if (!_speakerIsInitialized) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Speaker not initialized");
        return -1;
    }

    minVolume = _minSpeakerVolume;

    return 0;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::SpeakerVolumeStepSize(
    WebRtc_UWord16& stepSize) const {

    if (!_speakerIsInitialized) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Speaker not initialized");
        return -1;
    }
    stepSize = 1;

    return 0;
}





WebRtc_Word32 AudioDeviceAndroidOpenSLES::SpeakerMuteIsAvailable(
    bool& available) {

    available = false; 

    return 0;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::SetSpeakerMute(bool ) {

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::SpeakerMute(bool& ) const {

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::MicrophoneMuteIsAvailable(
    bool& available) {

    available = false; 

    return 0;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::SetMicrophoneMute(bool ) {

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::MicrophoneMute(
    bool& ) const {

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::MicrophoneBoostIsAvailable(
    bool& available) {

    available = false; 

    return 0;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::SetMicrophoneBoost(bool enable) {

    if (!_micIsInitialized) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Microphone not initialized");
        return -1;
    }

    if (enable) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Enabling not available");
        return -1;
    }

    return 0;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::MicrophoneBoost(bool& enabled) const {

    if (!_micIsInitialized) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Microphone not initialized");
        return -1;
    }

    enabled = false;

    return 0;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::StereoRecordingIsAvailable(
    bool& available) {

    available = false; 

    return 0;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::SetStereoRecording(bool enable) {

    if (enable) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Enabling not available");
        return -1;
    }

    return 0;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::StereoRecording(bool& enabled) const {

    enabled = false;

    return 0;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::StereoPlayoutIsAvailable(
    bool& available) {

    available = false; 

    return 0;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::SetStereoPlayout(bool enable) {

    if (enable) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Enabling not available");
        return -1;
    }

    return 0;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::StereoPlayout(bool& enabled) const {

    enabled = false;

    return 0;
}


WebRtc_Word32 AudioDeviceAndroidOpenSLES::SetAGC(bool enable) {

    _AGC = enable;

    return 0;
}

bool AudioDeviceAndroidOpenSLES::AGC() const {

    return _AGC;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::MicrophoneVolumeIsAvailable(
    bool& available) {

    available = true;

    return 0;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::SetMicrophoneVolume(
    WebRtc_UWord32 volume) {

    if (_slEngineObject == NULL) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "SetMicrophoneVolume, SL Engine Object doesnt exist");
        return -1;
    }

    
    if (_slMicVolume == NULL) {
        
        if ((*_slEngineObject)->GetInterface(_slEngineObject,
                                             SL_IID_DEVICEVOLUME,
                                             (void*) &_slMicVolume)
                != SL_RESULT_SUCCESS) {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  failed to create Output Mix object");
        }
    }

    if (_slMicVolume != NULL) {
        WebRtc_Word32 vol(0);
        vol = ((volume * (_maxSpeakerVolume - _minSpeakerVolume) +
                (int) (255 / 2)) / (255)) + _minSpeakerVolume;
        if ((*_slMicVolume)->SetVolume(_slMicVolume, _micDeviceId, vol)
                != SL_RESULT_SUCCESS) {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  failed to create Output Mix object");
        }
    }

    return 0;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::MicrophoneVolume(
    WebRtc_UWord32& ) const {
    return -1;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::MaxMicrophoneVolume(
    WebRtc_UWord32& ) const {
    return 0;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::MinMicrophoneVolume(
    WebRtc_UWord32& minVolume) const {

    minVolume = 0;
    return 0;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::MicrophoneVolumeStepSize(
    WebRtc_UWord16& stepSize) const {

    stepSize = 1;
    return 0;
}

WebRtc_Word16 AudioDeviceAndroidOpenSLES::PlayoutDevices() {

    return 1;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::SetPlayoutDevice(
    WebRtc_UWord16 index) {

    if (_playIsInitialized) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Playout already initialized");
        return -1;
    }

    if (0 != index) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Device index is out of range [0,0]");
        return -1;
    }

    
    
    _playoutDeviceIsSpecified = true;

    return 0;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::SetPlayoutDevice(
    AudioDeviceModule::WindowsDeviceType ) {

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::PlayoutDeviceName(
    WebRtc_UWord16 index,
    char name[kAdmMaxDeviceNameSize],
    char guid[kAdmMaxGuidSize]) {

    if (0 != index) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Device index is out of range [0,0]");
        return -1;
    }

    
    memset(name, 0, kAdmMaxDeviceNameSize);

    if (guid) {
        memset(guid, 0, kAdmMaxGuidSize);
    }

    return 0;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::RecordingDeviceName(
    WebRtc_UWord16 index,
    char name[kAdmMaxDeviceNameSize],
    char guid[kAdmMaxGuidSize]) {

    if (0 != index) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Device index is out of range [0,0]");
        return -1;
    }

    
    memset(name, 0, kAdmMaxDeviceNameSize);

    if (guid) {
        memset(guid, 0, kAdmMaxGuidSize);
    }

    return 0;
}

WebRtc_Word16 AudioDeviceAndroidOpenSLES::RecordingDevices() {

    return 1;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::SetRecordingDevice(
    WebRtc_UWord16 index) {

    if (_recIsInitialized) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Recording already initialized");
        return -1;
    }

    if (0 != index) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Device index is out of range [0,0]");
        return -1;
    }

    
    
    _recordingDeviceIsSpecified = true;

    return 0;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::SetRecordingDevice(
    AudioDeviceModule::WindowsDeviceType ) {

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::PlayoutIsAvailable(bool& available) {

    available = false;

    
    WebRtc_Word32 res = InitPlayout();

    
    StopPlayout();

    if (res != -1) {
        available = true;
    }

    return res;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::RecordingIsAvailable(
    bool& available) {

    available = false;

    
    WebRtc_Word32 res = InitRecording();

    
    StopRecording();

    if (res != -1) {
        available = true;
    }

    return res;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::InitPlayout() {

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

    if (!_playoutDeviceIsSpecified) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Playout device is not specified");
        return -1;
    }

    if (_playIsInitialized) {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     "  Playout already initialized");
        return 0;
    }

    
    if (InitSpeaker() == -1) {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  InitSpeaker() failed");
    }

    if (_slEngineObject == NULL || _slEngine == NULL) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  SLObject or Engiine is NULL");
        return -1;
    }

    WebRtc_Word32 res = -1;
    SLDataFormat_PCM pcm;
    SLDataSource audioSource;
    SLDataLocator_AndroidSimpleBufferQueue simpleBufferQueue;
    SLDataSink audioSink;
    SLDataLocator_OutputMix locator_outputmix;

    
    SLInterfaceID ids[N_MAX_INTERFACES];
    SLboolean req[N_MAX_INTERFACES];
    for (unsigned int i = 0; i < N_MAX_INTERFACES; i++) {
        ids[i] = SL_IID_NULL;
        req[i] = SL_BOOLEAN_FALSE;
    }
    ids[0] = SL_IID_ENVIRONMENTALREVERB;
    res = (*_slEngine)->CreateOutputMix(_slEngine, &_slOutputMixObject, 1, ids,
                                        req);
    if (res != SL_RESULT_SUCCESS) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  failed to get SL Output Mix object");
        return -1;
    }
    
    res = (*_slOutputMixObject)->Realize(_slOutputMixObject, SL_BOOLEAN_FALSE);
    if (res != SL_RESULT_SUCCESS) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  failed to realize SL Output Mix object");
        return -1;
    }

    
    
    simpleBufferQueue.locatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
    
    simpleBufferQueue.numBuffers = N_PLAY_QUEUE_BUFFERS;
    
    
    pcm.formatType = SL_DATAFORMAT_PCM;
    pcm.numChannels = 1;
    
    pcm.samplesPerSec = SL_SAMPLINGRATE_16;
    pcm.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
    pcm.containerSize = SL_PCMSAMPLEFORMAT_FIXED_16;
    pcm.channelMask = SL_SPEAKER_FRONT_CENTER;
    pcm.endianness = SL_BYTEORDER_LITTLEENDIAN;
    audioSource.pFormat = (void *) &pcm;
    audioSource.pLocator = (void *) &simpleBufferQueue;
    
    locator_outputmix.locatorType = SL_DATALOCATOR_OUTPUTMIX;
    locator_outputmix.outputMix = _slOutputMixObject;
    audioSink.pLocator = (void *) &locator_outputmix;
    audioSink.pFormat = NULL;

    
    
    ids[0] = SL_IID_BUFFERQUEUE;
    ids[1] = SL_IID_EFFECTSEND;
    req[0] = SL_BOOLEAN_TRUE;
    req[1] = SL_BOOLEAN_TRUE;
    
    res = (*_slEngine)->CreateAudioPlayer(_slEngine, &_slPlayer, &audioSource,
                                          &audioSink, 2, ids, req);
    if (res != SL_RESULT_SUCCESS) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  failed to create Audio Player");
        return -1;
    }

    
    res = (*_slPlayer)->Realize(_slPlayer, SL_BOOLEAN_FALSE);
    if (res != SL_RESULT_SUCCESS) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  failed to realize the player");
        return -1;
    }
    
    res = (*_slPlayer)->GetInterface(_slPlayer, SL_IID_PLAY,
                                     (void*) &_slPlayerPlay);
    if (res != SL_RESULT_SUCCESS) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  failed to get Player interface");
        return -1;
    }
    res = (*_slPlayer)->GetInterface(_slPlayer, SL_IID_BUFFERQUEUE,
                                     (void*) &_slPlayerSimpleBufferQueue);
    if (res != SL_RESULT_SUCCESS) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  failed to get Player Simple Buffer Queue interface");
        return -1;
    }

    
    res = (*_slPlayerSimpleBufferQueue)->RegisterCallback(
        _slPlayerSimpleBufferQueue,
        PlayerSimpleBufferQueueCallback,
        this);
    if (res != SL_RESULT_SUCCESS) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  failed to register Player Callback");
        return -1;
    }
    _playIsInitialized = true;
    return 0;
}





WebRtc_Word32 AudioDeviceAndroidOpenSLES::InitRecording() {

    CriticalSectionScoped lock(&_critSect);

    if (!_initialized) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "  Not initialized");
        return -1;
    }

    if (_recording) {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  Recording already started");
        return -1;
    }

    if (!_recordingDeviceIsSpecified) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Recording device is not specified");
        return -1;
    }

    if (_recIsInitialized) {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     "  Recording already initialized");
        return 0;
    }

    
    if (InitMicrophone() == -1) {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  InitMicrophone() failed");
    }

    if (_slEngineObject == NULL || _slEngine == NULL) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Recording object is NULL");
        return -1;
    }

    WebRtc_Word32 res(-1);
    SLDataSource audioSource;
    SLDataLocator_IODevice micLocator;
    SLDataSink audioSink;
    SLDataFormat_PCM pcm;
    SLDataLocator_AndroidSimpleBufferQueue simpleBufferQueue;

    
    micLocator.locatorType = SL_DATALOCATOR_IODEVICE;
    micLocator.deviceType = SL_IODEVICE_AUDIOINPUT;
    micLocator.deviceID = SL_DEFAULTDEVICEID_AUDIOINPUT; 
    micLocator.device = NULL;
    audioSource.pLocator = (void *) &micLocator;
    audioSource.pFormat = NULL;

    
    simpleBufferQueue.locatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
    simpleBufferQueue.numBuffers = N_REC_QUEUE_BUFFERS;
    
    pcm.formatType = SL_DATAFORMAT_PCM;
    pcm.numChannels = 1;
    
    pcm.samplesPerSec = SL_SAMPLINGRATE_16;
    pcm.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
    pcm.containerSize = 16;
    pcm.channelMask = SL_SPEAKER_FRONT_CENTER;
    pcm.endianness = SL_BYTEORDER_LITTLEENDIAN;
    audioSink.pFormat = (void *) &pcm;
    audioSink.pLocator = (void *) &simpleBufferQueue;

    const SLInterfaceID id[1] = { SL_IID_ANDROIDSIMPLEBUFFERQUEUE };
    const SLboolean req[1] = { SL_BOOLEAN_TRUE };
    res = (*_slEngine)->CreateAudioRecorder(_slEngine, &_slRecorder,
                                            &audioSource, &audioSink, 1, id,
                                            req);
    if (res != SL_RESULT_SUCCESS) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  failed to create Recorder");
        return -1;
    }

    
    res = (*_slRecorder)->Realize(_slRecorder, SL_BOOLEAN_FALSE);
    if (res != SL_RESULT_SUCCESS) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  failed to realize Recorder");
        return -1;
    }

    
    res = (*_slRecorder)->GetInterface(_slRecorder, SL_IID_RECORD,
                                       (void*) &_slRecorderRecord);
    if (res != SL_RESULT_SUCCESS) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  failed to get Recorder interface");
        return -1;
    }

    
    res = (*_slRecorder)->GetInterface(_slRecorder,
                                       SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
                                       (void*) &_slRecorderSimpleBufferQueue);
    if (res != SL_RESULT_SUCCESS) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  failed to get Recorder Simple Buffer Queue");
        return -1;
    }

    
    res = (*_slRecorderSimpleBufferQueue)->RegisterCallback(
        _slRecorderSimpleBufferQueue,
        RecorderSimpleBufferQueueCallback,
        this);
    if (res != SL_RESULT_SUCCESS) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  failed to register Recorder Callback");
        return -1;
    }

    _recIsInitialized = true;
    return 0;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::StartRecording() {

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

    if (_slRecorderRecord == NULL) {
      WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "  RecordITF is NULL");
        return -1;
    }

    if (_slRecorderSimpleBufferQueue == NULL) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Recorder Simple Buffer Queue is NULL");
        return -1;
    }

    
    memset(_recQueueBuffer, 0, sizeof(_recQueueBuffer)); 
    _recQueueSeq = 0;

    const char* threadName = "webrtc_opensles_audio_capture_thread";
    _ptrThreadRec = ThreadWrapper::CreateThread(RecThreadFunc, this,
            kRealtimePriority, threadName);
    if (_ptrThreadRec == NULL)
    {
        WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                "  failed to create the rec audio thread");
        return -1;
    }

    unsigned int threadID(0);
    if (!_ptrThreadRec->Start(threadID))
    {
        WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                "  failed to start the rec audio thread");
        delete _ptrThreadRec;
        _ptrThreadRec = NULL;
        return -1;
    }
    _recThreadID = threadID;
    _recThreadIsInitialized = true;
    memset(_recBuffer, 0, sizeof(_recBuffer));
    memset(_recLength, 0, sizeof(_recLength));
    memset(_recSeqNumber, 0, sizeof(_recSeqNumber));
    _recCurrentSeq = 0;
    _recBufferTotalSize = 0;
    _recWarning = 0;
    _recError = 0;

    
    
    WebRtc_Word32 res(-1);
    WebRtc_UWord32 nSample10ms = _adbSampleRate / 100;
    for (int i = 0; i < (N_REC_QUEUE_BUFFERS - 1); i++) {
        
        res = (*_slRecorderSimpleBufferQueue)->Enqueue(
            _slRecorderSimpleBufferQueue,
            (void*) _recQueueBuffer[_recQueueSeq],
            2 * nSample10ms);
        if (res != SL_RESULT_SUCCESS) {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  failed to Enqueue Empty Buffer to recorder");
            return -1;
        }
        _recQueueSeq++;
    }
    
    res = (*_slRecorderRecord)->SetRecordState(_slRecorderRecord,
                                               SL_RECORDSTATE_RECORDING);
    if (res != SL_RESULT_SUCCESS) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  failed to start recording");
        return -1;
    }
    _recording = true;

    return 0;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::StopRecording() {

    CriticalSectionScoped lock(&_critSect);

    if (!_recIsInitialized) {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     "  Recording is not initialized");
        return 0;
    }

    
    if (_ptrThreadRec != NULL)
    {
        WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                "Stopping capture thread");
        bool res = _ptrThreadRec->Stop();
        if (!res) {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                                    "Failed to stop Capture thread ");
        } else {
            delete _ptrThreadRec;
            _ptrThreadRec = NULL;
            _recThreadIsInitialized = false;
        }
    }

    if ((_slRecorderRecord != NULL) && (_slRecorder != NULL)) {
        
        WebRtc_Word32 res = (*_slRecorderRecord)->SetRecordState(
            _slRecorderRecord,
            SL_RECORDSTATE_STOPPED);
        if (res != SL_RESULT_SUCCESS) {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  failed to stop recording");
            return -1;
        }
        res = (*_slRecorderSimpleBufferQueue)->Clear(
              _slRecorderSimpleBufferQueue);
        if (res != SL_RESULT_SUCCESS) {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  failed to clear recorder buffer queue");
            return -1;
        }

        
        (*_slRecorder)->Destroy(_slRecorder);
        _slRecorder = NULL;
        _slRecorderRecord = NULL;
        _slRecorderRecord = NULL;
    }

    _recIsInitialized = false;
    _recording = false;
    _recWarning = 0;
    _recError = 0;
    _recQueueSeq = 0;
    return 0;
}

bool AudioDeviceAndroidOpenSLES::RecordingIsInitialized() const {

    return _recIsInitialized;
}


bool AudioDeviceAndroidOpenSLES::Recording() const {

    return _recording;
}

bool AudioDeviceAndroidOpenSLES::PlayoutIsInitialized() const {

    return _playIsInitialized;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::StartPlayout() {

    CriticalSectionScoped lock(&_critSect);

    if (!_playIsInitialized) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Playout not initialized");
        return -1;
    }

    if (_playing) {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     "  Playout already started");
        return 0;
    }

    if (_slPlayerPlay == NULL) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "  PlayItf is NULL");
        return -1;
    }
    if (_slPlayerSimpleBufferQueue == NULL) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  PlayerSimpleBufferQueue is NULL");
        return -1;
    }

    _recQueueSeq = 0;

    WebRtc_Word32 res(-1);
    
    WebRtc_UWord32 nSample10ms = _adbSampleRate / 100;
    WebRtc_Word8 playBuffer[2 * nSample10ms];
    WebRtc_UWord32 noSamplesOut(0);
    {
        noSamplesOut = _ptrAudioBuffer->RequestPlayoutData(nSample10ms);
        
        
        noSamplesOut = _ptrAudioBuffer->GetPlayoutData(playBuffer);
        
        memcpy(_playQueueBuffer[_playQueueSeq], playBuffer, 2 * noSamplesOut);
        

        
        
        
        
        res = (*_slPlayerSimpleBufferQueue)->Enqueue(
            _slPlayerSimpleBufferQueue,
            (void*) _playQueueBuffer[_playQueueSeq],
            2 * noSamplesOut);
        if (res != SL_RESULT_SUCCESS) {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "  player simpler buffer queue Enqueue failed, %d",
                         noSamplesOut);
            
        }
        _playQueueSeq = (_playQueueSeq + 1) % N_PLAY_QUEUE_BUFFERS;
    }

    
    res = (*_slPlayerPlay)->SetPlayState(_slPlayerPlay, SL_PLAYSTATE_PLAYING);
    if (res != SL_RESULT_SUCCESS) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  failed to start playout");
        return -1;
    }

    _playWarning = 0;
    _playError = 0;
    _playing = true;

    return 0;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::StopPlayout() {

    CriticalSectionScoped lock(&_critSect);

    if (!_playIsInitialized) {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     "  Playout is not initialized");
        return 0;
    }

    if ((_slPlayerPlay != NULL) && (_slOutputMixObject == NULL) && (_slPlayer
            == NULL)) {
        
        WebRtc_Word32 res =
                (*_slPlayerPlay)->SetPlayState(_slPlayerPlay,
                                               SL_PLAYSTATE_STOPPED);
        if (res != SL_RESULT_SUCCESS) {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  failed to stop playout");
            return -1;
        }
        res = (*_slPlayerSimpleBufferQueue)->Clear(_slPlayerSimpleBufferQueue);
        if (res != SL_RESULT_SUCCESS) {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  failed to clear recorder buffer queue");
            return -1;
        }

        
        (*_slPlayer)->Destroy(_slPlayer);
        
        (*_slOutputMixObject)->Destroy(_slOutputMixObject);
        _slPlayer = NULL;
        _slPlayerPlay = NULL;
        _slPlayerSimpleBufferQueue = NULL;
        _slOutputMixObject = NULL;
    }

    _playIsInitialized = false;
    _playing = false;
    _playWarning = 0;
    _playError = 0;
    _playQueueSeq = 0;

    return 0;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::PlayoutDelay(WebRtc_UWord16& delayMS) const {
    delayMS = _playoutDelay;

    return 0;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::RecordingDelay(WebRtc_UWord16& delayMS) const {
    delayMS = _recordingDelay;

    return 0;
}

bool AudioDeviceAndroidOpenSLES::Playing() const {

    return _playing;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::SetPlayoutBuffer(
    const AudioDeviceModule::BufferType ,
    WebRtc_UWord16 ) {

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::PlayoutBuffer(
    AudioDeviceModule::BufferType& type,
    WebRtc_UWord16& sizeMS) const {

    type = AudioDeviceModule::kAdaptiveBufferSize;
    sizeMS = _playoutDelay; 

    return 0;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::CPULoad(WebRtc_UWord16& ) const {

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

bool AudioDeviceAndroidOpenSLES::PlayoutWarning() const {
    return (_playWarning > 0);
}

bool AudioDeviceAndroidOpenSLES::PlayoutError() const {
    return (_playError > 0);
}

bool AudioDeviceAndroidOpenSLES::RecordingWarning() const {
    return (_recWarning > 0);
}

bool AudioDeviceAndroidOpenSLES::RecordingError() const {
    return (_recError > 0);
}

void AudioDeviceAndroidOpenSLES::ClearPlayoutWarning() {
    _playWarning = 0;
}

void AudioDeviceAndroidOpenSLES::ClearPlayoutError() {
    _playError = 0;
}

void AudioDeviceAndroidOpenSLES::ClearRecordingWarning() {
    _recWarning = 0;
}

void AudioDeviceAndroidOpenSLES::ClearRecordingError() {
    _recError = 0;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::SetLoudspeakerStatus(bool enable) {
    _loudSpeakerOn = enable;
    return 0;
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::GetLoudspeakerStatus(
    bool& enabled) const {

    enabled = _loudSpeakerOn;
    return 0;
}





void AudioDeviceAndroidOpenSLES::PlayerSimpleBufferQueueCallback(
    SLAndroidSimpleBufferQueueItf queueItf,
    void *pContext) {
    AudioDeviceAndroidOpenSLES* ptrThis =
            static_cast<AudioDeviceAndroidOpenSLES*> (pContext);
    ptrThis->PlayerSimpleBufferQueueCallbackHandler(queueItf);
}

void AudioDeviceAndroidOpenSLES::PlayerSimpleBufferQueueCallbackHandler(
    SLAndroidSimpleBufferQueueItf queueItf) {
    WebRtc_Word32 res;
    
    
    
    if (_playing && (_playQueueSeq < N_PLAY_QUEUE_BUFFERS)) {
        
        
        unsigned int noSamp10ms = _adbSampleRate / 100;
        
        WebRtc_Word8 playBuffer[2 * noSamp10ms];
        int noSamplesOut = 0;

        
        

        
        

        noSamplesOut = _ptrAudioBuffer->RequestPlayoutData(noSamp10ms);
        
        
        noSamplesOut = _ptrAudioBuffer->GetPlayoutData(playBuffer);
        
        if (noSamp10ms != (unsigned int) noSamplesOut) {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "noSamp10ms (%u) != noSamplesOut (%d)", noSamp10ms,
                         noSamplesOut);

            if (_playWarning > 0) {
                WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                             "  Pending play warning exists");
            }
            _playWarning = 1;
        }
        
        memcpy(_playQueueBuffer[_playQueueSeq], playBuffer, 2 * noSamplesOut);
        

        
        
        
        res = (*_slPlayerSimpleBufferQueue)->Enqueue(
            _slPlayerSimpleBufferQueue,
            _playQueueBuffer[_playQueueSeq],
            2 * noSamplesOut);
        if (res != SL_RESULT_SUCCESS) {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "  player simpler buffer queue Enqueue failed, %d",
                         noSamplesOut);
            return;
        }
        
        UpdatePlayoutDelay(noSamplesOut);
        
        _playQueueSeq = (_playQueueSeq + 1) % N_PLAY_QUEUE_BUFFERS;
    }
}

void AudioDeviceAndroidOpenSLES::RecorderSimpleBufferQueueCallback(
    SLAndroidSimpleBufferQueueItf queueItf,
    void *pContext) {
    AudioDeviceAndroidOpenSLES* ptrThis =
            static_cast<AudioDeviceAndroidOpenSLES*> (pContext);
    ptrThis->RecorderSimpleBufferQueueCallbackHandler(queueItf);
}

void AudioDeviceAndroidOpenSLES::RecorderSimpleBufferQueueCallbackHandler(
    SLAndroidSimpleBufferQueueItf queueItf) {
    WebRtc_Word32 res;
    
    
    if (_recording) {
        
        
        
        

        const unsigned int noSamp10ms = _adbSampleRate / 100;
        
        
        unsigned int dataPos = 0;
        WebRtc_UWord16 bufPos = 0;
        WebRtc_Word16 insertPos = -1;
        unsigned int nCopy = 0; 
        

        while (dataPos < noSamp10ms)

        {
            
            
            
            
            bufPos = 0;
            insertPos = -1;
            nCopy = 0;
            while (bufPos < N_REC_BUFFERS)
            {
                if ((_recLength[bufPos] > 0) && (_recLength[bufPos]
                                < noSamp10ms))
                {
                    
                    insertPos = static_cast<WebRtc_Word16> (bufPos);
                    bufPos = N_REC_BUFFERS; 
                }
                else if ((-1 == insertPos) && (0 == _recLength[bufPos]))
                {
                    
                    insertPos = static_cast<WebRtc_Word16> (bufPos);
                }
                ++bufPos;
            }

            if (insertPos > -1)
            {
                
                
                unsigned int dataToCopy = noSamp10ms - dataPos;
                unsigned int currentRecLen = _recLength[insertPos];
                unsigned int roomInBuffer = noSamp10ms - currentRecLen;
                nCopy = (dataToCopy < roomInBuffer ? dataToCopy : roomInBuffer);
                memcpy(&_recBuffer[insertPos][currentRecLen],
                        &_recQueueBuffer[_recQueueSeq][dataPos],
                        nCopy * sizeof(short));
                if (0 == currentRecLen)
                {
                    _recSeqNumber[insertPos] = _recCurrentSeq;
                    ++_recCurrentSeq;
                }
                _recBufferTotalSize += nCopy;
                
                
                _recLength[insertPos] += nCopy;
                dataPos += nCopy;
            }
            else
            {
                
                WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice,
                        _id, "  Could not insert into recording buffer");
                if (_recWarning > 0)
                {
                    WEBRTC_TRACE(kTraceWarning,
                            kTraceAudioDevice, _id,
                            "  Pending rec warning exists");
                }
                _recWarning = 1;
                dataPos = noSamp10ms; 
            }
        }

        
        
        memset(_recQueueBuffer[_recQueueSeq], 0, 2 * REC_BUF_SIZE_IN_SAMPLES);
        
        res = (*_slRecorderSimpleBufferQueue)->Enqueue(
              _slRecorderSimpleBufferQueue,
              (void*) _recQueueBuffer[_recQueueSeq],
              2 * noSamp10ms);
        if (res != SL_RESULT_SUCCESS) {
            return;
        }
        
        _recQueueSeq = (_recQueueSeq + 1) % N_REC_QUEUE_BUFFERS;
        
        _timeEventRec.Set();
    }
}

void AudioDeviceAndroidOpenSLES::CheckErr(SLresult res) {
    if (res != SL_RESULT_SUCCESS) {
        
        exit(-1);
    }
}

void AudioDeviceAndroidOpenSLES::UpdatePlayoutDelay(
    WebRtc_UWord32 nSamplePlayed) {
    
    
    
    
    
    _playoutDelay = (N_PLAY_QUEUE_BUFFERS - 0.5) * 10 + N_PLAY_QUEUE_BUFFERS
            * nSamplePlayed / (_adbSampleRate / 1000);
}

void AudioDeviceAndroidOpenSLES::UpdateRecordingDelay() {
    
    
    _recordingDelay = 10;
    const WebRtc_UWord32 noSamp10ms = _adbSampleRate / 100;
    
    
    _recordingDelay += (N_REC_QUEUE_BUFFERS * noSamp10ms) / (_adbSampleRate
            / 1000);
    
}

WebRtc_Word32 AudioDeviceAndroidOpenSLES::InitSampleRate() {
    if (_slEngineObject == NULL) {
      WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "  SL Object is NULL");
      return -1;
    }

    _samplingRateIn = SL_SAMPLINGRATE_16;
    _samplingRateOut = SL_SAMPLINGRATE_16;
    _adbSampleRate = 16000;

    WEBRTC_TRACE(kTraceStateInfo, kTraceAudioDevice, _id,
                 "  sample rate set to (%d)", _adbSampleRate);
    return 0;

}





bool AudioDeviceAndroidOpenSLES::RecThreadFunc(void* pThis) {
  return (static_cast<AudioDeviceAndroidOpenSLES*>(pThis)->RecThreadProcess());
}

bool AudioDeviceAndroidOpenSLES::RecThreadProcess() {

    
    
    
    _timeEventRec.Wait(100);

    int bufPos = 0;
    unsigned int lowestSeq = 0;
    int lowestSeqBufPos = 0;
    bool foundBuf = true;
    const unsigned int noSamp10ms = _adbSampleRate / 100;

    while (foundBuf)
    {
        
        
        
        foundBuf = false;

        for (bufPos = 0; bufPos < N_REC_BUFFERS; ++bufPos)
        {
            if (noSamp10ms == _recLength[bufPos])
            {
                if (!foundBuf) {
                    lowestSeq = _recSeqNumber[bufPos];
                    lowestSeqBufPos = bufPos;
                    foundBuf = true;
                } else if (_recSeqNumber[bufPos] < lowestSeq)
                {
                    lowestSeq = _recSeqNumber[bufPos];
                    lowestSeqBufPos = bufPos;
                }
            }
        } 

        
        if (foundBuf)
        {
            UpdateRecordingDelay();
            
            _ptrAudioBuffer->SetRecordedBuffer(_recBuffer[lowestSeqBufPos],
                                               noSamp10ms);

            
            
            

            
            _ptrAudioBuffer->SetVQEData(_playoutDelay, _recordingDelay, 0);

            
            
            
            _ptrAudioBuffer->DeliverRecordedData();
            

            
            _recSeqNumber[lowestSeqBufPos] = 0;
            _recBufferTotalSize -= _recLength[lowestSeqBufPos];
            
            _recLength[lowestSeqBufPos] = 0;
        }

    } 
    
    return true;
}

} 

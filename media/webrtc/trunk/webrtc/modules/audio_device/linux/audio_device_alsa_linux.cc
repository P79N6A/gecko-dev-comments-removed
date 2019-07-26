









#include <assert.h>

#include "webrtc/modules/audio_device/audio_device_config.h"
#include "webrtc/modules/audio_device/audio_device_utility.h"
#include "webrtc/modules/audio_device/linux/audio_device_alsa_linux.h"

#include "webrtc/system_wrappers/interface/event_wrapper.h"
#include "webrtc/system_wrappers/interface/sleep.h"
#include "webrtc/system_wrappers/interface/thread_wrapper.h"
#include "webrtc/system_wrappers/interface/trace.h"

#include "Latency.h"

#define LOG_FIRST_CAPTURE(x) LogTime(AsyncLatencyLogger::AudioCaptureBase, \
                                     reinterpret_cast<uint64_t>(x), 0)
#define LOG_CAPTURE_FRAMES(x, frames) LogLatency(AsyncLatencyLogger::AudioCapture, \
                                                 reinterpret_cast<uint64_t>(x), frames)

webrtc_adm_linux_alsa::AlsaSymbolTable AlsaSymbolTable;




#define LATE(sym) \
  LATESYM_GET(webrtc_adm_linux_alsa::AlsaSymbolTable, &AlsaSymbolTable, sym)


#undef snd_ctl_card_info_alloca
#define snd_ctl_card_info_alloca(ptr) \
        do { *ptr = (snd_ctl_card_info_t *) \
            __builtin_alloca (LATE(snd_ctl_card_info_sizeof)()); \
            memset(*ptr, 0, LATE(snd_ctl_card_info_sizeof)()); } while (0)

#undef snd_pcm_info_alloca
#define snd_pcm_info_alloca(pInfo) \
       do { *pInfo = (snd_pcm_info_t *) \
       __builtin_alloca (LATE(snd_pcm_info_sizeof)()); \
       memset(*pInfo, 0, LATE(snd_pcm_info_sizeof)()); } while (0)


void WebrtcAlsaErrorHandler(const char *file,
                          int line,
                          const char *function,
                          int err,
                          const char *fmt,...){};

namespace webrtc
{
static const unsigned int ALSA_PLAYOUT_FREQ = 48000;
static const unsigned int ALSA_PLAYOUT_CH = 2;
static const unsigned int ALSA_PLAYOUT_LATENCY = 40*1000; 
static const unsigned int ALSA_CAPTURE_FREQ = 48000;
static const unsigned int ALSA_CAPTURE_CH = 2;
static const unsigned int ALSA_CAPTURE_LATENCY = 40*1000; 
static const unsigned int ALSA_PLAYOUT_WAIT_TIMEOUT = 5; 
static const unsigned int ALSA_CAPTURE_WAIT_TIMEOUT = 5; 

#define FUNC_GET_NUM_OF_DEVICE 0
#define FUNC_GET_DEVICE_NAME 1
#define FUNC_GET_DEVICE_NAME_FOR_AN_ENUM 2

AudioDeviceLinuxALSA::AudioDeviceLinuxALSA(const int32_t id) :
    _ptrAudioBuffer(NULL),
    _critSect(*CriticalSectionWrapper::CreateCriticalSection()),
    _ptrThreadRec(NULL),
    _ptrThreadPlay(NULL),
    _recThreadID(0),
    _playThreadID(0),
    _id(id),
    _mixerManager(id),
    _inputDeviceIndex(0),
    _outputDeviceIndex(0),
    _inputDeviceIsSpecified(false),
    _outputDeviceIsSpecified(false),
    _handleRecord(NULL),
    _handlePlayout(NULL),
    _recordingBuffersizeInFrame(0),
    _recordingPeriodSizeInFrame(0),
    _playoutBufferSizeInFrame(0),
    _playoutPeriodSizeInFrame(0),
    _recordingBufferSizeIn10MS(0),
    _playoutBufferSizeIn10MS(0),
    _recordingFramesIn10MS(0),
    _playoutFramesIn10MS(0),
    _recordingFreq(ALSA_CAPTURE_FREQ),
    _playoutFreq(ALSA_PLAYOUT_FREQ),
    _recChannels(ALSA_CAPTURE_CH),
    _playChannels(ALSA_PLAYOUT_CH),
    _recordingBuffer(NULL),
    _playoutBuffer(NULL),
    _recordingFramesLeft(0),
    _playoutFramesLeft(0),
    _playBufType(AudioDeviceModule::kFixedBufferSize),
    _initialized(false),
    _recording(false),
    _firstRecord(true),
    _playing(false),
    _recIsInitialized(false),
    _playIsInitialized(false),
    _AGC(false),
    _recordingDelay(0),
    _playoutDelay(0),
    _playWarning(0),
    _playError(0),
    _recWarning(0),
    _recError(0),
    _playBufDelay(80),
    _playBufDelayFixed(80)
{
    memset(_oldKeyState, 0, sizeof(_oldKeyState));
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, id,
                 "%s created", __FUNCTION__);
}





AudioDeviceLinuxALSA::~AudioDeviceLinuxALSA()
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id,
                 "%s destroyed", __FUNCTION__);
    
    Terminate();

    
    if (_recordingBuffer)
    {
        delete [] _recordingBuffer;
        _recordingBuffer = NULL;
    }
    if (_playoutBuffer)
    {
        delete [] _playoutBuffer;
        _playoutBuffer = NULL;
    }
    delete &_critSect;
}

void AudioDeviceLinuxALSA::AttachAudioBuffer(AudioDeviceBuffer* audioBuffer)
{

    CriticalSectionScoped lock(&_critSect);

    _ptrAudioBuffer = audioBuffer;

    
    
    
    _ptrAudioBuffer->SetRecordingSampleRate(0);
    _ptrAudioBuffer->SetPlayoutSampleRate(0);
    _ptrAudioBuffer->SetRecordingChannels(0);
    _ptrAudioBuffer->SetPlayoutChannels(0);
}

int32_t AudioDeviceLinuxALSA::ActiveAudioLayer(
    AudioDeviceModule::AudioLayer& audioLayer) const
{
    audioLayer = AudioDeviceModule::kLinuxAlsaAudio;
    return 0;
}

int32_t AudioDeviceLinuxALSA::Init()
{

    CriticalSectionScoped lock(&_critSect);

    
    if (!AlsaSymbolTable.Load())
    {
        
        
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                   "  failed to load symbol table");
        return -1;
    }

    if (_initialized)
    {
        return 0;
    }

#ifdef USE_X11
    
    _XDisplay = XOpenDisplay(NULL);
    if (!_XDisplay)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
          "  failed to open X display, typing detection will not work");
    }
#endif

    _playWarning = 0;
    _playError = 0;
    _recWarning = 0;
    _recError = 0;

    _initialized = true;

    return 0;
}

int32_t AudioDeviceLinuxALSA::Terminate()
{

    if (!_initialized)
    {
        return 0;
    }

    CriticalSectionScoped lock(&_critSect);

    _mixerManager.Close();

    
    if (_ptrThreadRec)
    {
        ThreadWrapper* tmpThread = _ptrThreadRec;
        _ptrThreadRec = NULL;
        _critSect.Leave();

        tmpThread->SetNotAlive();

        if (tmpThread->Stop())
        {
            delete tmpThread;
        }
        else
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "  failed to close down the rec audio thread");
        }

        _critSect.Enter();
    }

    
    if (_ptrThreadPlay)
    {
        ThreadWrapper* tmpThread = _ptrThreadPlay;
        _ptrThreadPlay = NULL;
        _critSect.Leave();

        tmpThread->SetNotAlive();

        if (tmpThread->Stop())
        {
            delete tmpThread;
        }
        else
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "  failed to close down the play audio thread");
        }

        _critSect.Enter();
    }

#ifdef USE_X11
    if (_XDisplay)
    {
      XCloseDisplay(_XDisplay);
      _XDisplay = NULL;
    }
#endif

    _initialized = false;
    _outputDeviceIsSpecified = false;
    _inputDeviceIsSpecified = false;

    return 0;
}

bool AudioDeviceLinuxALSA::Initialized() const
{
    return (_initialized);
}

int32_t AudioDeviceLinuxALSA::SpeakerIsAvailable(bool& available)
{

    bool wasInitialized = _mixerManager.SpeakerIsInitialized();

    
    
    
    if (!wasInitialized && InitSpeaker() == -1)
    {
        available = false;
        return 0;
    }

    
    
    available = true;

    
    
    if (!wasInitialized)
    {
        _mixerManager.CloseSpeaker();
    }

    return 0;
}

int32_t AudioDeviceLinuxALSA::InitSpeaker()
{

    CriticalSectionScoped lock(&_critSect);

    if (_playing)
    {
        return -1;
    }

    char devName[kAdmMaxDeviceNameSize] = {0};
    GetDevicesInfo(2, true, _outputDeviceIndex, devName, kAdmMaxDeviceNameSize);
    return _mixerManager.OpenSpeaker(devName);
}

int32_t AudioDeviceLinuxALSA::MicrophoneIsAvailable(bool& available)
{

    bool wasInitialized = _mixerManager.MicrophoneIsInitialized();

    
    
    
    if (!wasInitialized && InitMicrophone() == -1)
    {
        available = false;
        return 0;
    }

    
    
    available = true;

    
    
    if (!wasInitialized)
    {
        _mixerManager.CloseMicrophone();
    }

    return 0;
}

int32_t AudioDeviceLinuxALSA::InitMicrophone()
{

    CriticalSectionScoped lock(&_critSect);

    if (_recording)
    {
        return -1;
    }

    char devName[kAdmMaxDeviceNameSize] = {0};
    GetDevicesInfo(2, false, _inputDeviceIndex, devName, kAdmMaxDeviceNameSize);
    return _mixerManager.OpenMicrophone(devName);
}

bool AudioDeviceLinuxALSA::SpeakerIsInitialized() const
{
    return (_mixerManager.SpeakerIsInitialized());
}

bool AudioDeviceLinuxALSA::MicrophoneIsInitialized() const
{
    return (_mixerManager.MicrophoneIsInitialized());
}

int32_t AudioDeviceLinuxALSA::SpeakerVolumeIsAvailable(bool& available)
{

    bool wasInitialized = _mixerManager.SpeakerIsInitialized();

    
    
    if (!wasInitialized && InitSpeaker() == -1)
    {
        
        
        available = false;
        return 0;
    }

    
    
    available = true;

    
    if (!wasInitialized)
    {
        _mixerManager.CloseSpeaker();
    }

    return 0;
}

int32_t AudioDeviceLinuxALSA::SetSpeakerVolume(uint32_t volume)
{

    return (_mixerManager.SetSpeakerVolume(volume));
}

int32_t AudioDeviceLinuxALSA::SpeakerVolume(uint32_t& volume) const
{

    uint32_t level(0);

    if (_mixerManager.SpeakerVolume(level) == -1)
    {
        return -1;
    }

    volume = level;
    
    return 0;
}


int32_t AudioDeviceLinuxALSA::SetWaveOutVolume(uint16_t volumeLeft,
                                               uint16_t volumeRight)
{

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

int32_t AudioDeviceLinuxALSA::WaveOutVolume(
    uint16_t& ,
    uint16_t& ) const
{

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

int32_t AudioDeviceLinuxALSA::MaxSpeakerVolume(
    uint32_t& maxVolume) const
{

    uint32_t maxVol(0);

    if (_mixerManager.MaxSpeakerVolume(maxVol) == -1)
    {
        return -1;
    }

    maxVolume = maxVol;
    
    return 0;
}

int32_t AudioDeviceLinuxALSA::MinSpeakerVolume(
    uint32_t& minVolume) const
{

    uint32_t minVol(0);

    if (_mixerManager.MinSpeakerVolume(minVol) == -1)
    {
        return -1;
    }

    minVolume = minVol;
    
    return 0;
}

int32_t AudioDeviceLinuxALSA::SpeakerVolumeStepSize(
    uint16_t& stepSize) const
{

    uint16_t delta(0); 
     
    if (_mixerManager.SpeakerVolumeStepSize(delta) == -1)
    {
        return -1;
    }

    stepSize = delta;

    return 0;
}

int32_t AudioDeviceLinuxALSA::SpeakerMuteIsAvailable(bool& available)
{

    bool isAvailable(false);
    bool wasInitialized = _mixerManager.SpeakerIsInitialized();

    
    
    
    if (!wasInitialized && InitSpeaker() == -1)
    {
        
        
        
        available = false;
        return 0;
    }

    
    _mixerManager.SpeakerMuteIsAvailable(isAvailable);

    available = isAvailable;

    
    if (!wasInitialized)
    {
        _mixerManager.CloseSpeaker();
    }

    return 0;
}

int32_t AudioDeviceLinuxALSA::SetSpeakerMute(bool enable)
{
    return (_mixerManager.SetSpeakerMute(enable));
}

int32_t AudioDeviceLinuxALSA::SpeakerMute(bool& enabled) const
{

    bool muted(0); 
        
    if (_mixerManager.SpeakerMute(muted) == -1)
    {
        return -1;
    }

    enabled = muted;
    
    return 0;
}

int32_t AudioDeviceLinuxALSA::MicrophoneMuteIsAvailable(bool& available)
{

    bool isAvailable(false);
    bool wasInitialized = _mixerManager.MicrophoneIsInitialized();

    
    
    
    if (!wasInitialized && InitMicrophone() == -1)
    {
        
        
        
        available = false;
        return 0;
    }

    
    
    _mixerManager.MicrophoneMuteIsAvailable(isAvailable);
    available = isAvailable;

    
    
    if (!wasInitialized)
    {
        _mixerManager.CloseMicrophone();
    }

    return 0;
}

int32_t AudioDeviceLinuxALSA::SetMicrophoneMute(bool enable)
{
    return (_mixerManager.SetMicrophoneMute(enable));
}





int32_t AudioDeviceLinuxALSA::MicrophoneMute(bool& enabled) const
{

    bool muted(0); 
        
    if (_mixerManager.MicrophoneMute(muted) == -1)
    {
        return -1;
    }

    enabled = muted;
    return 0;
}

int32_t AudioDeviceLinuxALSA::MicrophoneBoostIsAvailable(bool& available)
{
    
    bool isAvailable(false);
    bool wasInitialized = _mixerManager.MicrophoneIsInitialized();

    
    
    
    if (!wasInitialized && InitMicrophone() == -1)
    {
        
        
        
        available = false;
        return 0;
    }

    
    _mixerManager.MicrophoneBoostIsAvailable(isAvailable);
    available = isAvailable;

    
    if (!wasInitialized)
    {
        _mixerManager.CloseMicrophone();
    }

    return 0;
}

int32_t AudioDeviceLinuxALSA::SetMicrophoneBoost(bool enable)
{

    return (_mixerManager.SetMicrophoneBoost(enable));
}

int32_t AudioDeviceLinuxALSA::MicrophoneBoost(bool& enabled) const
{

    bool onOff(0); 
        
    if (_mixerManager.MicrophoneBoost(onOff) == -1)
    {
        return -1;
    }

    enabled = onOff;
    
    return 0;
}

int32_t AudioDeviceLinuxALSA::StereoRecordingIsAvailable(bool& available)
{

    CriticalSectionScoped lock(&_critSect);

    
    if (_recIsInitialized && (2 == _recChannels))
    {
        available = true;
        return 0;
    }

    
    bool recIsInitialized = _recIsInitialized;
    bool recording = _recording;
    int recChannels = _recChannels;

    available = false;
    
    
    if (_recIsInitialized)
    {
        StopRecording();
    }

    
    _recChannels = 2;
    if (InitRecording() == 0)
    {
        available = true;
    }

    
    StopRecording();

    
    _recChannels = recChannels;
    if (recIsInitialized)
    {
        InitRecording();
    }
    if (recording)
    {
        StartRecording();
    }

    return 0;
}

int32_t AudioDeviceLinuxALSA::SetStereoRecording(bool enable)
{

    if (enable)
        _recChannels = 2;
    else
        _recChannels = 1;

    return 0;
}

int32_t AudioDeviceLinuxALSA::StereoRecording(bool& enabled) const
{

    if (_recChannels == 2)
        enabled = true;
    else
        enabled = false;

    return 0;
}

int32_t AudioDeviceLinuxALSA::StereoPlayoutIsAvailable(bool& available)
{

    CriticalSectionScoped lock(&_critSect);

    
    if (_playIsInitialized && (2 == _playChannels))
    {
        available = true;
        return 0;
    }

    
    bool playIsInitialized = _playIsInitialized;
    bool playing = _playing;
    int playChannels = _playChannels;

    available = false;
    
    
    if (_playIsInitialized)
    {
        StopPlayout();
    }

    
    _playChannels = 2;
    if (InitPlayout() == 0)
    {
        available = true;
    }

    
    StopPlayout();

    
    _playChannels = playChannels;
    if (playIsInitialized)
    {
        InitPlayout();
    }
    if (playing)
    {
        StartPlayout();
    }

    return 0;
}

int32_t AudioDeviceLinuxALSA::SetStereoPlayout(bool enable)
{

    if (enable)
        _playChannels = 2;
    else
        _playChannels = 1;

    return 0;
}

int32_t AudioDeviceLinuxALSA::StereoPlayout(bool& enabled) const
{

    if (_playChannels == 2)
        enabled = true;
    else
        enabled = false;

    return 0;
}

int32_t AudioDeviceLinuxALSA::SetAGC(bool enable)
{

    _AGC = enable;

    return 0;
}

bool AudioDeviceLinuxALSA::AGC() const
{

    return _AGC;
}

int32_t AudioDeviceLinuxALSA::MicrophoneVolumeIsAvailable(bool& available)
{

    bool wasInitialized = _mixerManager.MicrophoneIsInitialized();

    
    
    if (!wasInitialized && InitMicrophone() == -1)
    {
        
        
        available = false;
        return 0;
    }

    
    
    available = true;

    
    if (!wasInitialized)
    {
        _mixerManager.CloseMicrophone();
    }

    return 0;
}

int32_t AudioDeviceLinuxALSA::SetMicrophoneVolume(uint32_t volume)
{

    return (_mixerManager.SetMicrophoneVolume(volume));
 
    return 0;
}

int32_t AudioDeviceLinuxALSA::MicrophoneVolume(uint32_t& volume) const
{

    uint32_t level(0);

    if (_mixerManager.MicrophoneVolume(level) == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  failed to retrive current microphone level");
        return -1;
    }

    volume = level;
    
    return 0;
}

int32_t AudioDeviceLinuxALSA::MaxMicrophoneVolume(
    uint32_t& maxVolume) const
{

    uint32_t maxVol(0);

    if (_mixerManager.MaxMicrophoneVolume(maxVol) == -1)
    {
        return -1;
    }

    maxVolume = maxVol;

    return 0;
}

int32_t AudioDeviceLinuxALSA::MinMicrophoneVolume(
    uint32_t& minVolume) const
{

    uint32_t minVol(0);

    if (_mixerManager.MinMicrophoneVolume(minVol) == -1)
    {
        return -1;
    }

    minVolume = minVol;

    return 0;
}

int32_t AudioDeviceLinuxALSA::MicrophoneVolumeStepSize(
    uint16_t& stepSize) const
{

    uint16_t delta(0); 
        
    if (_mixerManager.MicrophoneVolumeStepSize(delta) == -1)
    {
        return -1;
    }

    stepSize = delta;

    return 0;
}

int16_t AudioDeviceLinuxALSA::PlayoutDevices()
{

    return (int16_t)GetDevicesInfo(0, true);
}

int32_t AudioDeviceLinuxALSA::SetPlayoutDevice(uint16_t index)
{

    if (_playIsInitialized)
    {
        return -1;
    }

    uint32_t nDevices = GetDevicesInfo(0, true);
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "  number of availiable audio output devices is %u", nDevices);

    if (index > (nDevices-1))
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  device index is out of range [0,%u]", (nDevices-1));
        return -1;
    }

    _outputDeviceIndex = index;
    _outputDeviceIsSpecified = true;

    return 0;
}

int32_t AudioDeviceLinuxALSA::SetPlayoutDevice(
    AudioDeviceModule::WindowsDeviceType )
{
    WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                 "WindowsDeviceType not supported");
    return -1;
}

int32_t AudioDeviceLinuxALSA::PlayoutDeviceName(
    uint16_t index,
    char name[kAdmMaxDeviceNameSize],
    char guid[kAdmMaxGuidSize])
{

    const uint16_t nDevices(PlayoutDevices());

    if ((index > (nDevices-1)) || (name == NULL))
    {
        return -1;
    }

    memset(name, 0, kAdmMaxDeviceNameSize);

    if (guid != NULL)
    {
        memset(guid, 0, kAdmMaxGuidSize);
    }

    return GetDevicesInfo(1, true, index, name, kAdmMaxDeviceNameSize);
}

int32_t AudioDeviceLinuxALSA::RecordingDeviceName(
    uint16_t index,
    char name[kAdmMaxDeviceNameSize],
    char guid[kAdmMaxGuidSize])
{

    const uint16_t nDevices(RecordingDevices());

    if ((index > (nDevices-1)) || (name == NULL))
    {
        return -1;
    }

    memset(name, 0, kAdmMaxDeviceNameSize);

    if (guid != NULL)
    {
        memset(guid, 0, kAdmMaxGuidSize);
    }
    
    return GetDevicesInfo(1, false, index, name, kAdmMaxDeviceNameSize,
                          guid, kAdmMaxGuidSize);
}

int16_t AudioDeviceLinuxALSA::RecordingDevices()
{

    return (int16_t)GetDevicesInfo(0, false);
}

int32_t AudioDeviceLinuxALSA::SetRecordingDevice(uint16_t index)
{

    if (_recIsInitialized)
    {
        return -1;
    }

    uint32_t nDevices = GetDevicesInfo(0, false);
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "  number of availiable audio input devices is %u", nDevices);

    if (index > (nDevices-1))
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  device index is out of range [0,%u]", (nDevices-1));
        return -1;
    }

    _inputDeviceIndex = index;
    _inputDeviceIsSpecified = true;

    return 0;
}





int32_t AudioDeviceLinuxALSA::SetRecordingDevice(
    AudioDeviceModule::WindowsDeviceType )
{
    WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                 "WindowsDeviceType not supported");
    return -1;
}

int32_t AudioDeviceLinuxALSA::PlayoutIsAvailable(bool& available)
{
    
    available = false;

    
    
    _playChannels = 1;
    int32_t res = InitPlayout();

    
    StopPlayout();

    if (res != -1)
    {
        available = true;
    }
    else
    {
        
        res = StereoPlayoutIsAvailable(available);
        if (available)
        {
            
            _playChannels = 2;
        }
    }
    
    return res;
}

int32_t AudioDeviceLinuxALSA::RecordingIsAvailable(bool& available)
{
    
    available = false;

    
    
    _recChannels = 1;
    int32_t res = InitRecording();

    
    StopRecording();

    if (res != -1)
    {
        available = true;
    }
    else
    {
        
        res = StereoRecordingIsAvailable(available);
        if (available)
        {
            
            _recChannels = 2;
        }
    }
    
    return res;
}

int32_t AudioDeviceLinuxALSA::InitPlayout()
{

    int errVal = 0;

    CriticalSectionScoped lock(&_critSect);
    if (_playing)
    {
        return -1;
    }

    if (!_outputDeviceIsSpecified)
    {
        return -1;
    }

    if (_playIsInitialized)
    {
        return 0;
    }
    
    if (InitSpeaker() == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  InitSpeaker() failed");
    }

    
    
    if (_handlePlayout != NULL)
    {
        LATE(snd_pcm_close)(_handlePlayout);
        _handlePlayout = NULL;
        _playIsInitialized = false;
        if (errVal < 0)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  Error closing current playout sound device, error:"
                         " %s", LATE(snd_strerror)(errVal));
        }
    }

    
    char deviceName[kAdmMaxDeviceNameSize] = {0};
    GetDevicesInfo(2, true, _outputDeviceIndex, deviceName,
                   kAdmMaxDeviceNameSize);

    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "  InitPlayout open (%s)", deviceName);

    errVal = LATE(snd_pcm_open)
                 (&_handlePlayout,
                  deviceName,
                  SND_PCM_STREAM_PLAYBACK,
                  SND_PCM_NONBLOCK);

    if (errVal == -EBUSY) 
    {
        for (int i=0; i < 5; i++)
        {
            SleepMs(1000);
            errVal = LATE(snd_pcm_open)
                         (&_handlePlayout,
                          deviceName,
                          SND_PCM_STREAM_PLAYBACK,
                          SND_PCM_NONBLOCK);
            if (errVal == 0)
            {
                break;
            }
        }
    }
    if (errVal < 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "     unable to open playback device: %s (%d)",
                     LATE(snd_strerror)(errVal),
                     errVal);
        _handlePlayout = NULL;
        return -1;
    }

    _playoutFramesIn10MS = _playoutFreq/100;
    if ((errVal = LATE(snd_pcm_set_params)( _handlePlayout,
#if defined(WEBRTC_BIG_ENDIAN)
        SND_PCM_FORMAT_S16_BE,
#else
        SND_PCM_FORMAT_S16_LE, 
#endif
        SND_PCM_ACCESS_RW_INTERLEAVED, 
        _playChannels, 
        _playoutFreq, 
        1, 
        ALSA_PLAYOUT_LATENCY 
    )) < 0)
    {   
        _playoutFramesIn10MS = 0;
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "     unable to set playback device: %s (%d)",
                     LATE(snd_strerror)(errVal),
                     errVal);
        ErrorRecovery(errVal, _handlePlayout);
        errVal = LATE(snd_pcm_close)(_handlePlayout);
        _handlePlayout = NULL;
        return -1;
    }

    errVal = LATE(snd_pcm_get_params)(_handlePlayout,
        &_playoutBufferSizeInFrame, &_playoutPeriodSizeInFrame);
    if (errVal < 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "    snd_pcm_get_params %s",
                     LATE(snd_strerror)(errVal),
                     errVal);
        _playoutBufferSizeInFrame = 0;
        _playoutPeriodSizeInFrame = 0;
    }
    else {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     "    playout snd_pcm_get_params "
                     "buffer_size:%d period_size :%d",
                     _playoutBufferSizeInFrame, _playoutPeriodSizeInFrame);
    }

    if (_ptrAudioBuffer)
    {
        
        _ptrAudioBuffer->SetPlayoutSampleRate(_playoutFreq);
        _ptrAudioBuffer->SetPlayoutChannels(_playChannels);
    }

    
    _playoutBufferSizeIn10MS = LATE(snd_pcm_frames_to_bytes)(
        _handlePlayout, _playoutFramesIn10MS);

    
    _playWarning = 0;
    _playError = 0;

    if (_handlePlayout != NULL)
    {
        _playIsInitialized = true;
        return 0;
    }
    else
    {
        return -1;
    }

    return 0;
}

int32_t AudioDeviceLinuxALSA::InitRecording()
{

    int errVal = 0;

    CriticalSectionScoped lock(&_critSect);

    if (_recording)
    {
        return -1;
    }

    if (!_inputDeviceIsSpecified)
    {
        return -1;
    }

    if (_recIsInitialized)
    {
        return 0;
    }

    
    if (InitMicrophone() == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                   "  InitMicrophone() failed");
    }

    
    
    if (_handleRecord != NULL)
    {
        int errVal = LATE(snd_pcm_close)(_handleRecord);
        _handleRecord = NULL;
        _recIsInitialized = false;
        if (errVal < 0)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "     Error closing current recording sound device,"
                         " error: %s",
                         LATE(snd_strerror)(errVal));
        }
    }

    
    
    char deviceName[kAdmMaxDeviceNameSize] = {0};
    GetDevicesInfo(2, false, _inputDeviceIndex, deviceName,
                   kAdmMaxDeviceNameSize);

    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "InitRecording open (%s)", deviceName);
    errVal = LATE(snd_pcm_open)
                 (&_handleRecord,
                  deviceName,
                  SND_PCM_STREAM_CAPTURE,
                  SND_PCM_NONBLOCK);

    
    if (errVal == -EBUSY) 
    {
        for (int i=0; i < 5; i++)
        {
            SleepMs(1000);
            errVal = LATE(snd_pcm_open)
                         (&_handleRecord,
                          deviceName,
                          SND_PCM_STREAM_CAPTURE,
                          SND_PCM_NONBLOCK);
            if (errVal == 0)
            {
                break;
            }
        }
    }
    if (errVal < 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "    unable to open record device: %s",
                     LATE(snd_strerror)(errVal));
        _handleRecord = NULL;
        return -1;
    }

    _recordingFramesIn10MS = _recordingFreq/100;
    if ((errVal = LATE(snd_pcm_set_params)(_handleRecord,
#if defined(WEBRTC_BIG_ENDIAN)
        SND_PCM_FORMAT_S16_BE, 
#else
        SND_PCM_FORMAT_S16_LE, 
#endif
        SND_PCM_ACCESS_RW_INTERLEAVED, 
        _recChannels, 
        _recordingFreq, 
        1, 
        ALSA_CAPTURE_LATENCY 
    )) < 0)
    {
         
         if (_recChannels == 1)
           _recChannels = 2;
         else
           _recChannels = 1;

         if ((errVal = LATE(snd_pcm_set_params)(_handleRecord,
#if defined(WEBRTC_BIG_ENDIAN)
             SND_PCM_FORMAT_S16_BE, 
#else
             SND_PCM_FORMAT_S16_LE, 
#endif
             SND_PCM_ACCESS_RW_INTERLEAVED, 
             _recChannels, 
             _recordingFreq, 
             1, 
             ALSA_CAPTURE_LATENCY 
         )) < 0)
         {
             _recordingFramesIn10MS = 0;
             WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                          "    unable to set record settings: %s (%d)",
                          LATE(snd_strerror)(errVal), errVal);
             ErrorRecovery(errVal, _handleRecord);
             errVal = LATE(snd_pcm_close)(_handleRecord);
             _handleRecord = NULL;
             return -1;
         }
    }

    errVal = LATE(snd_pcm_get_params)(_handleRecord,
        &_recordingBuffersizeInFrame, &_recordingPeriodSizeInFrame);
    if (errVal < 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "    snd_pcm_get_params %s",
                     LATE(snd_strerror)(errVal), errVal);
        _recordingBuffersizeInFrame = 0;
        _recordingPeriodSizeInFrame = 0;
    }
    else {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     "    capture snd_pcm_get_params "
                     "buffer_size:%d period_size:%d",
                     _recordingBuffersizeInFrame, _recordingPeriodSizeInFrame);
    }

    if (_ptrAudioBuffer)
    {
        
        _ptrAudioBuffer->SetRecordingSampleRate(_recordingFreq);
        _ptrAudioBuffer->SetRecordingChannels(_recChannels);
    }

    
    _recordingBufferSizeIn10MS = LATE(snd_pcm_frames_to_bytes)(
        _handleRecord, _recordingFramesIn10MS);

    if (_handleRecord != NULL)
    {
        
        _recIsInitialized = true;
        return 0;
    }
    else
    {
        return -1;
    }

    return 0;
}

int32_t AudioDeviceLinuxALSA::StartRecording()
{

    if (!_recIsInitialized)
    {
        return -1;
    }

    if (_recording)
    {
        return 0;
    }

    _recording = true;

    int errVal = 0;
    _recordingFramesLeft = _recordingFramesIn10MS;

    
    if (!_recordingBuffer)
        _recordingBuffer = new int8_t[_recordingBufferSizeIn10MS];
    if (!_recordingBuffer)
    {
        WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                     "   failed to alloc recording buffer");
        _recording = false;
        return -1;
    }
    
    const char* threadName = "webrtc_audio_module_capture_thread";
    _firstRecord = true;
    _ptrThreadRec = ThreadWrapper::CreateThread(RecThreadFunc,
                                                this,
                                                kRealtimePriority,
                                                threadName);
    if (_ptrThreadRec == NULL)
    {
        WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                     "  failed to create the rec audio thread");
        _recording = false;
        delete [] _recordingBuffer;
        _recordingBuffer = NULL;
        return -1;
    }

    unsigned int threadID(0);
    if (!_ptrThreadRec->Start(threadID))
    {
        WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                     "  failed to start the rec audio thread");
        _recording = false;
        delete _ptrThreadRec;
        _ptrThreadRec = NULL;
        delete [] _recordingBuffer;
        _recordingBuffer = NULL;
        return -1;
    }
    _recThreadID = threadID;

    errVal = LATE(snd_pcm_prepare)(_handleRecord);
    if (errVal < 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "     capture snd_pcm_prepare failed (%s)\n",
                     LATE(snd_strerror)(errVal));
        
        
    }

    errVal = LATE(snd_pcm_start)(_handleRecord);
    if (errVal < 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "     capture snd_pcm_start err: %s",
                     LATE(snd_strerror)(errVal));
        errVal = LATE(snd_pcm_start)(_handleRecord);
        if (errVal < 0)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "     capture snd_pcm_start 2nd try err: %s",
                         LATE(snd_strerror)(errVal));
            StopRecording();
            return -1;
        }
    }

    return 0;
}

int32_t AudioDeviceLinuxALSA::StopRecording()
{

    {
      CriticalSectionScoped lock(&_critSect);

      if (!_recIsInitialized)
      {
          return 0;
      }

      if (_handleRecord == NULL)
      {
          return -1;
      }

      
      _recIsInitialized = false;
      _recording = false;
    }

    if (_ptrThreadRec && !_ptrThreadRec->Stop())
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "    failed to stop the rec audio thread");
        return -1;
    }
    else {
        delete _ptrThreadRec;
        _ptrThreadRec = NULL;
    }

    CriticalSectionScoped lock(&_critSect);
    _recordingFramesLeft = 0;
    if (_recordingBuffer)
    {
        delete [] _recordingBuffer;
        _recordingBuffer = NULL;
    }

    
    int errVal = LATE(snd_pcm_drop)(_handleRecord);
    if (errVal < 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "     Error stop recording: %s",
                     LATE(snd_strerror)(errVal));
        return -1;
    }

    errVal = LATE(snd_pcm_close)(_handleRecord);
    if (errVal < 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "     Error closing record sound device, error: %s",
                     LATE(snd_strerror)(errVal));
        return -1;
    }

    
    bool muteEnabled = false;
    MicrophoneMute(muteEnabled);
    if (muteEnabled)
    {
        SetMicrophoneMute(false);
    }

    
    _handleRecord = NULL;
    return 0;
}

bool AudioDeviceLinuxALSA::RecordingIsInitialized() const
{
    return (_recIsInitialized);
}

bool AudioDeviceLinuxALSA::Recording() const
{
    return (_recording);
}

bool AudioDeviceLinuxALSA::PlayoutIsInitialized() const
{
    return (_playIsInitialized);
}

int32_t AudioDeviceLinuxALSA::StartPlayout()
{
    if (!_playIsInitialized)
    {
        return -1;
    }

    if (_playing)
    {
        return 0;
    }

    _playing = true;

    _playoutFramesLeft = 0;
    if (!_playoutBuffer)
        _playoutBuffer = new int8_t[_playoutBufferSizeIn10MS];
    if (!_playoutBuffer)
    {
      WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                   "    failed to alloc playout buf");
      _playing = false;
      return -1;
    }

    
    const char* threadName = "webrtc_audio_module_play_thread";
    _ptrThreadPlay =  ThreadWrapper::CreateThread(PlayThreadFunc,
                                                  this,
                                                  kRealtimePriority,
                                                  threadName);
    if (_ptrThreadPlay == NULL)
    {
        WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                     "    failed to create the play audio thread");
        _playing = false;
        delete [] _playoutBuffer;
        _playoutBuffer = NULL;
        return -1;
    }

    int errVal = LATE(snd_pcm_prepare)(_handlePlayout);
    if (errVal < 0)
    {
        WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                     "     playout snd_pcm_prepare failed (%s)\n",
                     LATE(snd_strerror)(errVal));
        
        
    }


    unsigned int threadID(0);
    if (!_ptrThreadPlay->Start(threadID))
    {
        WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                     "  failed to start the play audio thread");
        _playing = false;
        delete _ptrThreadPlay;
        _ptrThreadPlay = NULL;
        delete [] _playoutBuffer;
        _playoutBuffer = NULL;
        return -1;
    }
    _playThreadID = threadID;

    return 0;
}

int32_t AudioDeviceLinuxALSA::StopPlayout()
{

    {
        CriticalSectionScoped lock(&_critSect);

        if (!_playIsInitialized)
        {
            return 0;
        }

        if (_handlePlayout == NULL)
        {
            return -1;
        }

        _playing = false;
    }

    
    if (_ptrThreadPlay && !_ptrThreadPlay->Stop())
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  failed to stop the play audio thread");
        return -1;
    }
    else {
        delete _ptrThreadPlay;
        _ptrThreadPlay = NULL;
    }

    CriticalSectionScoped lock(&_critSect);

    _playoutFramesLeft = 0;
    delete [] _playoutBuffer;
    _playoutBuffer = NULL;

    
    int errVal = LATE(snd_pcm_drop)(_handlePlayout);
    if (errVal < 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "    Error stop playing: %s",
                     LATE(snd_strerror)(errVal));
    }

    errVal = LATE(snd_pcm_close)(_handlePlayout);
     if (errVal < 0)
         WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                      "    Error closing playout sound device, error: %s",
                      LATE(snd_strerror)(errVal));

     
     _playIsInitialized = false;
     _handlePlayout = NULL;
     WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                  "  handle_playout is now set to NULL");

     return 0;
}

int32_t AudioDeviceLinuxALSA::PlayoutDelay(uint16_t& delayMS) const
{
    delayMS = (uint16_t)_playoutDelay * 1000 / _playoutFreq;
    return 0;
}

int32_t AudioDeviceLinuxALSA::RecordingDelay(uint16_t& delayMS) const
{
    
    delayMS = (uint16_t)(10 + _recordingDelay * 1000 / _recordingFreq);
    return 0;
}

bool AudioDeviceLinuxALSA::Playing() const
{
    return (_playing);
}




int32_t AudioDeviceLinuxALSA::SetPlayoutBuffer(
    const AudioDeviceModule::BufferType type,
    uint16_t sizeMS)
{
    _playBufType = type;
    if (type == AudioDeviceModule::kFixedBufferSize)
    {
        _playBufDelayFixed = sizeMS;
    }
    return 0;
}

int32_t AudioDeviceLinuxALSA::PlayoutBuffer(
    AudioDeviceModule::BufferType& type,
    uint16_t& sizeMS) const
{
    type = _playBufType;
    if (type == AudioDeviceModule::kFixedBufferSize)
    {
        sizeMS = _playBufDelayFixed; 
    }
    else
    {
        sizeMS = _playBufDelay; 
    }

    return 0;
}

int32_t AudioDeviceLinuxALSA::CPULoad(uint16_t& load) const
{

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
               "  API call not supported on this platform");
    return -1;
}

bool AudioDeviceLinuxALSA::PlayoutWarning() const
{
    return (_playWarning > 0);
}

bool AudioDeviceLinuxALSA::PlayoutError() const
{
    return (_playError > 0);
}

bool AudioDeviceLinuxALSA::RecordingWarning() const
{
    return (_recWarning > 0);
}

bool AudioDeviceLinuxALSA::RecordingError() const
{
    return (_recError > 0);
}

void AudioDeviceLinuxALSA::ClearPlayoutWarning()
{
    _playWarning = 0;
}

void AudioDeviceLinuxALSA::ClearPlayoutError()
{
    _playError = 0;
}

void AudioDeviceLinuxALSA::ClearRecordingWarning()
{
    _recWarning = 0;
}

void AudioDeviceLinuxALSA::ClearRecordingError()
{
    _recError = 0;
}





int32_t AudioDeviceLinuxALSA::GetDevicesInfo(
    const int32_t function,
    const bool playback,
    const int32_t enumDeviceNo,
    char* enumDeviceName,
    const int32_t ednLen,
    char* enumDeviceId,
    const int32_t ediLen) const
{
    
    
    

    const char *type = playback ? "Output" : "Input";
    
    
    const char *ignorePrefix = playback ? "dsnoop:" : "dmix:" ;
    
    
    

    int err;
    int enumCount(0);
    bool keepSearching(true);

    
    
    
    
    int card = -1;
    while (!(LATE(snd_card_next)(&card)) && (card >= 0) && keepSearching) {
        void **hints;
        err = LATE(snd_device_name_hint)(card, "pcm", &hints);
        if (err != 0)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "GetDevicesInfo - device name hint error: %s",
                         LATE(snd_strerror)(err));
            return -1;
        }

        enumCount++; 
        if ((function == FUNC_GET_DEVICE_NAME ||
            function == FUNC_GET_DEVICE_NAME_FOR_AN_ENUM) && enumDeviceNo == 0)
        {
            strcpy(enumDeviceName, "default");
            if (enumDeviceId)
                memset(enumDeviceId, 0, ediLen);

            err = LATE(snd_device_name_free_hint)(hints);
            if (err != 0)
            {
                WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                             "GetDevicesInfo - device name free hint error: %s",
                             LATE(snd_strerror)(err));
            }

            return 0;
        }

        for (void **list = hints; *list != NULL; ++list)
        {
            char *actualType = LATE(snd_device_name_get_hint)(*list, "IOID");
            if (actualType)
            {   
                bool wrongType = (strcmp(actualType, type) != 0);
                free(actualType);
                if (wrongType)
                {
                    
                    continue;
                }
            }

            char *name = LATE(snd_device_name_get_hint)(*list, "NAME");
            if (!name)
            {
                WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                             "Device has no name");
                
                continue;
            }

            
            if (strcmp(name, "default") != 0 &&
                strcmp(name, "null") != 0 &&
                strcmp(name, "pulse") != 0 &&
                strncmp(name, ignorePrefix, strlen(ignorePrefix)) != 0)
            {
                
                char *desc = LATE(snd_device_name_get_hint)(*list, "DESC");
                if (!desc)
                {
                    
                    
                    desc = name;
                }

                if (FUNC_GET_NUM_OF_DEVICE == function)
                {
                    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                                 "    Enum device %d - %s", enumCount, name);

                }
                if ((FUNC_GET_DEVICE_NAME == function) &&
                    (enumDeviceNo == enumCount))
                {
                    
                    strncpy(enumDeviceName, desc, ednLen);
                    enumDeviceName[ednLen-1] = '\0';
                    if (enumDeviceId)
                    {
                        strncpy(enumDeviceId, name, ediLen);
                        enumDeviceId[ediLen-1] = '\0';
                    }
                    keepSearching = false;
                    
                    char * pret = strchr(enumDeviceName, '\n'); 
                    if (pret)
                        *pret = '-';
                }
                if ((FUNC_GET_DEVICE_NAME_FOR_AN_ENUM == function) &&
                    (enumDeviceNo == enumCount))
                {
                    
                    strncpy(enumDeviceName, name, ednLen);
                    enumDeviceName[ednLen-1] = '\0';
                    if (enumDeviceId)
                    {
                        strncpy(enumDeviceId, name, ediLen);
                        enumDeviceId[ediLen-1] = '\0';
                    }
                    keepSearching = false;
                }

                if (keepSearching)
                    ++enumCount;

                if (desc != name)
                    free(desc);
            }

            free(name);

            if (!keepSearching)
                break;
        }

        err = LATE(snd_device_name_free_hint)(hints);
        if (err != 0)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "GetDevicesInfo - device name free hint error: %s",
                         LATE(snd_strerror)(err));
            
        }
      }

    if (FUNC_GET_NUM_OF_DEVICE == function)
    {
        if (enumCount == 1) 
            enumCount = 0;
        return enumCount; 
    }

    if (keepSearching)
    {
        
        
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "GetDevicesInfo - Could not find device name or numbers");
        return -1;
    }

    return 0;
}

int32_t AudioDeviceLinuxALSA::InputSanityCheckAfterUnlockedPeriod() const
{
    if (_handleRecord == NULL)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  input state has been modified during unlocked period");
        return -1;
    }
    return 0;
}

int32_t AudioDeviceLinuxALSA::OutputSanityCheckAfterUnlockedPeriod() const
{
    if (_handlePlayout == NULL)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  output state has been modified during unlocked period");
        return -1;
    }
    return 0;
}

int32_t AudioDeviceLinuxALSA::ErrorRecovery(int32_t error,
                                            snd_pcm_t* deviceHandle)
{
    int st = LATE(snd_pcm_state)(deviceHandle);
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
               "Trying to recover from error: %s (%d) (state %d)",
               (LATE(snd_pcm_stream)(deviceHandle) == SND_PCM_STREAM_CAPTURE) ?
                   "capture" : "playout", LATE(snd_strerror)(error), error, st);

    
    
    
    
    
    

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    

    int res = LATE(snd_pcm_recover)(deviceHandle, error, 1);
    if (0 == res)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                   "    Recovery - snd_pcm_recover OK");

        if ((error == -EPIPE || error == -ESTRPIPE) && 
            _recording &&
            LATE(snd_pcm_stream)(deviceHandle) == SND_PCM_STREAM_CAPTURE)
        {
            
            
            int err = LATE(snd_pcm_start)(deviceHandle);
            if (err != 0)
            {
                WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                             "  Recovery - snd_pcm_start error: %u", err);
                return -1;
            }
        }

        if ((error == -EPIPE || error == -ESTRPIPE) &&  
            _playing &&
            LATE(snd_pcm_stream)(deviceHandle) == SND_PCM_STREAM_PLAYBACK)
        {
            
            
            int err = LATE(snd_pcm_start)(deviceHandle);
            if (err != 0)
            {
              WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                       "    Recovery - snd_pcm_start error: %s",
                       LATE(snd_strerror)(err));
              return -1;
            }
        }

        return -EPIPE == error ? 1 : 0;
    }
    else {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Unrecoverable alsa stream error: %d", res);
    }

    return res;
}





bool AudioDeviceLinuxALSA::PlayThreadFunc(void* pThis)
{
    return (static_cast<AudioDeviceLinuxALSA*>(pThis)->PlayThreadProcess());
}

bool AudioDeviceLinuxALSA::RecThreadFunc(void* pThis)
{
    return (static_cast<AudioDeviceLinuxALSA*>(pThis)->RecThreadProcess());
}

bool AudioDeviceLinuxALSA::PlayThreadProcess()
{
    if(!_playing)
        return false;

    int err;
    snd_pcm_sframes_t frames;
    snd_pcm_sframes_t avail_frames;

    Lock();
    
    avail_frames = LATE(snd_pcm_avail_update)(_handlePlayout);
    if (avail_frames < 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                   "playout snd_pcm_avail_update error: %s",
                   LATE(snd_strerror)(avail_frames));
        ErrorRecovery(avail_frames, _handlePlayout);
        UnLock();
        return true;
    }
    else if (avail_frames == 0)
    {
        UnLock();

        
        err = LATE(snd_pcm_wait)(_handlePlayout, 2);
        if (err == 0)
        { 
            WEBRTC_TRACE(kTraceStream, kTraceAudioDevice, _id,
                         "playout snd_pcm_wait timeout");
        }

        return true;
    }

    if (_playoutFramesLeft <= 0)
    {
        UnLock();
        _ptrAudioBuffer->RequestPlayoutData(_playoutFramesIn10MS);
        Lock();

        _playoutFramesLeft = _ptrAudioBuffer->GetPlayoutData(_playoutBuffer);
        assert(_playoutFramesLeft == _playoutFramesIn10MS);
    }

    if (static_cast<uint32_t>(avail_frames) > _playoutFramesLeft)
        avail_frames = _playoutFramesLeft;

    int size = LATE(snd_pcm_frames_to_bytes)(_handlePlayout,
        _playoutFramesLeft);
    frames = LATE(snd_pcm_writei)(
        _handlePlayout,
        &_playoutBuffer[_playoutBufferSizeIn10MS - size],
        avail_frames);

    if (frames < 0)
    {
        WEBRTC_TRACE(kTraceStream, kTraceAudioDevice, _id,
                     "playout snd_pcm_writei error: %s",
                     LATE(snd_strerror)(frames));
        _playoutFramesLeft = 0;
        ErrorRecovery(frames, _handlePlayout);
        UnLock();
        return true;
    }
    else {
        assert(frames==avail_frames);
        _playoutFramesLeft -= frames;
    }

    UnLock();
    return true;
}

bool AudioDeviceLinuxALSA::RecThreadProcess()
{
    if (!_recording)
        return false;

    int err;
    snd_pcm_sframes_t frames;
    snd_pcm_sframes_t avail_frames;
    int8_t buffer[_recordingBufferSizeIn10MS];

    Lock();

    
    avail_frames = LATE(snd_pcm_avail_update)(_handleRecord);
    if (avail_frames < 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "capture snd_pcm_avail_update error: %s",
                     LATE(snd_strerror)(avail_frames));
        ErrorRecovery(avail_frames, _handleRecord);
        UnLock();
        return true;
    }
    else if (avail_frames == 0)
    { 
        UnLock();

        
        err = LATE(snd_pcm_wait)(_handleRecord,
            ALSA_CAPTURE_WAIT_TIMEOUT);
        if (err == 0) 
            WEBRTC_TRACE(kTraceStream, kTraceAudioDevice, _id,
                         "capture snd_pcm_wait timeout");

        return true;
    }

    if (static_cast<uint32_t>(avail_frames) > _recordingFramesLeft)
        avail_frames = _recordingFramesLeft;

    frames = LATE(snd_pcm_readi)(_handleRecord,
        buffer, avail_frames); 
    if (frames < 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "capture snd_pcm_readi error: %s",
                     LATE(snd_strerror)(frames));
        ErrorRecovery(frames, _handleRecord);
        UnLock();
        return true;
    }
    else if (frames > 0)
    {
        assert(frames == avail_frames);

        int left_size = LATE(snd_pcm_frames_to_bytes)(_handleRecord,
            _recordingFramesLeft);
        int size = LATE(snd_pcm_frames_to_bytes)(_handleRecord, frames);

        memcpy(&_recordingBuffer[_recordingBufferSizeIn10MS - left_size],
               buffer, size);
        _recordingFramesLeft -= frames;

        if (!_recordingFramesLeft)
        { 
            _recordingFramesLeft = _recordingFramesIn10MS;

            if (_firstRecord) {
              LOG_FIRST_CAPTURE(this);
              _firstRecord = false;
            }
            LOG_CAPTURE_FRAMES(this, _recordingFramesIn10MS);
            
            
            _ptrAudioBuffer->SetRecordedBuffer(_recordingBuffer,
                                               _recordingFramesIn10MS);

            uint32_t currentMicLevel = 0;
            uint32_t newMicLevel = 0;

            if (AGC())
            {
                
                if (MicrophoneVolume(currentMicLevel) == 0)
                {
                    if (currentMicLevel == 0xffffffff)
                        currentMicLevel = 100;
                    
                    _ptrAudioBuffer->SetCurrentMicLevel(currentMicLevel);
                }
            }

            
            _playoutDelay = 0;
            _recordingDelay = 0;
            if (_handlePlayout)
            {
                err = LATE(snd_pcm_delay)(_handlePlayout,
                    &_playoutDelay); 
                if (err < 0)
                {
                    
                    _playoutDelay = 0;
                    WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                                 "playout snd_pcm_delay: %s",
                                 LATE(snd_strerror)(err));
                }
            }

            err = LATE(snd_pcm_delay)(_handleRecord,
                &_recordingDelay); 
            if (err < 0)
            {
                
                _recordingDelay = 0;
                WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                             "capture snd_pcm_delay: %s",
                             LATE(snd_strerror)(err));
            }

           
            _ptrAudioBuffer->SetVQEData(
                _playoutDelay * 1000 / _playoutFreq,
                _recordingDelay * 1000 / _recordingFreq, 0);

            _ptrAudioBuffer->SetTypingStatus(KeyPressed());

            
            
            UnLock();
            _ptrAudioBuffer->DeliverRecordedData();
            Lock();

            if (AGC())
            {
                newMicLevel = _ptrAudioBuffer->NewMicLevel();
                if (newMicLevel != 0)
                {
                    
                    
                    
                    if (SetMicrophoneVolume(newMicLevel) == -1)
                        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                                     "  the required modification of the "
                                     "microphone volume failed");
                }
            }
        }
    }

    UnLock();
    return true;
}


bool AudioDeviceLinuxALSA::KeyPressed() const{
#ifdef USE_X11
  char szKey[32];
  unsigned int i = 0;
  char state = 0;

  if (!_XDisplay)
    return false;

  
  XQueryKeymap(_XDisplay, szKey);

  
  for (i = 0; i < sizeof(szKey); i++)
    state |= (szKey[i] ^ _oldKeyState[i]) & szKey[i];

  
  memcpy((char*)_oldKeyState, (char*)szKey, sizeof(_oldKeyState));
  return (state != 0);
#else
  return false;
#endif
}
}  











#include <cassert>

#include "audio_device_utility.h"
#include "audio_device_pulse_linux.h"
#include "audio_device_config.h"

#include "event_wrapper.h"
#include "trace.h"
#include "thread_wrapper.h"

webrtc_adm_linux_pulse::PulseAudioSymbolTable PaSymbolTable;




#define LATE(sym) \
  LATESYM_GET(webrtc_adm_linux_pulse::PulseAudioSymbolTable, &PaSymbolTable, sym)

namespace webrtc
{





bool AudioDeviceLinuxPulse::PulseAudioIsSupported()
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, -1, "%s",
                 __FUNCTION__);

    bool pulseAudioIsSupported(true);

    
    AudioDeviceLinuxPulse* admPulse = new AudioDeviceLinuxPulse(-1);
    if (admPulse->InitPulseAudio() == -1)
    {
        pulseAudioIsSupported = false;
    }
    admPulse->TerminatePulseAudio();
    delete admPulse;

    if (pulseAudioIsSupported)
    {
        WEBRTC_TRACE(kTraceStateInfo, kTraceAudioDevice, -1,
                     "*** Linux Pulse Audio is supported ***");
    } else
    {
        WEBRTC_TRACE(kTraceStateInfo, kTraceAudioDevice, -1,
                     "*** Linux Pulse Audio is NOT supported => will revert to the ALSA API ***");
    }

    return (pulseAudioIsSupported);
}

AudioDeviceLinuxPulse::AudioDeviceLinuxPulse(const WebRtc_Word32 id) :
    _ptrAudioBuffer(NULL),
    _critSect(*CriticalSectionWrapper::CreateCriticalSection()),
    _timeEventRec(*EventWrapper::Create()),
    _timeEventPlay(*EventWrapper::Create()),
    _recStartEvent(*EventWrapper::Create()),
    _playStartEvent(*EventWrapper::Create()),
    _ptrThreadPlay(NULL),
    _ptrThreadRec(NULL),
    _recThreadID(0),
    _playThreadID(0),
    _id(id),
    _mixerManager(id),
    _inputDeviceIndex(0),
    _outputDeviceIndex(0),
    _inputDeviceIsSpecified(false),
    _outputDeviceIsSpecified(false),
    _samplingFreq(0),
    _recChannels(1),
    _playChannels(1),
    _playBufType(AudioDeviceModule::kFixedBufferSize),
    _initialized(false),
    _recording(false),
    _playing(false),
    _recIsInitialized(false),
    _playIsInitialized(false),
    _startRec(false),
    _stopRec(false),
    _startPlay(false),
    _stopPlay(false),
    _AGC(false),
    update_speaker_volume_at_startup_(false),
    _playBufDelayFixed(20),
    _sndCardPlayDelay(0),
    _sndCardRecDelay(0),
    _writeErrors(0),
    _playWarning(0),
    _playError(0),
    _recWarning(0),
    _recError(0),
    _deviceIndex(-1),
    _numPlayDevices(0),
    _numRecDevices(0),
    _playDeviceName(NULL),
    _recDeviceName(NULL),
    _playDisplayDeviceName(NULL),
    _recDisplayDeviceName(NULL),
    _playBuffer(NULL),
    _playbackBufferSize(0),
    _playbackBufferUnused(0),
    _tempBufferSpace(0),
    _recBuffer(NULL),
    _recordBufferSize(0),
    _recordBufferUsed(0),
    _tempSampleData(NULL),
    _tempSampleDataSize(0),
    _configuredLatencyPlay(0),
    _configuredLatencyRec(0),
    _paDeviceIndex(-1),
    _paStateChanged(false),
    _paMainloop(NULL),
    _paMainloopApi(NULL),
    _paContext(NULL),
    _recStream(NULL),
    _playStream(NULL),
    _recStreamFlags(0),
    _playStreamFlags(0)
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, id,
                 "%s created", __FUNCTION__);

    memset(_paServerVersion, 0, sizeof(_paServerVersion));
    memset(&_playBufferAttr, 0, sizeof(_playBufferAttr));
    memset(&_recBufferAttr, 0, sizeof(_recBufferAttr));
}

AudioDeviceLinuxPulse::~AudioDeviceLinuxPulse()
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id,
                 "%s destroyed", __FUNCTION__);

    Terminate();

    if (_recBuffer)
    {
        delete [] _recBuffer;
        _recBuffer = NULL;
    }
    if (_playBuffer)
    {
        delete [] _playBuffer;
        _playBuffer = NULL;
    }
    if (_playDeviceName)
    {
        delete [] _playDeviceName;
        _playDeviceName = NULL;
    }
    if (_recDeviceName)
    {
        delete [] _recDeviceName;
        _recDeviceName = NULL;
    }

    delete &_recStartEvent;
    delete &_playStartEvent;
    delete &_timeEventRec;
    delete &_timeEventPlay;
    delete &_critSect;
}

void AudioDeviceLinuxPulse::AttachAudioBuffer(AudioDeviceBuffer* audioBuffer)
{

    CriticalSectionScoped lock(&_critSect);

    _ptrAudioBuffer = audioBuffer;

    
    
    
    _ptrAudioBuffer->SetRecordingSampleRate(0);
    _ptrAudioBuffer->SetPlayoutSampleRate(0);
    _ptrAudioBuffer->SetRecordingChannels(0);
    _ptrAudioBuffer->SetPlayoutChannels(0);
}





WebRtc_Word32 AudioDeviceLinuxPulse::ActiveAudioLayer(
    AudioDeviceModule::AudioLayer& audioLayer) const
{
    audioLayer = AudioDeviceModule::kLinuxPulseAudio;
    return 0;
}

WebRtc_Word32 AudioDeviceLinuxPulse::Init()
{

    CriticalSectionScoped lock(&_critSect);

    if (_initialized)
    {
        return 0;
    }

    
    if (InitPulseAudio() < 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  failed to initialize PulseAudio");

        if (TerminatePulseAudio() < 0)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  failed to terminate PulseAudio");
        }

        return -1;
    }

    _playWarning = 0;
    _playError = 0;
    _recWarning = 0;
    _recError = 0;

    
    const char* threadName = "webrtc_audio_module_rec_thread";
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

    
    threadName = "webrtc_audio_module_play_thread";
    _ptrThreadPlay = ThreadWrapper::CreateThread(PlayThreadFunc, this,
                                                 kRealtimePriority, threadName);
    if (_ptrThreadPlay == NULL)
    {
        WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                     "  failed to create the play audio thread");
        return -1;
    }

    threadID = 0;
    if (!_ptrThreadPlay->Start(threadID))
    {
        WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                     "  failed to start the play audio thread");

        delete _ptrThreadPlay;
        _ptrThreadPlay = NULL;
        return -1;
    }
    _playThreadID = threadID;

    _initialized = true;

    return 0;
}

WebRtc_Word32 AudioDeviceLinuxPulse::Terminate()
{

    if (!_initialized)
    {
        return 0;
    }

    Lock();

    _mixerManager.Close();

    
    if (_ptrThreadRec)
    {
        ThreadWrapper* tmpThread = _ptrThreadRec;
        _ptrThreadRec = NULL;
        UnLock();

        tmpThread->SetNotAlive();
        _timeEventRec.Set();
        if (tmpThread->Stop())
        {
            delete tmpThread;
        } else
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "  failed to close down the rec audio thread");
        }
        
        Lock();
    }

    
    if (_ptrThreadPlay)
    {
        ThreadWrapper* tmpThread = _ptrThreadPlay;
        _ptrThreadPlay = NULL;
        _critSect.Leave();

        tmpThread->SetNotAlive();
        _timeEventPlay.Set();
        if (tmpThread->Stop())
        {
            delete tmpThread;
        } else
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "  failed to close down the play audio thread");
        }
    } else {
      UnLock();
    }

    
    if (TerminatePulseAudio() < 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  failed to terminate PulseAudio");
        return -1;
    }

    _initialized = false;
    _outputDeviceIsSpecified = false;
    _inputDeviceIsSpecified = false;

    return 0;
}

bool AudioDeviceLinuxPulse::Initialized() const
{
    return (_initialized);
}

WebRtc_Word32 AudioDeviceLinuxPulse::SpeakerIsAvailable(bool& available)
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

WebRtc_Word32 AudioDeviceLinuxPulse::InitSpeaker()
{

    CriticalSectionScoped lock(&_critSect);

    if (_playing)
    {
        return -1;
    }

    if (!_outputDeviceIsSpecified)
    {
        return -1;
    }

    
    if (_outputDeviceIndex == 0)
    {
        WebRtc_UWord16 deviceIndex = 0;
        GetDefaultDeviceInfo(false, NULL, deviceIndex);
        _paDeviceIndex = deviceIndex;
    } else
    {
        
        
        _deviceIndex = _outputDeviceIndex;

        
        PlayoutDevices();
    }

    
    
    if (_mixerManager.OpenSpeaker(_paDeviceIndex) == -1)
    {
        return -1;
    }

    
    _deviceIndex = -1;
    _paDeviceIndex = -1;

    return 0;
}

WebRtc_Word32 AudioDeviceLinuxPulse::MicrophoneIsAvailable(bool& available)
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

WebRtc_Word32 AudioDeviceLinuxPulse::InitMicrophone()
{

    CriticalSectionScoped lock(&_critSect);

    if (_recording)
    {
        return -1;
    }

    if (!_inputDeviceIsSpecified)
    {
        return -1;
    }

    
    if (_inputDeviceIndex == 0)
    {
        WebRtc_UWord16 deviceIndex = 0;
        GetDefaultDeviceInfo(true, NULL, deviceIndex);
        _paDeviceIndex = deviceIndex;
    } else
    {
        
        
        _deviceIndex = _inputDeviceIndex;

        
        RecordingDevices();
    }

    
    
    if (_mixerManager.OpenMicrophone(_paDeviceIndex) == -1)
    {
        return -1;
    }

    
    _deviceIndex = -1;
    _paDeviceIndex = -1;

    return 0;
}

bool AudioDeviceLinuxPulse::SpeakerIsInitialized() const
{
    return (_mixerManager.SpeakerIsInitialized());
}

bool AudioDeviceLinuxPulse::MicrophoneIsInitialized() const
{
    return (_mixerManager.MicrophoneIsInitialized());
}

WebRtc_Word32 AudioDeviceLinuxPulse::SpeakerVolumeIsAvailable(bool& available)
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

WebRtc_Word32 AudioDeviceLinuxPulse::SetSpeakerVolume(WebRtc_UWord32 volume)
{
    if (!_playing) {
      
      update_speaker_volume_at_startup_ = true;
    }
    return (_mixerManager.SetSpeakerVolume(volume));
}

WebRtc_Word32 AudioDeviceLinuxPulse::SpeakerVolume(WebRtc_UWord32& volume) const
{

    WebRtc_UWord32 level(0);

    if (_mixerManager.SpeakerVolume(level) == -1)
    {
        return -1;
    }

    volume = level;

    return 0;
}

WebRtc_Word32 AudioDeviceLinuxPulse::SetWaveOutVolume(
    WebRtc_UWord16 volumeLeft,
    WebRtc_UWord16 volumeRight)
{

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

WebRtc_Word32 AudioDeviceLinuxPulse::WaveOutVolume(
    WebRtc_UWord16& ,
    WebRtc_UWord16& ) const
{

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

WebRtc_Word32 AudioDeviceLinuxPulse::MaxSpeakerVolume(
    WebRtc_UWord32& maxVolume) const
{

    WebRtc_UWord32 maxVol(0);

    if (_mixerManager.MaxSpeakerVolume(maxVol) == -1)
    {
        return -1;
    }

    maxVolume = maxVol;

    return 0;
}

WebRtc_Word32 AudioDeviceLinuxPulse::MinSpeakerVolume(
    WebRtc_UWord32& minVolume) const
{

    WebRtc_UWord32 minVol(0);

    if (_mixerManager.MinSpeakerVolume(minVol) == -1)
    {
        return -1;
    }

    minVolume = minVol;

    return 0;
}

WebRtc_Word32 AudioDeviceLinuxPulse::SpeakerVolumeStepSize(
    WebRtc_UWord16& stepSize) const
{

    WebRtc_UWord16 delta(0);

    if (_mixerManager.SpeakerVolumeStepSize(delta) == -1)
    {
        return -1;
    }

    stepSize = delta;

    return 0;
}

WebRtc_Word32 AudioDeviceLinuxPulse::SpeakerMuteIsAvailable(bool& available)
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

WebRtc_Word32 AudioDeviceLinuxPulse::SetSpeakerMute(bool enable)
{

    return (_mixerManager.SetSpeakerMute(enable));
}

WebRtc_Word32 AudioDeviceLinuxPulse::SpeakerMute(bool& enabled) const
{

    bool muted(0);
    if (_mixerManager.SpeakerMute(muted) == -1)
    {
        return -1;
    }

    enabled = muted;
    return 0;
}

WebRtc_Word32 AudioDeviceLinuxPulse::MicrophoneMuteIsAvailable(bool& available)
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

WebRtc_Word32 AudioDeviceLinuxPulse::SetMicrophoneMute(bool enable)
{

    return (_mixerManager.SetMicrophoneMute(enable));
}

WebRtc_Word32 AudioDeviceLinuxPulse::MicrophoneMute(bool& enabled) const
{

    bool muted(0);
    if (_mixerManager.MicrophoneMute(muted) == -1)
    {
        return -1;
    }

    enabled = muted;
    return 0;
}

WebRtc_Word32 AudioDeviceLinuxPulse::MicrophoneBoostIsAvailable(bool& available)
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

WebRtc_Word32 AudioDeviceLinuxPulse::SetMicrophoneBoost(bool enable)
{

    return (_mixerManager.SetMicrophoneBoost(enable));
}

WebRtc_Word32 AudioDeviceLinuxPulse::MicrophoneBoost(bool& enabled) const
{

    bool onOff(0);

    if (_mixerManager.MicrophoneBoost(onOff) == -1)
    {
        return -1;
    }

    enabled = onOff;

    return 0;
}

WebRtc_Word32 AudioDeviceLinuxPulse::StereoRecordingIsAvailable(bool& available)
{

    if (_recChannels == 2 && _recording) {
      available = true;
      return 0;
    }

    available = false;
    bool wasInitialized = _mixerManager.MicrophoneIsInitialized();
    int error = 0;

    if (!wasInitialized && InitMicrophone() == -1)
    {
        
        available = false;
        return 0;
    }

#ifndef WEBRTC_PA_GTALK
    
    bool isAvailable(false);
    error = _mixerManager.StereoRecordingIsAvailable(isAvailable);
    if (!error)
      available = isAvailable;
#endif

    
    if (!wasInitialized)
    {
        _mixerManager.CloseMicrophone();
    }

    return error;
}

WebRtc_Word32 AudioDeviceLinuxPulse::SetStereoRecording(bool enable)
{

#ifndef WEBRTC_PA_GTALK
    if (enable)
        _recChannels = 2;
    else
        _recChannels = 1;
#endif

    return 0;
}

WebRtc_Word32 AudioDeviceLinuxPulse::StereoRecording(bool& enabled) const
{

    if (_recChannels == 2)
        enabled = true;
    else
        enabled = false;

    return 0;
}

WebRtc_Word32 AudioDeviceLinuxPulse::StereoPlayoutIsAvailable(bool& available)
{

    if (_playChannels == 2 && _playing) {
      available = true;
      return 0;
    }

    available = false;
    bool wasInitialized = _mixerManager.SpeakerIsInitialized();
    int error = 0;

    if (!wasInitialized && InitSpeaker() == -1)
    {
        
        return -1;
    }

#ifndef WEBRTC_PA_GTALK
    
    bool isAvailable(false);
    error = _mixerManager.StereoPlayoutIsAvailable(isAvailable);
    if (!error)
      available = isAvailable;
#endif

    
    if (!wasInitialized)
    {
        _mixerManager.CloseSpeaker();
    }

    return error;
}

WebRtc_Word32 AudioDeviceLinuxPulse::SetStereoPlayout(bool enable)
{

#ifndef WEBRTC_PA_GTALK
    if (enable)
        _playChannels = 2;
    else
        _playChannels = 1;
#endif

    return 0;
}

WebRtc_Word32 AudioDeviceLinuxPulse::StereoPlayout(bool& enabled) const
{

    if (_playChannels == 2)
        enabled = true;
    else
        enabled = false;

    return 0;
}

WebRtc_Word32 AudioDeviceLinuxPulse::SetAGC(bool enable)
{

    _AGC = enable;

    return 0;
}

bool AudioDeviceLinuxPulse::AGC() const
{

    return _AGC;
}

WebRtc_Word32 AudioDeviceLinuxPulse::MicrophoneVolumeIsAvailable(
    bool& available)
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

WebRtc_Word32 AudioDeviceLinuxPulse::SetMicrophoneVolume(WebRtc_UWord32 volume)
{

    return (_mixerManager.SetMicrophoneVolume(volume));
}

WebRtc_Word32 AudioDeviceLinuxPulse::MicrophoneVolume(
    WebRtc_UWord32& volume) const
{

    WebRtc_UWord32 level(0);

    if (_mixerManager.MicrophoneVolume(level) == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  failed to retrive current microphone level");
        return -1;
    }

    volume = level;

    return 0;
}

WebRtc_Word32 AudioDeviceLinuxPulse::MaxMicrophoneVolume(
    WebRtc_UWord32& maxVolume) const
{

    WebRtc_UWord32 maxVol(0);

    if (_mixerManager.MaxMicrophoneVolume(maxVol) == -1)
    {
        return -1;
    }

    maxVolume = maxVol;

    return 0;
}

WebRtc_Word32 AudioDeviceLinuxPulse::MinMicrophoneVolume(
    WebRtc_UWord32& minVolume) const
{

    WebRtc_UWord32 minVol(0);

    if (_mixerManager.MinMicrophoneVolume(minVol) == -1)
    {
        return -1;
    }

    minVolume = minVol;

    return 0;
}

WebRtc_Word32 AudioDeviceLinuxPulse::MicrophoneVolumeStepSize(
    WebRtc_UWord16& stepSize) const
{

    WebRtc_UWord16 delta(0);

    if (_mixerManager.MicrophoneVolumeStepSize(delta) == -1)
    {
        return -1;
    }

    stepSize = delta;

    return 0;
}

WebRtc_Word16 AudioDeviceLinuxPulse::PlayoutDevices()
{

    PaLock();

    pa_operation* paOperation = NULL;
    _numPlayDevices = 1; 

    
    paOperation = LATE(pa_context_get_sink_info_list)(_paContext,
                                                      PaSinkInfoCallback,
                                                      this);

    WaitForOperationCompletion(paOperation);

    PaUnLock();

    return _numPlayDevices;
}

WebRtc_Word32 AudioDeviceLinuxPulse::SetPlayoutDevice(WebRtc_UWord16 index)
{

    if (_playIsInitialized)
    {
        return -1;
    }

    const WebRtc_UWord16 nDevices = PlayoutDevices();

    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "  number of availiable output devices is %u", nDevices);

    if (index > (nDevices - 1))
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  device index is out of range [0,%u]", (nDevices - 1));
        return -1;
    }

    _outputDeviceIndex = index;
    _outputDeviceIsSpecified = true;

    return 0;
}

WebRtc_Word32 AudioDeviceLinuxPulse::SetPlayoutDevice(
    AudioDeviceModule::WindowsDeviceType )
{
    WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                 "WindowsDeviceType not supported");
    return -1;
}

WebRtc_Word32 AudioDeviceLinuxPulse::PlayoutDeviceName(
    WebRtc_UWord16 index,
    char name[kAdmMaxDeviceNameSize],
    char guid[kAdmMaxGuidSize])
{

    const WebRtc_UWord16 nDevices = PlayoutDevices();

    if ((index > (nDevices - 1)) || (name == NULL))
    {
        return -1;
    }

    memset(name, 0, kAdmMaxDeviceNameSize);

    if (guid != NULL)
    {
        memset(guid, 0, kAdmMaxGuidSize);
    }

    
    if (index == 0)
    {
        WebRtc_UWord16 deviceIndex = 0;
        return GetDefaultDeviceInfo(false, name, deviceIndex);
    }

    
    
    _playDisplayDeviceName = name;
    _deviceIndex = index;

    
    PlayoutDevices();

    
    _playDisplayDeviceName = NULL;
    _deviceIndex = -1;

    return 0;
}

WebRtc_Word32 AudioDeviceLinuxPulse::RecordingDeviceName(
    WebRtc_UWord16 index,
    char name[kAdmMaxDeviceNameSize],
    char guid[kAdmMaxGuidSize])
{

    const WebRtc_UWord16 nDevices(RecordingDevices());

    if ((index > (nDevices - 1)) || (name == NULL))
    {
        return -1;
    }

    memset(name, 0, kAdmMaxDeviceNameSize);

    if (guid != NULL)
    {
        memset(guid, 0, kAdmMaxGuidSize);
    }

    
    if (index == 0)
    {
        WebRtc_UWord16 deviceIndex = 0;
        return GetDefaultDeviceInfo(true, name, deviceIndex);
    }

    
    
    _recDisplayDeviceName = name;
    _deviceIndex = index;

    
    RecordingDevices();

    
    _recDisplayDeviceName = NULL;
    _deviceIndex = -1;

    return 0;
}

WebRtc_Word16 AudioDeviceLinuxPulse::RecordingDevices()
{

    PaLock();

    pa_operation* paOperation = NULL;
    _numRecDevices = 1; 

    
    paOperation = LATE(pa_context_get_source_info_list)(_paContext,
                                                        PaSourceInfoCallback,
                                                        this);

    WaitForOperationCompletion(paOperation);

    PaUnLock();

    return _numRecDevices;
}

WebRtc_Word32 AudioDeviceLinuxPulse::SetRecordingDevice(WebRtc_UWord16 index)
{

    if (_recIsInitialized)
    {
        return -1;
    }

    const WebRtc_UWord16 nDevices(RecordingDevices());

    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "  number of availiable input devices is %u", nDevices);

    if (index > (nDevices - 1))
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  device index is out of range [0,%u]", (nDevices - 1));
        return -1;
    }

    _inputDeviceIndex = index;
    _inputDeviceIsSpecified = true;

    return 0;
}

WebRtc_Word32 AudioDeviceLinuxPulse::SetRecordingDevice(
    AudioDeviceModule::WindowsDeviceType )
{
    WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                 "WindowsDeviceType not supported");
    return -1;
}

WebRtc_Word32 AudioDeviceLinuxPulse::PlayoutIsAvailable(bool& available)
{

    available = false;

    
    WebRtc_Word32 res = InitPlayout();

    
    StopPlayout();

    if (res != -1)
    {
        available = true;
    }

    return res;
}

WebRtc_Word32 AudioDeviceLinuxPulse::RecordingIsAvailable(bool& available)
{

    available = false;

    
    WebRtc_Word32 res = InitRecording();

    
    StopRecording();

    if (res != -1)
    {
        available = true;
    }

    return res;
}

WebRtc_Word32 AudioDeviceLinuxPulse::InitPlayout()
{

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

    
    WebRtc_UWord32 samplingRate = _samplingFreq * 1000;
    if (samplingRate == 44000)
    {
        samplingRate = 44100;
    }

    
    pa_sample_spec playSampleSpec;
    playSampleSpec.channels = _playChannels;
    playSampleSpec.format = PA_SAMPLE_S16LE;
    playSampleSpec.rate = samplingRate;

    
    _playStream = LATE(pa_stream_new)(_paContext, "playStream",
                                      &playSampleSpec, NULL);

    if (!_playStream)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  failed to create play stream, err=%d",
                     LATE(pa_context_errno)(_paContext));
        return -1;
    }

    
    _mixerManager.SetPlayStream(_playStream);

    if (_ptrAudioBuffer)
    {
        
        _ptrAudioBuffer->SetPlayoutSampleRate(_samplingFreq * 1000);
        _ptrAudioBuffer->SetPlayoutChannels((WebRtc_UWord8) _playChannels);
    }

    WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                 "  stream state %d\n", LATE(pa_stream_get_state)(_playStream));

    
    _playStreamFlags = (pa_stream_flags_t) (PA_STREAM_AUTO_TIMING_UPDATE
        | PA_STREAM_INTERPOLATE_TIMING);

    if (_configuredLatencyPlay != WEBRTC_PA_NO_LATENCY_REQUIREMENTS)
    {
        
        
        
        
        
        if (LATE(pa_context_get_protocol_version)(_paContext)
            >= WEBRTC_PA_ADJUST_LATENCY_PROTOCOL_VERSION)
        {
            _playStreamFlags |= PA_STREAM_ADJUST_LATENCY;
        }

        const pa_sample_spec *spec =
            LATE(pa_stream_get_sample_spec)(_playStream);
        if (!spec)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  pa_stream_get_sample_spec()");
            return -1;
        }

        size_t bytesPerSec = LATE(pa_bytes_per_second)(spec);
        WebRtc_UWord32 latency = bytesPerSec
            * WEBRTC_PA_PLAYBACK_LATENCY_MINIMUM_MSECS / WEBRTC_PA_MSECS_PER_SEC;

        
        _playBufferAttr.maxlength = latency; 
        _playBufferAttr.tlength = latency; 
        
        _playBufferAttr.minreq = latency / WEBRTC_PA_PLAYBACK_REQUEST_FACTOR;
        _playBufferAttr.prebuf = _playBufferAttr.tlength
            - _playBufferAttr.minreq; 

        _configuredLatencyPlay = latency;
    }

    
    _playbackBufferSize = _samplingFreq * 10 * 2 * _playChannels;
    _playbackBufferUnused = _playbackBufferSize;
    _playBuffer = new WebRtc_Word8[_playbackBufferSize];

    
    LATE(pa_stream_set_underflow_callback)(_playStream,
                                           PaStreamUnderflowCallback, this);

    
    LATE(pa_stream_set_state_callback)(_playStream, PaStreamStateCallback, this);

    
    _playIsInitialized = true;
    _sndCardPlayDelay = 0;
    _sndCardRecDelay = 0;

    return 0;
}

WebRtc_Word32 AudioDeviceLinuxPulse::InitRecording()
{

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

    
    WebRtc_UWord32 samplingRate = _samplingFreq * 1000;
    if (samplingRate == 44000)
    {
        samplingRate = 44100;
    }

    
    pa_sample_spec recSampleSpec;
    recSampleSpec.channels = _recChannels;
    recSampleSpec.format = PA_SAMPLE_S16LE;
    recSampleSpec.rate = samplingRate;

    
    _recStream = LATE(pa_stream_new)(_paContext, "recStream", &recSampleSpec,
                                     NULL);
    if (!_recStream)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  failed to create rec stream, err=%d",
                     LATE(pa_context_errno)(_paContext));
        return -1;
    }

    
    _mixerManager.SetRecStream(_recStream);

    if (_ptrAudioBuffer)
    {
        
        _ptrAudioBuffer->SetRecordingSampleRate(_samplingFreq * 1000);
        _ptrAudioBuffer->SetRecordingChannels((WebRtc_UWord8) _recChannels);
    }

    if (_configuredLatencyRec != WEBRTC_PA_NO_LATENCY_REQUIREMENTS)
    {
        _recStreamFlags = (pa_stream_flags_t) (PA_STREAM_AUTO_TIMING_UPDATE
            | PA_STREAM_INTERPOLATE_TIMING);

        
        
        
        
        
        if (LATE(pa_context_get_protocol_version)(_paContext)
            >= WEBRTC_PA_ADJUST_LATENCY_PROTOCOL_VERSION)
        {
            _recStreamFlags |= PA_STREAM_ADJUST_LATENCY;
        }

        const pa_sample_spec *spec =
            LATE(pa_stream_get_sample_spec)(_recStream);
        if (!spec)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  pa_stream_get_sample_spec(rec)");
            return -1;
        }

        size_t bytesPerSec = LATE(pa_bytes_per_second)(spec);
        WebRtc_UWord32 latency = bytesPerSec
            * WEBRTC_PA_LOW_CAPTURE_LATENCY_MSECS / WEBRTC_PA_MSECS_PER_SEC;

        
        
        
        _recBufferAttr.fragsize = latency; 
        _recBufferAttr.maxlength = latency + bytesPerSec
            * WEBRTC_PA_CAPTURE_BUFFER_EXTRA_MSECS / WEBRTC_PA_MSECS_PER_SEC;

        _configuredLatencyRec = latency;
    }

    _recordBufferSize = _samplingFreq * 10 * 2 * _recChannels;
    _recordBufferUsed = 0;
    _recBuffer = new WebRtc_Word8[_recordBufferSize];

    
    LATE(pa_stream_set_overflow_callback)(_recStream, PaStreamOverflowCallback,
                                          this);

    
    LATE(pa_stream_set_state_callback)(_recStream, PaStreamStateCallback, this);

    
    _recIsInitialized = true;

    return 0;
}

WebRtc_Word32 AudioDeviceLinuxPulse::StartRecording()
{

    if (!_recIsInitialized)
    {
        return -1;
    }

    if (_recording)
    {
        return 0;
    }

    
    _startRec = true;

    
    _timeEventRec.Set();
    if (kEventTimeout == _recStartEvent.Wait(10000))
    {
        _startRec = false;
        StopRecording();
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  failed to activate recording");
        return -1;
    }

    if (_recording)
    {
        
    } else
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  failed to activate recording");
        return -1;
    }

    return 0;
}

WebRtc_Word32 AudioDeviceLinuxPulse::StopRecording()
{

    CriticalSectionScoped lock(&_critSect);

    if (!_recIsInitialized)
    {
        return 0;
    }

    if (_recStream == NULL)
    {
        return -1;
    }

    _recIsInitialized = false;
    _recording = false;

    WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                 "  stopping recording");

    
    PaLock();

    DisableReadCallback();
    LATE(pa_stream_set_overflow_callback)(_recStream, NULL, NULL);

    
    LATE(pa_stream_set_state_callback)(_recStream, NULL, NULL);

    if (LATE(pa_stream_get_state)(_recStream) != PA_STREAM_UNCONNECTED)
    {
        
        if (LATE(pa_stream_disconnect)(_recStream) != PA_OK)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  failed to disconnect rec stream, err=%d\n",
                         LATE(pa_context_errno)(_paContext));
            PaUnLock();
            return -1;
        }

        WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                     "  disconnected recording");
    }

    LATE(pa_stream_unref)(_recStream);
    _recStream = NULL;

    PaUnLock();

    
    _mixerManager.SetRecStream(_recStream);

    if (_recBuffer)
    {
        delete [] _recBuffer;
        _recBuffer = NULL;
    }

    return 0;
}

bool AudioDeviceLinuxPulse::RecordingIsInitialized() const
{
    return (_recIsInitialized);
}

bool AudioDeviceLinuxPulse::Recording() const
{
    return (_recording);
}

bool AudioDeviceLinuxPulse::PlayoutIsInitialized() const
{
    return (_playIsInitialized);
}

WebRtc_Word32 AudioDeviceLinuxPulse::StartPlayout()
{

    if (!_playIsInitialized)
    {
        return -1;
    }

    if (_playing)
    {
        return 0;
    }

    
    _startPlay = true;

    
    _timeEventPlay.Set();
    if (kEventTimeout == _playStartEvent.Wait(10000))
    {
        _startPlay = false;
        StopPlayout();
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  failed to activate playout");
        return -1;
    }

    if (_playing)
    {
        
    } else
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  failed to activate playing");
        return -1;
    }

    return 0;
}

WebRtc_Word32 AudioDeviceLinuxPulse::StopPlayout()
{

    CriticalSectionScoped lock(&_critSect);

    if (!_playIsInitialized)
    {
        return 0;
    }

    if (_playStream == NULL)
    {
        return -1;
    }

    _playIsInitialized = false;
    _playing = false;
    _sndCardPlayDelay = 0;
    _sndCardRecDelay = 0;

    WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                 "  stopping playback");

    
    PaLock();

    DisableWriteCallback();
    LATE(pa_stream_set_underflow_callback)(_playStream, NULL, NULL);

    
    LATE(pa_stream_set_state_callback)(_playStream, NULL, NULL);

    if (LATE(pa_stream_get_state)(_playStream) != PA_STREAM_UNCONNECTED)
    {
        
        if (LATE(pa_stream_disconnect)(_playStream) != PA_OK)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  failed to disconnect play stream, err=%d",
                         LATE(pa_context_errno)(_paContext));
            PaUnLock();
            return -1;
        }

        WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                     "  disconnected playback");
    }

    LATE(pa_stream_unref)(_playStream);
    _playStream = NULL;

    PaUnLock();

    
    _mixerManager.SetPlayStream(_playStream);

    if (_playBuffer)
    {
        delete [] _playBuffer;
        _playBuffer = NULL;
    }

    return 0;
}

WebRtc_Word32 AudioDeviceLinuxPulse::PlayoutDelay(WebRtc_UWord16& delayMS) const
{
    delayMS = (WebRtc_UWord16) _sndCardPlayDelay;
    return 0;
}

WebRtc_Word32 AudioDeviceLinuxPulse::RecordingDelay(WebRtc_UWord16& delayMS) const
{
    delayMS = (WebRtc_UWord16) _sndCardRecDelay;
    return 0;
}

bool AudioDeviceLinuxPulse::Playing() const
{
    return (_playing);
}

WebRtc_Word32 AudioDeviceLinuxPulse::SetPlayoutBuffer(
    const AudioDeviceModule::BufferType type,
    WebRtc_UWord16 sizeMS)
{

    if (type != AudioDeviceModule::kFixedBufferSize)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     " Adaptive buffer size not supported on this platform");
        return -1;
    }

    _playBufType = type;
    _playBufDelayFixed = sizeMS;

    return 0;
}

WebRtc_Word32 AudioDeviceLinuxPulse::PlayoutBuffer(
    AudioDeviceModule::BufferType& type,
    WebRtc_UWord16& sizeMS) const
{

    type = _playBufType;
    sizeMS = _playBufDelayFixed;

    return 0;
}

WebRtc_Word32 AudioDeviceLinuxPulse::CPULoad(WebRtc_UWord16& ) const
{

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

bool AudioDeviceLinuxPulse::PlayoutWarning() const
{
  CriticalSectionScoped lock(&_critSect);
  return (_playWarning > 0);
}

bool AudioDeviceLinuxPulse::PlayoutError() const
{
  CriticalSectionScoped lock(&_critSect);
  return (_playError > 0);
}

bool AudioDeviceLinuxPulse::RecordingWarning() const
{
  CriticalSectionScoped lock(&_critSect);
  return (_recWarning > 0);
}

bool AudioDeviceLinuxPulse::RecordingError() const
{
  CriticalSectionScoped lock(&_critSect);
  return (_recError > 0);
}

void AudioDeviceLinuxPulse::ClearPlayoutWarning()
{
  CriticalSectionScoped lock(&_critSect);
  _playWarning = 0;
}

void AudioDeviceLinuxPulse::ClearPlayoutError()
{
  CriticalSectionScoped lock(&_critSect);
  _playError = 0;
}

void AudioDeviceLinuxPulse::ClearRecordingWarning()
{
  CriticalSectionScoped lock(&_critSect);
  _recWarning = 0;
}

void AudioDeviceLinuxPulse::ClearRecordingError()
{
  CriticalSectionScoped lock(&_critSect);
  _recError = 0;
}





void AudioDeviceLinuxPulse::PaContextStateCallback(pa_context *c, void *pThis)
{
    static_cast<AudioDeviceLinuxPulse*> (pThis)->PaContextStateCallbackHandler(
        c);
}





void AudioDeviceLinuxPulse::PaSinkInfoCallback(pa_context *,
                                               const pa_sink_info *i, int eol,
                                               void *pThis)
{
    static_cast<AudioDeviceLinuxPulse*> (pThis)->PaSinkInfoCallbackHandler(
        i, eol);
}

void AudioDeviceLinuxPulse::PaSourceInfoCallback(pa_context *,
                                                 const pa_source_info *i,
                                                 int eol, void *pThis)
{
    static_cast<AudioDeviceLinuxPulse*> (pThis)->PaSourceInfoCallbackHandler(
        i, eol);
}

void AudioDeviceLinuxPulse::PaServerInfoCallback(pa_context *,
                                                 const pa_server_info *i,
                                                 void *pThis)
{
    static_cast<AudioDeviceLinuxPulse*> (pThis)->PaServerInfoCallbackHandler(i);
}

void AudioDeviceLinuxPulse::PaStreamStateCallback(pa_stream *p, void *pThis)
{
    static_cast<AudioDeviceLinuxPulse*> (pThis)->PaStreamStateCallbackHandler(p);
}

void AudioDeviceLinuxPulse::PaContextStateCallbackHandler(pa_context *c)
{
    WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                 "  context state cb");

    pa_context_state_t state = LATE(pa_context_get_state)(c);
    switch (state)
    {
        case PA_CONTEXT_UNCONNECTED:
            WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                         "  unconnected");
            break;
        case PA_CONTEXT_CONNECTING:
        case PA_CONTEXT_AUTHORIZING:
        case PA_CONTEXT_SETTING_NAME:
            WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                         "  no state");
            break;
        case PA_CONTEXT_FAILED:
        case PA_CONTEXT_TERMINATED:
            WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                         "  failed");
            _paStateChanged = true;
            LATE(pa_threaded_mainloop_signal)(_paMainloop, 0);
            break;
        case PA_CONTEXT_READY:
            WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                         "  ready");
            _paStateChanged = true;
            LATE(pa_threaded_mainloop_signal)(_paMainloop, 0);
            break;
    }
}

void AudioDeviceLinuxPulse::PaSinkInfoCallbackHandler(const pa_sink_info *i,
                                                      int eol)
{
    if (eol)
    {
        
        LATE(pa_threaded_mainloop_signal)(_paMainloop, 0);
        return;
    }

    if (_numPlayDevices == _deviceIndex)
    {
        
        _paDeviceIndex = i->index;

        if (_playDeviceName)
        {
            
            strncpy(_playDeviceName, i->name, kAdmMaxDeviceNameSize);
            _playDeviceName[kAdmMaxDeviceNameSize - 1] = '\0';
        }
        if (_playDisplayDeviceName)
        {
            
            strncpy(_playDisplayDeviceName, i->description,
                    kAdmMaxDeviceNameSize);
            _playDisplayDeviceName[kAdmMaxDeviceNameSize - 1] = '\0';
        }
    }

    _numPlayDevices++;
}

void AudioDeviceLinuxPulse::PaSourceInfoCallbackHandler(
    const pa_source_info *i,
    int eol)
{
    if (eol)
    {
        
        LATE(pa_threaded_mainloop_signal)(_paMainloop, 0);
        return;
    }

    
     if (i->monitor_of_sink == PA_INVALID_INDEX)
    {
        if (_numRecDevices == _deviceIndex)
        {
            
            _paDeviceIndex = i->index;

            if (_recDeviceName)
            {
                
                strncpy(_recDeviceName, i->name, kAdmMaxDeviceNameSize);
                _recDeviceName[kAdmMaxDeviceNameSize - 1] = '\0';
            }
            if (_recDisplayDeviceName)
            {
                
                strncpy(_recDisplayDeviceName, i->description,
                        kAdmMaxDeviceNameSize);
                _recDisplayDeviceName[kAdmMaxDeviceNameSize - 1] = '\0';
            }
        }

        _numRecDevices++;
    }
}

void AudioDeviceLinuxPulse::PaServerInfoCallbackHandler(const pa_server_info *i)
{
    
    WebRtc_UWord32 paSampleRate = i->sample_spec.rate;
    if (paSampleRate == 44100)
    {
#ifdef WEBRTC_PA_GTALK
        paSampleRate = 48000;
#else
        paSampleRate = 44000;
#endif
    }

    _samplingFreq = paSampleRate / 1000;

    
    strncpy(_paServerVersion, i->server_version, 31);
    _paServerVersion[31] = '\0';

    if (_recDisplayDeviceName)
    {
        
        strncpy(_recDisplayDeviceName, i->default_source_name,
                kAdmMaxDeviceNameSize);
        _recDisplayDeviceName[kAdmMaxDeviceNameSize - 1] = '\0';
    }

    if (_playDisplayDeviceName)
    {
        
        strncpy(_playDisplayDeviceName, i->default_sink_name,
                kAdmMaxDeviceNameSize);
        _playDisplayDeviceName[kAdmMaxDeviceNameSize - 1] = '\0';
    }

    LATE(pa_threaded_mainloop_signal)(_paMainloop, 0);
}

void AudioDeviceLinuxPulse::PaStreamStateCallbackHandler(pa_stream *p)
{
    WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                 "  stream state cb");

    pa_stream_state_t state = LATE(pa_stream_get_state)(p);
    switch (state)
    {
        case PA_STREAM_UNCONNECTED:
            WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                         "  unconnected");
            break;
        case PA_STREAM_CREATING:
            WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                         "  creating");
            break;
        case PA_STREAM_FAILED:
        case PA_STREAM_TERMINATED:
            WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                         "  failed");
            break;
        case PA_STREAM_READY:
            WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                         "  ready");
            break;
    }

    LATE(pa_threaded_mainloop_signal)(_paMainloop, 0);
}

WebRtc_Word32 AudioDeviceLinuxPulse::CheckPulseAudioVersion()
{
    






    PaLock();

    pa_operation* paOperation = NULL;

    
    paOperation = LATE(pa_context_get_server_info)(_paContext,
                                                   PaServerInfoCallback, this);

    WaitForOperationCompletion(paOperation);

    PaUnLock();

    WEBRTC_TRACE(kTraceStateInfo, kTraceAudioDevice, -1,
                 "  checking PulseAudio version: %s", _paServerVersion);

    















































    return 0;
}

WebRtc_Word32 AudioDeviceLinuxPulse::InitSamplingFrequency()
{
    PaLock();

    pa_operation* paOperation = NULL;

    
    paOperation = LATE(pa_context_get_server_info)(_paContext,
                                                   PaServerInfoCallback, this);

    WaitForOperationCompletion(paOperation);

    PaUnLock();

    return 0;
}

WebRtc_Word32 AudioDeviceLinuxPulse::GetDefaultDeviceInfo(bool recDevice,
                                                          char* name,
                                                          WebRtc_UWord16& index)
{
    char tmpName[kAdmMaxDeviceNameSize] = {0};
    
    WebRtc_UWord16 nameLen = kAdmMaxDeviceNameSize - 9;
    char* pName = NULL;

    if (name)
    {
        
        strcpy(name, "default: ");
        pName = &name[9];
    }

    
    
    if (recDevice)
    {
        _recDisplayDeviceName = tmpName;
    } else
    {
        _playDisplayDeviceName = tmpName;
    }

    
    _paDeviceIndex = -1;
    _deviceIndex = 0;
    _numPlayDevices = 0;
    _numRecDevices = 0;

    PaLock();

    pa_operation* paOperation = NULL;

    
    paOperation = LATE(pa_context_get_server_info)(_paContext,
                                                   PaServerInfoCallback, this);

    WaitForOperationCompletion(paOperation);

    
    if (recDevice)
    {
        paOperation
            = LATE(pa_context_get_source_info_by_name)(_paContext,
                                                       (char *) tmpName,
                                                       PaSourceInfoCallback,
                                                       this);
    } else
    {
        paOperation
            = LATE(pa_context_get_sink_info_by_name)(_paContext,
                                                     (char *) tmpName,
                                                     PaSinkInfoCallback, this);
    }

    WaitForOperationCompletion(paOperation);

    PaUnLock();

    
    index = _paDeviceIndex;

    if (name)
    {
        
        strncpy(pName, tmpName, nameLen);
    }

    
    _playDisplayDeviceName = NULL;
    _recDisplayDeviceName = NULL;
    _paDeviceIndex = -1;
    _deviceIndex = -1;
    _numPlayDevices = 0;
    _numRecDevices = 0;

    return 0;
}

WebRtc_Word32 AudioDeviceLinuxPulse::InitPulseAudio()
{
    int retVal = 0;

    
    if (!PaSymbolTable.Load())
    {
        
        
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  failed to load symbol table");
        return -1;
    }

    
    
    if (_paMainloop) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  PA mainloop has already existed");
        return -1;
    }
    _paMainloop = LATE(pa_threaded_mainloop_new)();
    if (!_paMainloop)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  could not create mainloop");
        return -1;
    }

    
    retVal = LATE(pa_threaded_mainloop_start)(_paMainloop);
    if (retVal != PA_OK)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  failed to start main loop, error=%d", retVal);
        return -1;
    }

    WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                 "  mainloop running!");

    PaLock();

    _paMainloopApi = LATE(pa_threaded_mainloop_get_api)(_paMainloop);
    if (!_paMainloopApi)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  could not create mainloop API");
        PaUnLock();
        return -1;
    }

    
    if (_paContext){
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  PA context has already existed");
        PaUnLock();
        return -1;
    }
    _paContext = LATE(pa_context_new)(_paMainloopApi, "WEBRTC VoiceEngine");

    if (!_paContext)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  could not create context");
        PaUnLock();
        return -1;
    }

    
    LATE(pa_context_set_state_callback)(_paContext, PaContextStateCallback,
                                        this);

    
    _paStateChanged = false;
    retVal = LATE(pa_context_connect)(_paContext, NULL, PA_CONTEXT_NOAUTOSPAWN,
                                      NULL);

    if (retVal != PA_OK)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  failed to connect context, error=%d", retVal);
        PaUnLock();
        return -1;
    }

    
    while (!_paStateChanged)
    {
        LATE(pa_threaded_mainloop_wait)(_paMainloop);
    }

    
    pa_context_state_t state = LATE(pa_context_get_state)(_paContext);

    if (state != PA_CONTEXT_READY)
    {
        if (state == PA_CONTEXT_FAILED)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  failed to connect to PulseAudio sound server");
        } else if (state == PA_CONTEXT_TERMINATED)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  PulseAudio connection terminated early");
        } else
        {
            
            
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  unknown problem connecting to PulseAudio");
        }
        PaUnLock();
        return -1;
    }

    PaUnLock();

    
    _mixerManager.SetPulseAudioObjects(_paMainloop, _paContext);

    
    if (CheckPulseAudioVersion() < 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  PulseAudio version %s not supported", _paServerVersion);
        return -1;
    }

    
    if (InitSamplingFrequency() < 0 || _samplingFreq == 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  failed to initialize sampling frequency, set to %d",
                     _samplingFreq);
        return -1;
    }

    return 0;
}

WebRtc_Word32 AudioDeviceLinuxPulse::TerminatePulseAudio()
{
    
    
    if (!_paMainloop) {
        return 0;
    }

    PaLock();

    
    if (_paContext)
    {
        LATE(pa_context_disconnect)(_paContext);
    }

    
    if (_paContext)
    {
        LATE(pa_context_unref)(_paContext);
    }

    PaUnLock();
    _paContext = NULL;

    
    if (_paMainloop)
    {
        LATE(pa_threaded_mainloop_stop)(_paMainloop);
    }

    
    if (_paMainloop)
    {
        LATE(pa_threaded_mainloop_free)(_paMainloop);
    }

    _paMainloop = NULL;

    WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                 "  PulseAudio terminated");

    return 0;
}

void AudioDeviceLinuxPulse::PaLock()
{
    LATE(pa_threaded_mainloop_lock)(_paMainloop);
}

void AudioDeviceLinuxPulse::PaUnLock()
{
    LATE(pa_threaded_mainloop_unlock)(_paMainloop);
}

void AudioDeviceLinuxPulse::WaitForOperationCompletion(
    pa_operation* paOperation) const
{
    if (!paOperation)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "paOperation NULL in WaitForOperationCompletion");
        return;
    }

    while (LATE(pa_operation_get_state)(paOperation) == PA_OPERATION_RUNNING)
    {
        LATE(pa_threaded_mainloop_wait)(_paMainloop);
    }

    LATE(pa_operation_unref)(paOperation);
}





void AudioDeviceLinuxPulse::EnableWriteCallback()
{
    if (LATE(pa_stream_get_state)(_playStream) == PA_STREAM_READY)
    {
        
        _tempBufferSpace = LATE(pa_stream_writable_size)(_playStream);
        if (_tempBufferSpace > 0)
        {
            
            
            
            _timeEventPlay.Set();
            return;
        }
    }

    LATE(pa_stream_set_write_callback)(_playStream, &PaStreamWriteCallback,
                                       this);
}

void AudioDeviceLinuxPulse::DisableWriteCallback()
{
    LATE(pa_stream_set_write_callback)(_playStream, NULL, NULL);
}

void AudioDeviceLinuxPulse::PaStreamWriteCallback(pa_stream *,
                                                  size_t buffer_space,
                                                  void *pThis)
{
    static_cast<AudioDeviceLinuxPulse*> (pThis)->PaStreamWriteCallbackHandler(
        buffer_space);
}

void AudioDeviceLinuxPulse::PaStreamWriteCallbackHandler(size_t bufferSpace)
{
    _tempBufferSpace = bufferSpace;

    
    
    
    DisableWriteCallback();
    _timeEventPlay.Set();
}

void AudioDeviceLinuxPulse::PaStreamUnderflowCallback(pa_stream *,
                                                      void *pThis)
{
    static_cast<AudioDeviceLinuxPulse*> (pThis)->PaStreamUnderflowCallbackHandler();
}

void AudioDeviceLinuxPulse::PaStreamUnderflowCallbackHandler()
{
    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  Playout underflow");

    if (_configuredLatencyPlay == WEBRTC_PA_NO_LATENCY_REQUIREMENTS)
    {
        
        
        return;
    }

    

    const pa_sample_spec *spec = LATE(pa_stream_get_sample_spec)(_playStream);
    if (!spec)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  pa_stream_get_sample_spec()");
        return;
    }

    size_t bytesPerSec = LATE(pa_bytes_per_second)(spec);
    WebRtc_UWord32 newLatency = _configuredLatencyPlay + bytesPerSec
        * WEBRTC_PA_PLAYBACK_LATENCY_INCREMENT_MSECS / WEBRTC_PA_MSECS_PER_SEC;

    
    _playBufferAttr.maxlength = newLatency;
    _playBufferAttr.tlength = newLatency;
    _playBufferAttr.minreq = newLatency / WEBRTC_PA_PLAYBACK_REQUEST_FACTOR;
    _playBufferAttr.prebuf = _playBufferAttr.tlength - _playBufferAttr.minreq;

    pa_operation *op = LATE(pa_stream_set_buffer_attr)(_playStream,
                                                       &_playBufferAttr, NULL,
                                                       NULL);
    if (!op)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  pa_stream_set_buffer_attr()");
        return;
    }

    
    LATE(pa_operation_unref)(op);

    
    _configuredLatencyPlay = newLatency;
}

void AudioDeviceLinuxPulse::EnableReadCallback()
{
    LATE(pa_stream_set_read_callback)(_recStream, &PaStreamReadCallback, this);
}

void AudioDeviceLinuxPulse::DisableReadCallback()
{
    LATE(pa_stream_set_read_callback)(_recStream, NULL, NULL);
}

void AudioDeviceLinuxPulse::PaStreamReadCallback(pa_stream *,
                                                 size_t ,
                                                 void *pThis)
{
    static_cast<AudioDeviceLinuxPulse*> (pThis)->PaStreamReadCallbackHandler();
}

void AudioDeviceLinuxPulse::PaStreamReadCallbackHandler()
{
    
    
    if (LATE(pa_stream_peek)(_recStream, &_tempSampleData, &_tempSampleDataSize)
        != 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Can't read data!");
        return;
    }

    
    
    
    DisableReadCallback();
    _timeEventRec.Set();
}

void AudioDeviceLinuxPulse::PaStreamOverflowCallback(pa_stream *,
                                                     void *pThis)
{
    static_cast<AudioDeviceLinuxPulse*> (pThis)->PaStreamOverflowCallbackHandler();
}

void AudioDeviceLinuxPulse::PaStreamOverflowCallbackHandler()
{
    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  Recording overflow");
}

WebRtc_Word32 AudioDeviceLinuxPulse::LatencyUsecs(pa_stream *stream)
{
    if (!WEBRTC_PA_REPORT_LATENCY)
    {
        return 0;
    }

    if (!stream)
    {
        return 0;
    }

    pa_usec_t latency;
    int negative;
    if (LATE(pa_stream_get_latency)(stream, &latency, &negative) != 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Can't query latency");
        
        
        return 0;
    }

    if (negative)
    {
        WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                     "  warning: pa_stream_get_latency reported negative delay");

        
        
        
        WebRtc_Word32 tmpLatency = (WebRtc_Word32) -latency;
        if (tmpLatency < 0)
        {
            
            tmpLatency = 0;
        }

        return tmpLatency;
    } else
    {
        return (WebRtc_Word32) latency;
    }
}

WebRtc_Word32 AudioDeviceLinuxPulse::ReadRecordedData(const void* bufferData,
                                                      size_t bufferSize)
{
    size_t size = bufferSize;
    WebRtc_UWord32 numRecSamples = _recordBufferSize / (2 * _recChannels);

    
    WebRtc_UWord32 recDelay = (WebRtc_UWord32) ((LatencyUsecs(_recStream)
        / 1000) + 10 * ((size + _recordBufferUsed) / _recordBufferSize));

    _sndCardRecDelay = recDelay;

    if (_playStream)
    {
        
        _sndCardPlayDelay = (WebRtc_UWord32) (LatencyUsecs(_playStream) / 1000);
    }

    if (_recordBufferUsed > 0)
    {
        
        size_t copy = _recordBufferSize - _recordBufferUsed;
        if (size < copy)
        {
            copy = size;
        }

        memcpy(&_recBuffer[_recordBufferUsed], bufferData, copy);
        _recordBufferUsed += copy;
        bufferData = static_cast<const char *> (bufferData) + copy;
        size -= copy;

        if (_recordBufferUsed != _recordBufferSize)
        {
            
            return 0;
        }

        
        if (ProcessRecordedData(_recBuffer, numRecSamples, recDelay) == -1)
        {
            
            return -1;
        }

        _recordBufferUsed = 0;
    }

    
    while (size >= _recordBufferSize)
    {
        
        if (ProcessRecordedData(
            static_cast<WebRtc_Word8 *> (const_cast<void *> (bufferData)),
            numRecSamples, recDelay) == -1)
        {
            
            return -1;
        }

        bufferData = static_cast<const char *> (bufferData) + _recordBufferSize;
        size -= _recordBufferSize;

        
        recDelay -= 10;
    }

    
    if (size > 0)
    {
        memcpy(_recBuffer, bufferData, size);
        _recordBufferUsed = size;
    }

    return 0;
}

WebRtc_Word32 AudioDeviceLinuxPulse::ProcessRecordedData(
    WebRtc_Word8 *bufferData,
    WebRtc_UWord32 bufferSizeInSamples,
    WebRtc_UWord32 recDelay)
{
    WebRtc_UWord32 currentMicLevel(0);
    WebRtc_UWord32 newMicLevel(0);

    _ptrAudioBuffer->SetRecordedBuffer(bufferData, bufferSizeInSamples);

    if (AGC())
    {
        
        if (MicrophoneVolume(currentMicLevel) == 0)
        {
            
            _ptrAudioBuffer->SetCurrentMicLevel(currentMicLevel);
        }
    }

    const WebRtc_UWord32 clockDrift(0);
    
    
    
    
    
    if (recDelay > 10)
        recDelay -= 10;
    else
        recDelay = 0;
    _ptrAudioBuffer->SetVQEData(_sndCardPlayDelay, recDelay, clockDrift);

    
    
    UnLock();
    _ptrAudioBuffer->DeliverRecordedData();
    Lock();

    
    if (!_recording)
    {
        return -1;
    }

    if (AGC())
    {
        newMicLevel = _ptrAudioBuffer->NewMicLevel();
        if (newMicLevel != 0)
        {
            
            
            
            
            WEBRTC_TRACE(kTraceStream, kTraceAudioDevice, _id,
                         "  AGC change of volume: old=%u => new=%u",
                         currentMicLevel, newMicLevel);
            if (SetMicrophoneVolume(newMicLevel) == -1)
            {
                WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice,
                             _id,
                             "  the required modification of the microphone "
                             "volume failed");
            }
        }
    }

    return 0;
}

bool AudioDeviceLinuxPulse::PlayThreadFunc(void* pThis)
{
    return (static_cast<AudioDeviceLinuxPulse*> (pThis)->PlayThreadProcess());
}

bool AudioDeviceLinuxPulse::RecThreadFunc(void* pThis)
{
    return (static_cast<AudioDeviceLinuxPulse*> (pThis)->RecThreadProcess());
}

bool AudioDeviceLinuxPulse::PlayThreadProcess()
{
    switch (_timeEventPlay.Wait(1000))
    {
        case kEventSignaled:
            _timeEventPlay.Reset();
            break;
        case kEventError:
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "EventWrapper::Wait() failed");
            return true;
        case kEventTimeout:
            return true;
    }

    Lock();

    if (_startPlay)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     "_startPlay true, performing initial actions");

        _startPlay = false;
        _playDeviceName = NULL;

        
        if (_outputDeviceIndex > 0)
        {
            
            _playDeviceName = new char[kAdmMaxDeviceNameSize];
            _deviceIndex = _outputDeviceIndex;
            PlayoutDevices();
        }

        
        if (LATE(pa_context_get_protocol_version)(_paContext)
            >= WEBRTC_PA_ADJUST_LATENCY_PROTOCOL_VERSION)
        {
            
            
            bool enabled(false);
            _mixerManager.SpeakerMute(enabled);
            if (enabled)
            {
                _playStreamFlags |= PA_STREAM_START_MUTED;
            }
        }

        
        WebRtc_UWord32 volume = 0;
        if (update_speaker_volume_at_startup_)
          _mixerManager.SpeakerVolume(volume);

        PaLock();

        
        pa_cvolume* ptr_cvolume = NULL;
        if (update_speaker_volume_at_startup_) {
          pa_cvolume cVolumes;
          ptr_cvolume = &cVolumes;

          
          const pa_sample_spec *spec =
              LATE(pa_stream_get_sample_spec)(_playStream);
          LATE(pa_cvolume_set)(&cVolumes, spec->channels, volume);
          update_speaker_volume_at_startup_ = false;
        }

        
        if (LATE(pa_stream_connect_playback)(
            _playStream,
            _playDeviceName,
            &_playBufferAttr,
            (pa_stream_flags_t) _playStreamFlags,
            ptr_cvolume, NULL) != PA_OK)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  failed to connect play stream, err=%d",
                         LATE(pa_context_errno)(_paContext));
        }

        WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                     "  play stream connected");

        
        while (LATE(pa_stream_get_state)(_playStream) != PA_STREAM_READY)
        {
            LATE(pa_threaded_mainloop_wait)(_paMainloop);
        }

        WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                     "  play stream ready");

        
        EnableWriteCallback();

        PaUnLock();

        
        if (_playDeviceName)
        {
            delete [] _playDeviceName;
            _playDeviceName = NULL;
        }

        _playing = true;
        _playStartEvent.Set();

        UnLock();
        return true;
    }

    if (_playing)
    {
        if (!_recording)
        {
            
            _sndCardPlayDelay = (WebRtc_UWord32) (LatencyUsecs(_playStream)
                / 1000);
        }

        if (_playbackBufferUnused < _playbackBufferSize)
        {

            size_t write = _playbackBufferSize - _playbackBufferUnused;
            if (_tempBufferSpace < write)
            {
                write = _tempBufferSpace;
            }

            PaLock();
            if (LATE(pa_stream_write)(
                                      _playStream,
                                      (void *) &_playBuffer[_playbackBufferUnused],
                                      write, NULL, (int64_t) 0,
                                      PA_SEEK_RELATIVE) != PA_OK)
            {
                _writeErrors++;
                if (_writeErrors > 10)
                {
                    if (_playError == 1)
                    {
                        WEBRTC_TRACE(kTraceWarning,
                                     kTraceUtility, _id,
                                     "  pending playout error exists");
                    }
                    _playError = 1; 
                    WEBRTC_TRACE(
                                 kTraceError,
                                 kTraceUtility,
                                 _id,
                                 "  kPlayoutError message posted: "
                                 "_writeErrors=%u, error=%d",
                                 _writeErrors,
                                 LATE(pa_context_errno)(_paContext));
                    _writeErrors = 0;
                }
            }
            PaUnLock();

            _playbackBufferUnused += write;
            _tempBufferSpace -= write;
        }

        WebRtc_UWord32 numPlaySamples = _playbackBufferSize / (2
            * _playChannels);
        if (_tempBufferSpace > 0) 
        {
            
            
            
            UnLock();
            WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                         "  requesting data");
            WebRtc_UWord32 nSamples =
                _ptrAudioBuffer->RequestPlayoutData(numPlaySamples);
            Lock();

            
            if (!_playing)
            {
                UnLock();
                return true;
            }

            nSamples = _ptrAudioBuffer->GetPlayoutData(_playBuffer);
            if (nSamples != numPlaySamples)
            {
                WEBRTC_TRACE(kTraceError, kTraceAudioDevice,
                             _id, "  invalid number of output samples(%d)",
                             nSamples);
            }

            size_t write = _playbackBufferSize;
            if (_tempBufferSpace < write)
            {
                write = _tempBufferSpace;
            }

            WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                         "  will write");
            PaLock();
            if (LATE(pa_stream_write)(_playStream, (void *) &_playBuffer[0],
                                      write, NULL, (int64_t) 0,
                                      PA_SEEK_RELATIVE) != PA_OK)
            {
                _writeErrors++;
                if (_writeErrors > 10)
                {
                    if (_playError == 1)
                    {
                        WEBRTC_TRACE(kTraceWarning,
                                     kTraceUtility, _id,
                                     "  pending playout error exists");
                    }
                    _playError = 1; 
                    WEBRTC_TRACE(
                                 kTraceError,
                                 kTraceUtility,
                                 _id,
                                 "  kPlayoutError message posted: "
                                 "_writeErrors=%u, error=%d",
                                 _writeErrors,
                                 LATE(pa_context_errno)(_paContext));
                    _writeErrors = 0;
                }
            }
            PaUnLock();

            _playbackBufferUnused = write;
        }

        _tempBufferSpace = 0;
        PaLock();
        EnableWriteCallback();
        PaUnLock();

    } 

    UnLock();
    return true;
}

bool AudioDeviceLinuxPulse::RecThreadProcess()
{
    switch (_timeEventRec.Wait(1000))
    {
        case kEventSignaled:
            _timeEventRec.Reset();
            break;
        case kEventError:
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "EventWrapper::Wait() failed");
            return true;
        case kEventTimeout:
            return true;
    }

    Lock();

    if (_startRec)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     "_startRec true, performing initial actions");

        _recDeviceName = NULL;

        
        if (_inputDeviceIndex > 0)
        {
            
            _recDeviceName = new char[kAdmMaxDeviceNameSize];
            _deviceIndex = _inputDeviceIndex;
            RecordingDevices();
        }

        PaLock();

        WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                     "  connecting stream");

        
        if (LATE(pa_stream_connect_record)(_recStream, _recDeviceName,
                                           &_recBufferAttr,
                                           (pa_stream_flags_t) _recStreamFlags)
            != PA_OK)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  failed to connect rec stream, err=%d",
                         LATE(pa_context_errno)(_paContext));
        }

        WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                     "  connected");

        
        while (LATE(pa_stream_get_state)(_recStream) != PA_STREAM_READY)
        {
            LATE(pa_threaded_mainloop_wait)(_paMainloop);
        }

        WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                     "  done");

        
        EnableReadCallback();

        PaUnLock();

        
        if (_recDeviceName)
        {
            delete [] _recDeviceName;
            _recDeviceName = NULL;
        }

        _startRec = false;
        _recording = true;
        _recStartEvent.Set();

        UnLock();
        return true;
    }

    if (_recording)
    {
        
        if (ReadRecordedData(_tempSampleData, _tempSampleDataSize) == -1)
        {
            UnLock();
            return true;
        }

        _tempSampleData = NULL;
        _tempSampleDataSize = 0;

        PaLock();
        while (true)
        {
            
            if (LATE(pa_stream_drop)(_recStream) != 0)
            {
                WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice,
                             _id, "  failed to drop, err=%d\n",
                             LATE(pa_context_errno)(_paContext));
            }

            if (LATE(pa_stream_readable_size)(_recStream) <= 0)
            {
                
                break;
            }

            
            const void *sampleData;
            size_t sampleDataSize;

            if (LATE(pa_stream_peek)(_recStream, &sampleData, &sampleDataSize)
                != 0)
            {
                _recError = 1; 
                WEBRTC_TRACE(kTraceError, kTraceAudioDevice,
                             _id, "  RECORD_ERROR message posted, error = %d",
                             LATE(pa_context_errno)(_paContext));
                break;
            }

            _sndCardRecDelay = (WebRtc_UWord32) (LatencyUsecs(_recStream)
                / 1000);

            
            PaUnLock();
            
            if (ReadRecordedData(sampleData, sampleDataSize) == -1)
            {
                UnLock();
                return true;
            }
            PaLock();

            
        }

        EnableReadCallback();
        PaUnLock();

    } 

    UnLock();
    return true;
}

}

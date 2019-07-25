









#include "audio_device_dummy.h"

#include <string.h>

#include "trace.h"
#include "thread_wrapper.h"
#include "event_wrapper.h"




namespace webrtc {

const WebRtc_UWord32 REC_TIMER_PERIOD_MS = 10;
const WebRtc_UWord32 PLAY_TIMER_PERIOD_MS = 10;









AudioDeviceDummy::AudioDeviceDummy(const WebRtc_Word32 id) :
	  _ptrAudioBuffer(NULL),
    _critSect(*CriticalSectionWrapper::CreateCriticalSection()),
    _id(id),
    _timeEventRec(*EventWrapper::Create()),
    _timeEventPlay(*EventWrapper::Create()),
    _recStartEvent(*EventWrapper::Create()),
    _playStartEvent(*EventWrapper::Create()),
    _ptrThreadRec(NULL),
    _ptrThreadPlay(NULL),
    _recThreadID(0),
    _playThreadID(0),
    _initialized(false),
    _recording(false),
    _playing(false),
    _recIsInitialized(false),
    _playIsInitialized(false),
    _speakerIsInitialized(false),
    _microphoneIsInitialized(false),
    _playDataFile(NULL)
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, id, "%s created", __FUNCTION__);

    memset(_recBuffer, 0, sizeof(_recBuffer));
    WebRtc_Word16* tmp = (WebRtc_Word16*)_recBuffer;

    





    
    for(int i=0; i<20; ++i)
    {
      tmp[i*8] = 0;
      tmp[i*8+1] = -5000;
      tmp[i*8+2] = -16000;
      tmp[i*8+3] = -5000;
      tmp[i*8+4] = 0;
      tmp[i*8+5] = 5000;
      tmp[i*8+6] = 16000;
      tmp[i*8+7] = 5000;
    }
  
#ifdef RECORD_PLAYOUT
    _playDataFile = fopen("webrtc_VoiceEngine_playout.pcm", "wb");
    if (!_playDataFile)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                   "  Could not open file for writing playout data");
    }
#endif
}





AudioDeviceDummy::~AudioDeviceDummy()
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id, "%s destroyed", __FUNCTION__);

    Terminate();

    _ptrAudioBuffer = NULL;

    delete &_recStartEvent;
    delete &_playStartEvent;
    delete &_timeEventRec;
    delete &_timeEventPlay;
    delete &_critSect;

    if (_playDataFile)
    {
        fclose(_playDataFile);
    }
}









void AudioDeviceDummy::AttachAudioBuffer(AudioDeviceBuffer* audioBuffer)
{

    _ptrAudioBuffer = audioBuffer;

    
    _ptrAudioBuffer->SetRecordingSampleRate(16000);
    _ptrAudioBuffer->SetPlayoutSampleRate(16000);
    _ptrAudioBuffer->SetRecordingChannels(1);
    _ptrAudioBuffer->SetPlayoutChannels(1);
}





WebRtc_Word32 AudioDeviceDummy::ActiveAudioLayer(
    AudioDeviceModule::AudioLayer& audioLayer) const
{
    audioLayer = AudioDeviceModule::kDummyAudio;
    return 0;
}





WebRtc_Word32 AudioDeviceDummy::Init()
{

    CriticalSectionScoped lock(&_critSect);

    if (_initialized)
    {
        return 0;
    }

    const bool periodic(true);
    unsigned int threadID(0);
    char threadName[64] = {0};

    
    strncpy(threadName, "webrtc_audio_module_rec_thread", 63);
    _ptrThreadRec = ThreadWrapper::CreateThread(
        RecThreadFunc, this, kRealtimePriority, threadName);
    if (_ptrThreadRec == NULL)
    {
        WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                     "  failed to create the rec audio thread");
        return -1;
    }

    if (!_ptrThreadRec->Start(threadID))
    {
        WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                     "  failed to start the rec audio thread");
        delete _ptrThreadRec;
        _ptrThreadRec = NULL;
        return -1;
    }
    _recThreadID = threadID;

    if (!_timeEventRec.StartTimer(periodic, REC_TIMER_PERIOD_MS))
    {
        WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                     "  failed to start the rec timer event");
        if (_ptrThreadRec->Stop())
        {
            delete _ptrThreadRec;
            _ptrThreadRec = NULL;
        }
        else
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "  unable to stop the activated rec thread");
        }
        return -1;
    }

    
    strncpy(threadName, "webrtc_audio_module_play_thread", 63);
    _ptrThreadPlay = ThreadWrapper::CreateThread(
        PlayThreadFunc, this, kRealtimePriority, threadName);
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

    if (!_timeEventPlay.StartTimer(periodic, PLAY_TIMER_PERIOD_MS))
    {
        WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                     "  failed to start the play timer event");
        if (_ptrThreadPlay->Stop())
        {
            delete _ptrThreadPlay;
            _ptrThreadPlay = NULL;
        }
        else
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "  unable to stop the activated play thread");
        }
        return -1;
    }

    _initialized = true;

    return 0;
}





WebRtc_Word32 AudioDeviceDummy::Terminate()
{

    CriticalSectionScoped lock(&_critSect);

    if (!_initialized)
    {
        return 0;
    }

    
    if (_ptrThreadRec)
    {
        ThreadWrapper* tmpThread = _ptrThreadRec;
        _ptrThreadRec = NULL;
        _critSect.Leave();

        tmpThread->SetNotAlive();
        _timeEventRec.Set();

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

    _timeEventRec.StopTimer();

    
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
        }
        else
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "  failed to close down the play audio thread");
        }

        _critSect.Enter();
    }

    _timeEventPlay.StopTimer();

    _initialized = false;

    return 0;
}





bool AudioDeviceDummy::Initialized() const
{
    return (_initialized);
}





WebRtc_Word32 AudioDeviceDummy::SpeakerIsAvailable(bool& available)
{

    CriticalSectionScoped lock(&_critSect);

    available = true;

    return 0;
}





WebRtc_Word32 AudioDeviceDummy::InitSpeaker()
{

    CriticalSectionScoped lock(&_critSect);

    if (_playing)
    {
        return -1;
    }

	_speakerIsInitialized = true;

	return 0;
}





WebRtc_Word32 AudioDeviceDummy::MicrophoneIsAvailable(bool& available)
{

    CriticalSectionScoped lock(&_critSect);

    available = true;

    return 0;
}





WebRtc_Word32 AudioDeviceDummy::InitMicrophone()
{

    CriticalSectionScoped lock(&_critSect);

    if (_recording)
    {
        return -1;
    }

    _microphoneIsInitialized = true;

    return 0;
}





bool AudioDeviceDummy::SpeakerIsInitialized() const
{

    return (_speakerIsInitialized);
}





bool AudioDeviceDummy::MicrophoneIsInitialized() const
{

    return (_microphoneIsInitialized);
}





WebRtc_Word32 AudioDeviceDummy::SpeakerVolumeIsAvailable(bool& available)
{

    CriticalSectionScoped lock(&_critSect);

    available = false;

    return 0;
}





WebRtc_Word32 AudioDeviceDummy::SetSpeakerVolume(WebRtc_UWord32 volume)
{

	return -1;
}





WebRtc_Word32 AudioDeviceDummy::SpeakerVolume(WebRtc_UWord32& volume) const
{

    return -1;
}





WebRtc_Word32 AudioDeviceDummy::SetWaveOutVolume(WebRtc_UWord16 volumeLeft, WebRtc_UWord16 volumeRight)
{

    return -1;
}





WebRtc_Word32 AudioDeviceDummy::WaveOutVolume(WebRtc_UWord16& volumeLeft, WebRtc_UWord16& volumeRight) const
{

    return -1;
}





WebRtc_Word32 AudioDeviceDummy::MaxSpeakerVolume(WebRtc_UWord32& maxVolume) const
{

    return -1;
}





WebRtc_Word32 AudioDeviceDummy::MinSpeakerVolume(WebRtc_UWord32& minVolume) const
{

    return -1;
}





WebRtc_Word32 AudioDeviceDummy::SpeakerVolumeStepSize(WebRtc_UWord16& stepSize) const
{
	
    return -1;
}





WebRtc_Word32 AudioDeviceDummy::SpeakerMuteIsAvailable(bool& available)
{

    CriticalSectionScoped lock(&_critSect);

    available = false;

    return 0;
}





WebRtc_Word32 AudioDeviceDummy::SetSpeakerMute(bool enable)
{

    return -1;
}





WebRtc_Word32 AudioDeviceDummy::SpeakerMute(bool& enabled) const
{

    return -1;
}





WebRtc_Word32 AudioDeviceDummy::MicrophoneMuteIsAvailable(bool& available)
{

    CriticalSectionScoped lock(&_critSect);

    available = false;

    return 0;
}





WebRtc_Word32 AudioDeviceDummy::SetMicrophoneMute(bool enable)
{

    return -1;
}





WebRtc_Word32 AudioDeviceDummy::MicrophoneMute(bool& enabled) const
{

    return -1;
}





WebRtc_Word32 AudioDeviceDummy::MicrophoneBoostIsAvailable(bool& available)
{

    available = false;
    return 0;
}





WebRtc_Word32 AudioDeviceDummy::SetMicrophoneBoost(bool enable)
{

    return -1;
}





WebRtc_Word32 AudioDeviceDummy::MicrophoneBoost(bool& enabled) const
{

    return -1;
}





WebRtc_Word32 AudioDeviceDummy::StereoRecordingIsAvailable(bool& available)
{

    available = false;
    return 0;
}





WebRtc_Word32 AudioDeviceDummy::SetStereoRecording(bool enable)
{

    CriticalSectionScoped lock(&_critSect);

    if (enable)
    {
        return -1;
    }

    return 0;
}





WebRtc_Word32 AudioDeviceDummy::StereoRecording(bool& enabled) const
{

    enabled = false;

    return 0;
}





WebRtc_Word32 AudioDeviceDummy::StereoPlayoutIsAvailable(bool& available)
{

    available = false;
    return 0;
}





WebRtc_Word32 AudioDeviceDummy::SetStereoPlayout(bool enable)
{

    CriticalSectionScoped lock(&_critSect);

    if (enable)
    {
        return -1;
    }

    return 0;
}





WebRtc_Word32 AudioDeviceDummy::StereoPlayout(bool& enabled) const
{

    enabled = false;

    return 0;
}





WebRtc_Word32 AudioDeviceDummy::SetAGC(bool enable)
{

    return -1;
}





bool AudioDeviceDummy::AGC() const
{
    
    return false;
}





WebRtc_Word32 AudioDeviceDummy::MicrophoneVolumeIsAvailable(bool& available)
{

    CriticalSectionScoped lock(&_critSect);

    available = false;

    return 0;
}





WebRtc_Word32 AudioDeviceDummy::SetMicrophoneVolume(WebRtc_UWord32 volume)
{
    WEBRTC_TRACE(kTraceStream, kTraceAudioDevice, _id,
                 "AudioDeviceDummy::SetMicrophoneVolume(volume=%u)", volume);

    CriticalSectionScoped lock(&_critSect);

    return -1;
}





WebRtc_Word32 AudioDeviceDummy::MicrophoneVolume(WebRtc_UWord32& volume) const
{
    

    CriticalSectionScoped lock(&_critSect);

    return -1;
}





WebRtc_Word32 AudioDeviceDummy::MaxMicrophoneVolume(
    WebRtc_UWord32& maxVolume) const
{
    WEBRTC_TRACE(kTraceStream, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    return -1;
}





WebRtc_Word32 AudioDeviceDummy::MinMicrophoneVolume(
    WebRtc_UWord32& minVolume) const
{

    return -1;
}





WebRtc_Word32 AudioDeviceDummy::MicrophoneVolumeStepSize(
    WebRtc_UWord16& stepSize) const
{

    return -1;
}





WebRtc_Word16 AudioDeviceDummy::PlayoutDevices()
{

    CriticalSectionScoped lock(&_critSect);

    return 1;
}





WebRtc_Word32 AudioDeviceDummy::SetPlayoutDevice(WebRtc_UWord16 index)
{

    if (_playIsInitialized)
    {
        return -1;
    }

    if (index != 0)
    {
      return -1;
    }

    return 0;
}





WebRtc_Word32 AudioDeviceDummy::SetPlayoutDevice(
    AudioDeviceModule::WindowsDeviceType device)
{
	return -1;
}





WebRtc_Word32 AudioDeviceDummy::PlayoutDeviceName(
    WebRtc_UWord16 index,
    char name[kAdmMaxDeviceNameSize],
    char guid[kAdmMaxGuidSize])
{

    if (index != 0)
    {
        return -1;
    }

    memset(name, 0, kAdmMaxDeviceNameSize);

    if (guid != NULL)
    {
      memset(guid, 0, kAdmMaxGuidSize);
    }

    return 0;
}





WebRtc_Word32 AudioDeviceDummy::RecordingDeviceName(
    WebRtc_UWord16 index,
    char name[kAdmMaxDeviceNameSize],
    char guid[kAdmMaxGuidSize])
{

    if (index != 0)
    {
        return -1;
    }

    memset(name, 0, kAdmMaxDeviceNameSize);

    if (guid != NULL)
    {
        memset(guid, 0, kAdmMaxGuidSize);
    }

    return 0;
}





WebRtc_Word16 AudioDeviceDummy::RecordingDevices()
{

    CriticalSectionScoped lock(&_critSect);

    return 1;
}





WebRtc_Word32 AudioDeviceDummy::SetRecordingDevice(WebRtc_UWord16 index)
{

    if (_recIsInitialized)
    {
        return -1;
    }

    if (index != 0 )
    {
        return -1;
    }

    return 0;
}





WebRtc_Word32 AudioDeviceDummy::SetRecordingDevice(
    AudioDeviceModule::WindowsDeviceType device)
{
    return -1;
}





WebRtc_Word32 AudioDeviceDummy::PlayoutIsAvailable(bool& available)
{

    available = true;

    return 0;
}





WebRtc_Word32 AudioDeviceDummy::RecordingIsAvailable(bool& available)
{

    available = true;

    return 0;
}





WebRtc_Word32 AudioDeviceDummy::InitPlayout()
{

    CriticalSectionScoped lock(&_critSect);

    if (_playing)
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

    _playIsInitialized = true;

    return 0;
}





WebRtc_Word32 AudioDeviceDummy::InitRecording()
{

    CriticalSectionScoped lock(&_critSect);

    if (_recording)
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

    _recIsInitialized = true;

    return 0;

}





WebRtc_Word32 AudioDeviceDummy::StartRecording()
{

    CriticalSectionScoped lock(&_critSect);

    if (!_recIsInitialized)
    {
        return -1;
    }

    if (_recording)
    {
        return 0;
    }

    _recording = true;

    return 0;
}





WebRtc_Word32 AudioDeviceDummy::StopRecording()
{

    CriticalSectionScoped lock(&_critSect);

    if (!_recIsInitialized)
    {
        return 0;
    }

    _recIsInitialized = false;
    _recording = false;

    return 0;
}





bool AudioDeviceDummy::RecordingIsInitialized() const
{
    return (_recIsInitialized);
}





bool AudioDeviceDummy::Recording() const
{
    return (_recording);
}





bool AudioDeviceDummy::PlayoutIsInitialized() const
{

    return (_playIsInitialized);
}





WebRtc_Word32 AudioDeviceDummy::StartPlayout()
{

    CriticalSectionScoped lock(&_critSect);

    if (!_playIsInitialized)
    {
        return -1;
    }

    if (_playing)
    {
        return 0;
    }

    _playing = true;

    return 0;
}





WebRtc_Word32 AudioDeviceDummy::StopPlayout()
{

    if (!_playIsInitialized)
    {
        return 0;
    }

    _playIsInitialized = false;
    _playing = false;

    return 0;
}





WebRtc_Word32 AudioDeviceDummy::PlayoutDelay(WebRtc_UWord16& delayMS) const
{
    CriticalSectionScoped lock(&_critSect);
    delayMS = 0;
    return 0;
}





WebRtc_Word32 AudioDeviceDummy::RecordingDelay(WebRtc_UWord16& delayMS) const
{
    CriticalSectionScoped lock(&_critSect);
    delayMS = 0;
    return 0;
}





bool AudioDeviceDummy::Playing() const
{
    return (_playing);
}




WebRtc_Word32 AudioDeviceDummy::SetPlayoutBuffer(
    const AudioDeviceModule::BufferType type, WebRtc_UWord16 sizeMS)
{

    CriticalSectionScoped lock(&_critSect);

    

    return 0;
}





WebRtc_Word32 AudioDeviceDummy::PlayoutBuffer(
    AudioDeviceModule::BufferType& type, WebRtc_UWord16& sizeMS) const
{
    CriticalSectionScoped lock(&_critSect);

    type = AudioDeviceModule::kAdaptiveBufferSize;
    sizeMS = 0;

    return 0;
}





WebRtc_Word32 AudioDeviceDummy::CPULoad(WebRtc_UWord16& load) const
{

    load = 0;

    return 0;
}





bool AudioDeviceDummy::PlayoutWarning() const
{
    return false;
}





bool AudioDeviceDummy::PlayoutError() const
{
    return false;
}





bool AudioDeviceDummy::RecordingWarning() const
{
    return false;
}





bool AudioDeviceDummy::RecordingError() const
{
    return false;
}





void AudioDeviceDummy::ClearPlayoutWarning()
{
}





void AudioDeviceDummy::ClearPlayoutError()
{
}





void AudioDeviceDummy::ClearRecordingWarning()
{
}





void AudioDeviceDummy::ClearRecordingError()
{
}









bool AudioDeviceDummy::PlayThreadFunc(void* pThis)
{
    return (static_cast<AudioDeviceDummy*>(pThis)->PlayThreadProcess());
}





bool AudioDeviceDummy::RecThreadFunc(void* pThis)
{
    return (static_cast<AudioDeviceDummy*>(pThis)->RecThreadProcess());
}





bool AudioDeviceDummy::PlayThreadProcess()
{
    switch (_timeEventPlay.Wait(1000))
    {
    case kEventSignaled:
        break;
    case kEventError:
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                   "EventWrapper::Wait() failed => restarting timer");
        _timeEventPlay.StopTimer();
        _timeEventPlay.StartTimer(true, PLAY_TIMER_PERIOD_MS);
        return true;
    case kEventTimeout:
        return true;
    }

    Lock();

    if(_playing)
    {
        WebRtc_Word8 playBuffer[2*160];

        UnLock();
        WebRtc_Word32 nSamples = (WebRtc_Word32)_ptrAudioBuffer->RequestPlayoutData(160);
        Lock();

        if (!_playing)
        {
            UnLock();
            return true;
        }

        nSamples = _ptrAudioBuffer->GetPlayoutData(playBuffer);
        if (nSamples != 160)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                "  invalid number of output samples(%d)", nSamples);
        }

        if (_playDataFile)
        {
            int wr = fwrite(playBuffer, 2, 160, _playDataFile);
            if (wr != 160)
            {
                WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                           "  Could not write playout data to file (%d) ferror = %d",
                           wr, ferror(_playDataFile));
            }
        }
    }

    UnLock();
    return true;
}





bool AudioDeviceDummy::RecThreadProcess()
{
    switch (_timeEventRec.Wait(1000))
    {
    case kEventSignaled:
        break;
    case kEventError:
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                   "EventWrapper::Wait() failed => restarting timer");
        _timeEventRec.StopTimer();
        _timeEventRec.StartTimer(true, REC_TIMER_PERIOD_MS);
        return true;
    case kEventTimeout:
        return true;
    }

    Lock();

    if (_recording)
    {
        
        _ptrAudioBuffer->SetRecordedBuffer(_recBuffer, 160);

        
        _ptrAudioBuffer->SetVQEData(0, 0, 0);

        
        UnLock();
        _ptrAudioBuffer->DeliverRecordedData();
    }
    else
    {
        UnLock();
    }

    return true;
}

}  

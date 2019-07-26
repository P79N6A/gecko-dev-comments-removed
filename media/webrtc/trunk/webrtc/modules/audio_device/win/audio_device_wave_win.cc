









#include "audio_device_utility.h"
#include "audio_device_wave_win.h"
#include "audio_device_config.h"

#include "trace.h"
#include "thread_wrapper.h"
#include "event_wrapper.h"

#include <windows.h>
#include <objbase.h>    
#include <strsafe.h>    
#include <cassert>


#ifndef WAVE_MAPPED_DEFAULT_COMMUNICATION_DEVICE
#define WAVE_MAPPED_DEFAULT_COMMUNICATION_DEVICE   0x0010
#endif




#define DRV_RESERVED                      0x0800
#define DRV_QUERYFUNCTIONINSTANCEID       (DRV_RESERVED + 17)
#define DRV_QUERYFUNCTIONINSTANCEIDSIZE   (DRV_RESERVED + 18)

#define POW2(A) (2 << ((A) - 1))

namespace webrtc {









AudioDeviceWindowsWave::AudioDeviceWindowsWave(const WebRtc_Word32 id) :
    _ptrAudioBuffer(NULL),
    _critSect(*CriticalSectionWrapper::CreateCriticalSection()),
    _timeEvent(*EventWrapper::Create()),
    _recStartEvent(*EventWrapper::Create()),
    _playStartEvent(*EventWrapper::Create()),
    _hGetCaptureVolumeThread(NULL),
    _hShutdownGetVolumeEvent(NULL),
    _hSetCaptureVolumeThread(NULL),
    _hShutdownSetVolumeEvent(NULL),
    _hSetCaptureVolumeEvent(NULL),
    _ptrThread(NULL),
    _threadID(0),
    _critSectCb(*CriticalSectionWrapper::CreateCriticalSection()),
    _id(id),
    _mixerManager(id),
    _usingInputDeviceIndex(false),
    _usingOutputDeviceIndex(false),
    _inputDevice(AudioDeviceModule::kDefaultDevice),
    _outputDevice(AudioDeviceModule::kDefaultDevice),
    _inputDeviceIndex(0),
    _outputDeviceIndex(0),
    _inputDeviceIsSpecified(false),
    _outputDeviceIsSpecified(false),
    _initialized(false),
    _recIsInitialized(false),
    _playIsInitialized(false),
    _recording(false),
    _playing(false),
    _startRec(false),
    _stopRec(false),
    _startPlay(false),
    _stopPlay(false),
    _AGC(false),
    _hWaveIn(NULL),
    _hWaveOut(NULL),
    _recChannels(N_REC_CHANNELS),
    _playChannels(N_PLAY_CHANNELS),
    _recBufCount(0),
    _recPutBackDelay(0),
    _recDelayCount(0),
    _playBufCount(0),
    _prevPlayTime(0),
    _prevRecTime(0),
    _prevTimerCheckTime(0),
    _timesdwBytes(0),
    _timerFaults(0),
    _timerRestartAttempts(0),
    _no_of_msecleft_warnings(0),
    _MAX_minBuffer(65),
    _useHeader(0),
    _dTcheckPlayBufDelay(10),
    _playBufDelay(80),
    _playBufDelayFixed(80),
    _minPlayBufDelay(20),
    _avgCPULoad(0),
    _sndCardPlayDelay(0),
    _sndCardRecDelay(0),
    _plSampOld(0),
    _rcSampOld(0),
    _playBufType(AudioDeviceModule::kAdaptiveBufferSize),
    _recordedBytes(0),
    _playWarning(0),
    _playError(0),
    _recWarning(0),
    _recError(0),
    _newMicLevel(0),
    _minMicVolume(0),
    _maxMicVolume(0)
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, id, "%s created", __FUNCTION__);

    
    if (!QueryPerformanceFrequency(&_perfFreq))
    {
        _perfFreq.QuadPart = 0;
    }

    _hShutdownGetVolumeEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    _hShutdownSetVolumeEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    _hSetCaptureVolumeEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
}





AudioDeviceWindowsWave::~AudioDeviceWindowsWave()
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id, "%s destroyed", __FUNCTION__);

    Terminate();

    delete &_recStartEvent;
    delete &_playStartEvent;
    delete &_timeEvent;
    delete &_critSect;
    delete &_critSectCb;

    if (NULL != _hShutdownGetVolumeEvent)
    {
        CloseHandle(_hShutdownGetVolumeEvent);
        _hShutdownGetVolumeEvent = NULL;
    }

    if (NULL != _hShutdownSetVolumeEvent)
    {
        CloseHandle(_hShutdownSetVolumeEvent);
        _hShutdownSetVolumeEvent = NULL;
    }

    if (NULL != _hSetCaptureVolumeEvent)
    {
        CloseHandle(_hSetCaptureVolumeEvent);
        _hSetCaptureVolumeEvent = NULL;
    }
}









void AudioDeviceWindowsWave::AttachAudioBuffer(AudioDeviceBuffer* audioBuffer)
{

    CriticalSectionScoped lock(&_critSect);

    _ptrAudioBuffer = audioBuffer;

    
    _ptrAudioBuffer->SetRecordingSampleRate(N_REC_SAMPLES_PER_SEC);
    _ptrAudioBuffer->SetPlayoutSampleRate(N_PLAY_SAMPLES_PER_SEC);
    _ptrAudioBuffer->SetRecordingChannels(N_REC_CHANNELS);
    _ptrAudioBuffer->SetPlayoutChannels(N_PLAY_CHANNELS);
}





WebRtc_Word32 AudioDeviceWindowsWave::ActiveAudioLayer(AudioDeviceModule::AudioLayer& audioLayer) const
{
    audioLayer = AudioDeviceModule::kWindowsWaveAudio;
    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::Init()
{

    CriticalSectionScoped lock(&_critSect);

    if (_initialized)
    {
        return 0;
    }

    const WebRtc_UWord32 nowTime(AudioDeviceUtility::GetTimeInMS());

    _recordedBytes = 0;
    _prevRecByteCheckTime = nowTime;
    _prevRecTime = nowTime;
    _prevPlayTime = nowTime;
    _prevTimerCheckTime = nowTime;

    _playWarning = 0;
    _playError = 0;
    _recWarning = 0;
    _recError = 0;

    _mixerManager.EnumerateAll();

    if (_ptrThread)
    {
        
        return 0;
    }

    const char* threadName = "webrtc_audio_module_thread";
    _ptrThread = ThreadWrapper::CreateThread(ThreadFunc, 
                                             this, 
                                             kRealtimePriority,
                                             threadName);
    if (_ptrThread == NULL)
    {
        WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                     "failed to create the audio thread");
        return -1;
    }

    unsigned int threadID(0);
    if (!_ptrThread->Start(threadID))
    {
        WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                     "failed to start the audio thread");
        delete _ptrThread;
        _ptrThread = NULL;
        return -1;
    }
    _threadID = threadID;

    const bool periodic(true);
    if (!_timeEvent.StartTimer(periodic, TIMER_PERIOD_MS))
    {
        WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                     "failed to start the timer event");
        if (_ptrThread->Stop())
        {
            delete _ptrThread;
            _ptrThread = NULL;
        }
        else
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "unable to stop the activated thread");
        }
        return -1;
    }
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "periodic timer (dT=%d) is now active", TIMER_PERIOD_MS);

    _hGetCaptureVolumeThread = CreateThread(NULL,
                                            0,
                                            GetCaptureVolumeThread,
                                            this,
                                            0,
                                            NULL);
    if (_hGetCaptureVolumeThread == NULL)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
            "  failed to create the volume getter thread");
        return -1;
    }

    SetThreadPriority(_hGetCaptureVolumeThread, THREAD_PRIORITY_NORMAL);

    _hSetCaptureVolumeThread = CreateThread(NULL,
                                            0,
                                            SetCaptureVolumeThread,
                                            this,
                                            0,
                                            NULL);
    if (_hSetCaptureVolumeThread == NULL)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
            "  failed to create the volume setter thread");
        return -1;
    }

    SetThreadPriority(_hSetCaptureVolumeThread, THREAD_PRIORITY_NORMAL);

    _initialized = true;

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::Terminate()
{

    if (!_initialized)
    {
        return 0;
    }

    _critSect.Enter();

    _mixerManager.Close();

    if (_ptrThread)
    {
        ThreadWrapper* tmpThread = _ptrThread;
        _ptrThread = NULL;
        _critSect.Leave();

        tmpThread->SetNotAlive();
        _timeEvent.Set();

        if (tmpThread->Stop())
        {
            delete tmpThread;
        }
        else
        {
            _critSect.Leave();
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "failed to close down the audio thread");
            return -1;
        }
    }
    else
    {
        _critSect.Leave();
    }

    _critSect.Enter();
    SetEvent(_hShutdownGetVolumeEvent);
    _critSect.Leave();
    WebRtc_Word32 ret = WaitForSingleObject(_hGetCaptureVolumeThread, 2000);
    if (ret != WAIT_OBJECT_0)
    {
        
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
            "  failed to close down volume getter thread");
        CloseHandle(_hGetCaptureVolumeThread);
        _hGetCaptureVolumeThread = NULL;
        return -1;
    }
    _critSect.Enter();
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, 
        "  volume getter thread is now closed");

    SetEvent(_hShutdownSetVolumeEvent);
    _critSect.Leave();
    ret = WaitForSingleObject(_hSetCaptureVolumeThread, 2000);
    if (ret != WAIT_OBJECT_0)
    {
        
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
            "  failed to close down volume setter thread");
        CloseHandle(_hSetCaptureVolumeThread);
        _hSetCaptureVolumeThread = NULL;
        return -1;
    }
    _critSect.Enter();
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
        "  volume setter thread is now closed");

    CloseHandle(_hGetCaptureVolumeThread);
    _hGetCaptureVolumeThread = NULL;

    CloseHandle(_hSetCaptureVolumeThread);
    _hSetCaptureVolumeThread = NULL;

    _critSect.Leave();

    _timeEvent.StopTimer();

    _initialized = false;
    _outputDeviceIsSpecified = false;
    _inputDeviceIsSpecified = false;

    return 0;
}


DWORD WINAPI AudioDeviceWindowsWave::GetCaptureVolumeThread(LPVOID context)
{
    return(((AudioDeviceWindowsWave*)context)->DoGetCaptureVolumeThread());
}

DWORD WINAPI AudioDeviceWindowsWave::SetCaptureVolumeThread(LPVOID context)
{
    return(((AudioDeviceWindowsWave*)context)->DoSetCaptureVolumeThread());
}

DWORD AudioDeviceWindowsWave::DoGetCaptureVolumeThread()
{
    HANDLE waitObject = _hShutdownGetVolumeEvent;

    while (1)
    {
        DWORD waitResult = WaitForSingleObject(waitObject, 
                                               GET_MIC_VOLUME_INTERVAL_MS);
        switch (waitResult)
        {
            case WAIT_OBJECT_0: 
                return 0;
            case WAIT_TIMEOUT:	
                break;
            default:            
                WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                    "  unknown wait termination on get volume thread");
                return -1;
        }

        if (AGC())
        {
            WebRtc_UWord32 currentMicLevel = 0;
            if (MicrophoneVolume(currentMicLevel) == 0)
            {
                
                _critSect.Enter();
                if (_ptrAudioBuffer)
                {
                    _ptrAudioBuffer->SetCurrentMicLevel(currentMicLevel);				
                }
                _critSect.Leave();
            }
        }
    }
}

DWORD AudioDeviceWindowsWave::DoSetCaptureVolumeThread()
{
    HANDLE waitArray[2] = {_hShutdownSetVolumeEvent, _hSetCaptureVolumeEvent};

    while (1)
    {
        DWORD waitResult = WaitForMultipleObjects(2, waitArray, FALSE, INFINITE);
        switch (waitResult)
        {
            case WAIT_OBJECT_0:     
                return 0;
            case WAIT_OBJECT_0 + 1: 
                break;
            default:                
                WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                    "  unknown wait termination on set volume thread");
                return -1;
        }

        _critSect.Enter();
        WebRtc_UWord32 newMicLevel = _newMicLevel;
        _critSect.Leave();

        if (SetMicrophoneVolume(newMicLevel) == -1)
        {   
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                "  the required modification of the microphone volume failed");
        }
    }      
    return 0;
}





bool AudioDeviceWindowsWave::Initialized() const
{
    return (_initialized);
}





WebRtc_Word32 AudioDeviceWindowsWave::SpeakerIsAvailable(bool& available)
{

    
    
    
    if (InitSpeaker() == -1)
    {
        available = false;
        return 0;
    }

    
    
    available = true;

    
    
    _mixerManager.CloseSpeaker();

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::InitSpeaker()
{

    CriticalSectionScoped lock(&_critSect);

    if (_playing)
    {
        return -1;
    }

    if (_mixerManager.EnumerateSpeakers() == -1)
    {
        
        return -1;
    }

    if (IsUsingOutputDeviceIndex())
    {
        if (_mixerManager.OpenSpeaker(OutputDeviceIndex()) == -1)
        {
            return -1;
        }
    }
    else
    {
        if (_mixerManager.OpenSpeaker(OutputDevice()) == -1)
        {
            return -1;
        }
    }

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::MicrophoneIsAvailable(bool& available)
{

    
    
    
    if (InitMicrophone() == -1)
    {
        available = false;
        return 0;
    }

    
    
    available = true;

    
    
    _mixerManager.CloseMicrophone();

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::InitMicrophone()
{

    CriticalSectionScoped lock(&_critSect);

    if (_recording)
    {
        return -1;
    }

    if (_mixerManager.EnumerateMicrophones() == -1)
    {
        
        return -1;
    }

    if (IsUsingInputDeviceIndex())
    {
        if (_mixerManager.OpenMicrophone(InputDeviceIndex()) == -1)
        {
            return -1;
        }
    }
    else
    {
        if (_mixerManager.OpenMicrophone(InputDevice()) == -1)
        {
            return -1;
        }
    }

    WebRtc_UWord32 maxVol = 0;
    if (_mixerManager.MaxMicrophoneVolume(maxVol) == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
            "  unable to retrieve max microphone volume");
    }
    _maxMicVolume = maxVol;

    WebRtc_UWord32 minVol = 0;
    if (_mixerManager.MinMicrophoneVolume(minVol) == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
            "  unable to retrieve min microphone volume");
    }
    _minMicVolume = minVol;

    return 0;
}





bool AudioDeviceWindowsWave::SpeakerIsInitialized() const
{
    return (_mixerManager.SpeakerIsInitialized());
}





bool AudioDeviceWindowsWave::MicrophoneIsInitialized() const
{
    return (_mixerManager.MicrophoneIsInitialized());
}





WebRtc_Word32 AudioDeviceWindowsWave::SpeakerVolumeIsAvailable(bool& available)
{

    bool isAvailable(false);

    
    
    
    if (InitSpeaker() == -1)
    {
        
        available = false;
        return 0;
    }

    
    
    _mixerManager.SpeakerVolumeIsAvailable(isAvailable);
    available = isAvailable;

    
    
    _mixerManager.CloseSpeaker();

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::SetSpeakerVolume(WebRtc_UWord32 volume)
{

    return (_mixerManager.SetSpeakerVolume(volume));
}





WebRtc_Word32 AudioDeviceWindowsWave::SpeakerVolume(WebRtc_UWord32& volume) const
{

    WebRtc_UWord32 level(0);

    if (_mixerManager.SpeakerVolume(level) == -1)
    {
        return -1;
    }

    volume = level;
    return 0;
}


















WebRtc_Word32 AudioDeviceWindowsWave::SetWaveOutVolume(WebRtc_UWord16 volumeLeft, WebRtc_UWord16 volumeRight)
{

    MMRESULT res(0);
    WAVEOUTCAPS caps;

    CriticalSectionScoped lock(&_critSect);

    if (_hWaveOut == NULL)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "no open playout device exists => using default");
    }

    
    
    
    res = waveOutGetDevCaps((UINT_PTR)_hWaveOut, &caps, sizeof(WAVEOUTCAPS));
    if (MMSYSERR_NOERROR != res)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "waveOutGetDevCaps() failed (err=%d)", res);
        TraceWaveOutError(res);
    }
    if (!(caps.dwSupport & WAVECAPS_VOLUME))
    {
        
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "device does not support volume control using the Wave API");
        return -1;
    }
    if (!(caps.dwSupport & WAVECAPS_LRVOLUME))
    {
        
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "device does not support volume control on both channels");
    }

    DWORD dwVolume(0x00000000);
    dwVolume = (DWORD)(((volumeRight & 0xFFFF) << 16) | (volumeLeft & 0xFFFF));

    res = waveOutSetVolume(_hWaveOut, dwVolume);
    if (MMSYSERR_NOERROR != res)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "waveOutSetVolume() failed (err=%d)", res);
        TraceWaveOutError(res);
        return -1;
    }

    return 0;
}

















WebRtc_Word32 AudioDeviceWindowsWave::WaveOutVolume(WebRtc_UWord16& volumeLeft, WebRtc_UWord16& volumeRight) const
{

    MMRESULT res(0);
    WAVEOUTCAPS caps;

    CriticalSectionScoped lock(&_critSect);

    if (_hWaveOut == NULL)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "no open playout device exists => using default");
    }

    
    
    
    res = waveOutGetDevCaps((UINT_PTR)_hWaveOut, &caps, sizeof(WAVEOUTCAPS));
    if (MMSYSERR_NOERROR != res)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "waveOutGetDevCaps() failed (err=%d)", res);
        TraceWaveOutError(res);
    }
    if (!(caps.dwSupport & WAVECAPS_VOLUME))
    {
        
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "device does not support volume control using the Wave API");
        return -1;
    }
    if (!(caps.dwSupport & WAVECAPS_LRVOLUME))
    {
        
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "device does not support volume control on both channels");
    }

    DWORD dwVolume(0x00000000);

    res = waveOutGetVolume(_hWaveOut, &dwVolume);
    if (MMSYSERR_NOERROR != res)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "waveOutGetVolume() failed (err=%d)", res);
        TraceWaveOutError(res);
        return -1;
    }

    WORD wVolumeLeft = LOWORD(dwVolume);
    WORD wVolumeRight = HIWORD(dwVolume);

    volumeLeft = static_cast<WebRtc_UWord16> (wVolumeLeft);
    volumeRight = static_cast<WebRtc_UWord16> (wVolumeRight);

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::MaxSpeakerVolume(WebRtc_UWord32& maxVolume) const
{

    WebRtc_UWord32 maxVol(0);

    if (_mixerManager.MaxSpeakerVolume(maxVol) == -1)
    {
        return -1;
    }

    maxVolume = maxVol;
    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::MinSpeakerVolume(WebRtc_UWord32& minVolume) const
{

    WebRtc_UWord32 minVol(0);

    if (_mixerManager.MinSpeakerVolume(minVol) == -1)
    {
        return -1;
    }

    minVolume = minVol;
    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::SpeakerVolumeStepSize(WebRtc_UWord16& stepSize) const
{

    WebRtc_UWord16 delta(0);

    if (_mixerManager.SpeakerVolumeStepSize(delta) == -1)
    {
        return -1;
    }

    stepSize = delta;
    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::SpeakerMuteIsAvailable(bool& available)
{

    bool isAvailable(false);

    
    
    
    if (InitSpeaker() == -1)
    {
        
        
        
        available = false;
        return 0;
    }

    
    
    _mixerManager.SpeakerMuteIsAvailable(isAvailable);
    available = isAvailable;

    
    
    _mixerManager.CloseSpeaker();

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::SetSpeakerMute(bool enable)
{
    return (_mixerManager.SetSpeakerMute(enable));
}





WebRtc_Word32 AudioDeviceWindowsWave::SpeakerMute(bool& enabled) const
{

    bool muted(0);

    if (_mixerManager.SpeakerMute(muted) == -1)
    {
        return -1;
    }

    enabled = muted;
    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::MicrophoneMuteIsAvailable(bool& available)
{

    bool isAvailable(false);

    
    
    
    if (InitMicrophone() == -1)
    {
        
        
        
        available = false;
        return 0;
    }

    
    
    _mixerManager.MicrophoneMuteIsAvailable(isAvailable);
    available = isAvailable;

    
    
    _mixerManager.CloseMicrophone();

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::SetMicrophoneMute(bool enable)
{
    return (_mixerManager.SetMicrophoneMute(enable));
}





WebRtc_Word32 AudioDeviceWindowsWave::MicrophoneMute(bool& enabled) const
{

    bool muted(0);

    if (_mixerManager.MicrophoneMute(muted) == -1)
    {
        return -1;
    }

    enabled = muted;
    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::MicrophoneBoostIsAvailable(bool& available)
{

    bool isAvailable(false);

    
    
    
    if (InitMicrophone() == -1)
    {
        
        
        
        available = false;
        return 0;
    }

    
    
    _mixerManager.MicrophoneBoostIsAvailable(isAvailable);
    available = isAvailable;

    
    
    _mixerManager.CloseMicrophone();

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::SetMicrophoneBoost(bool enable)
{

    return (_mixerManager.SetMicrophoneBoost(enable));
}





WebRtc_Word32 AudioDeviceWindowsWave::MicrophoneBoost(bool& enabled) const
{

    bool onOff(0);

    if (_mixerManager.MicrophoneBoost(onOff) == -1)
    {
        return -1;
    }

    enabled = onOff;
    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::StereoRecordingIsAvailable(bool& available)
{
    available = true;
    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::SetStereoRecording(bool enable)
{

    if (enable)
        _recChannels = 2;
    else
        _recChannels = 1;

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::StereoRecording(bool& enabled) const
{

    if (_recChannels == 2)
        enabled = true;
    else
        enabled = false;

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::StereoPlayoutIsAvailable(bool& available)
{
    available = true;
    return 0;
}
























WebRtc_Word32 AudioDeviceWindowsWave::SetStereoPlayout(bool enable)
{

    if (enable)
        _playChannels = 2;
    else
        _playChannels = 1;

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::StereoPlayout(bool& enabled) const
{

    if (_playChannels == 2)
        enabled = true;
    else
        enabled = false;

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::SetAGC(bool enable)
{

    _AGC = enable;

    return 0;
}





bool AudioDeviceWindowsWave::AGC() const
{
    return _AGC;
}





WebRtc_Word32 AudioDeviceWindowsWave::MicrophoneVolumeIsAvailable(bool& available)
{

    bool isAvailable(false);

    
    
    
    if (InitMicrophone() == -1)
    {
        
        available = false;
        return 0;
    }

    
    
    _mixerManager.MicrophoneVolumeIsAvailable(isAvailable);
    available = isAvailable;

    
    
    _mixerManager.CloseMicrophone();

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::SetMicrophoneVolume(WebRtc_UWord32 volume)
{
    return (_mixerManager.SetMicrophoneVolume(volume));
}





WebRtc_Word32 AudioDeviceWindowsWave::MicrophoneVolume(WebRtc_UWord32& volume) const
{
    WebRtc_UWord32 level(0);

    if (_mixerManager.MicrophoneVolume(level) == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "failed to retrive current microphone level");
        return -1;
    }

    volume = level;
    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::MaxMicrophoneVolume(WebRtc_UWord32& maxVolume) const
{
    
    
    
    
    
    if (_maxMicVolume == 0)
    {
        return -1;
    }

    maxVolume = _maxMicVolume;;
    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::MinMicrophoneVolume(WebRtc_UWord32& minVolume) const
{
    minVolume = _minMicVolume;
    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::MicrophoneVolumeStepSize(WebRtc_UWord16& stepSize) const
{

    WebRtc_UWord16 delta(0);

    if (_mixerManager.MicrophoneVolumeStepSize(delta) == -1)
    {
        return -1;
    }

    stepSize = delta;
    return 0;
}





WebRtc_Word16 AudioDeviceWindowsWave::PlayoutDevices()
{

    return (waveOutGetNumDevs());
}





WebRtc_Word32 AudioDeviceWindowsWave::SetPlayoutDevice(WebRtc_UWord16 index)
{

    if (_playIsInitialized)
    {
        return -1;
    }

    UINT nDevices = waveOutGetNumDevs();
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "number of availiable waveform-audio output devices is %u", nDevices);

    if (index < 0 || index > (nDevices-1))
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "device index is out of range [0,%u]", (nDevices-1));
        return -1;
    }

    _usingOutputDeviceIndex = true;
    _outputDeviceIndex = index;
    _outputDeviceIsSpecified = true;

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::SetPlayoutDevice(AudioDeviceModule::WindowsDeviceType device)
{
    if (_playIsInitialized)
    {
        return -1;
    }

    if (device == AudioDeviceModule::kDefaultDevice)
    {
    }
    else if (device == AudioDeviceModule::kDefaultCommunicationDevice)
    {
    }

    _usingOutputDeviceIndex = false;
    _outputDevice = device;
    _outputDeviceIsSpecified = true;

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::PlayoutDeviceName(
    WebRtc_UWord16 index,
    char name[kAdmMaxDeviceNameSize],
    char guid[kAdmMaxGuidSize])
{

    WebRtc_UWord16 nDevices(PlayoutDevices());

    
    
    if (index == (WebRtc_UWord16)(-1))
    {
        index = 0;
    }

    if ((index > (nDevices-1)) || (name == NULL))
    {
        return -1;
    }

    memset(name, 0, kAdmMaxDeviceNameSize);

    if (guid != NULL)
    {
        memset(guid, 0, kAdmMaxGuidSize);
    }

    WAVEOUTCAPSW caps;    
    MMRESULT res;

    res = waveOutGetDevCapsW(index, &caps, sizeof(WAVEOUTCAPSW));
    if (res != MMSYSERR_NOERROR)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "waveOutGetDevCapsW() failed (err=%d)", res);
        return -1;
    }
    if (WideCharToMultiByte(CP_UTF8, 0, caps.szPname, -1, name, kAdmMaxDeviceNameSize, NULL, NULL) == 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "WideCharToMultiByte(CP_UTF8) failed with error code %d - 1", GetLastError());
    }

    if (guid == NULL)
    {
        return 0;
    }

    
    

    size_t cbEndpointId(0);

    
    
    res = waveOutMessage((HWAVEOUT)IntToPtr(index),
                          DRV_QUERYFUNCTIONINSTANCEIDSIZE,
                         (DWORD_PTR)&cbEndpointId, NULL);
    if (res != MMSYSERR_NOERROR)
    {
        
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "waveOutMessage(DRV_QUERYFUNCTIONINSTANCEIDSIZE) failed (err=%d)", res);
        TraceWaveOutError(res);
        
        if (WideCharToMultiByte(CP_UTF8, 0, caps.szPname, -1, guid, kAdmMaxGuidSize, NULL, NULL) == 0)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "WideCharToMultiByte(CP_UTF8) failed with error code %d - 2", GetLastError());
        }
        return 0;
    }

    

    WCHAR *pstrEndpointId = NULL;
    pstrEndpointId = (WCHAR*)CoTaskMemAlloc(cbEndpointId);

    
    res = waveOutMessage((HWAVEOUT)IntToPtr(index),
                          DRV_QUERYFUNCTIONINSTANCEID,
                         (DWORD_PTR)pstrEndpointId,
                          cbEndpointId);
    if (res != MMSYSERR_NOERROR)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "waveOutMessage(DRV_QUERYFUNCTIONINSTANCEID) failed (err=%d)", res);
        TraceWaveOutError(res);
        
        if (WideCharToMultiByte(CP_UTF8, 0, caps.szPname, -1, guid, kAdmMaxGuidSize, NULL, NULL) == 0)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "WideCharToMultiByte(CP_UTF8) failed with error code %d - 3", GetLastError());
        }
        CoTaskMemFree(pstrEndpointId);
        return 0;
    }

    if (WideCharToMultiByte(CP_UTF8, 0, pstrEndpointId, -1, guid, kAdmMaxGuidSize, NULL, NULL) == 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "WideCharToMultiByte(CP_UTF8) failed with error code %d - 4", GetLastError());
    }
    CoTaskMemFree(pstrEndpointId);

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::RecordingDeviceName(
    WebRtc_UWord16 index,
    char name[kAdmMaxDeviceNameSize],
    char guid[kAdmMaxGuidSize])
{

    WebRtc_UWord16 nDevices(RecordingDevices());

    
    
    if (index == (WebRtc_UWord16)(-1))
    {
        index = 0;
    }

    if ((index > (nDevices-1)) || (name == NULL))
    {
        return -1;
    }

    memset(name, 0, kAdmMaxDeviceNameSize);

    if (guid != NULL)
    {
        memset(guid, 0, kAdmMaxGuidSize);
    }

    WAVEINCAPSW caps;    
    MMRESULT res;

    res = waveInGetDevCapsW(index, &caps, sizeof(WAVEINCAPSW));
    if (res != MMSYSERR_NOERROR)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "waveInGetDevCapsW() failed (err=%d)", res);
        return -1;
    }
    if (WideCharToMultiByte(CP_UTF8, 0, caps.szPname, -1, name, kAdmMaxDeviceNameSize, NULL, NULL) == 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "WideCharToMultiByte(CP_UTF8) failed with error code %d - 1", GetLastError());
    }

    if (guid == NULL)
    {
        return 0;
    }

    
    

    size_t cbEndpointId(0);

    
    
    res = waveInMessage((HWAVEIN)IntToPtr(index),
                         DRV_QUERYFUNCTIONINSTANCEIDSIZE,
                        (DWORD_PTR)&cbEndpointId, NULL);
    if (res != MMSYSERR_NOERROR)
    {
        
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "waveInMessage(DRV_QUERYFUNCTIONINSTANCEIDSIZE) failed (err=%d)", res);
        TraceWaveInError(res);
        
        if (WideCharToMultiByte(CP_UTF8, 0, caps.szPname, -1, guid, kAdmMaxGuidSize, NULL, NULL) == 0)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "WideCharToMultiByte(CP_UTF8) failed with error code %d - 2", GetLastError());
        }
        return 0;
    }

    

    WCHAR *pstrEndpointId = NULL;
    pstrEndpointId = (WCHAR*)CoTaskMemAlloc(cbEndpointId);

    
    res = waveInMessage((HWAVEIN)IntToPtr(index),
                          DRV_QUERYFUNCTIONINSTANCEID,
                         (DWORD_PTR)pstrEndpointId,
                          cbEndpointId);
    if (res != MMSYSERR_NOERROR)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "waveInMessage(DRV_QUERYFUNCTIONINSTANCEID) failed (err=%d)", res);
        TraceWaveInError(res);
        
        if (WideCharToMultiByte(CP_UTF8, 0, caps.szPname, -1, guid, kAdmMaxGuidSize, NULL, NULL) == 0)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "WideCharToMultiByte(CP_UTF8) failed with error code %d - 3", GetLastError());
        }
        CoTaskMemFree(pstrEndpointId);
        return 0;
    }

    if (WideCharToMultiByte(CP_UTF8, 0, pstrEndpointId, -1, guid, kAdmMaxGuidSize, NULL, NULL) == 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "WideCharToMultiByte(CP_UTF8) failed with error code %d - 4", GetLastError());
    }
    CoTaskMemFree(pstrEndpointId);

    return 0;
}





WebRtc_Word16 AudioDeviceWindowsWave::RecordingDevices()
{

    return (waveInGetNumDevs());
}





WebRtc_Word32 AudioDeviceWindowsWave::SetRecordingDevice(WebRtc_UWord16 index)
{

    if (_recIsInitialized)
    {
        return -1;
    }

    UINT nDevices = waveInGetNumDevs();
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "number of availiable waveform-audio input devices is %u", nDevices);

    if (index < 0 || index > (nDevices-1))
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "device index is out of range [0,%u]", (nDevices-1));
        return -1;
    }

    _usingInputDeviceIndex = true;
    _inputDeviceIndex = index;
    _inputDeviceIsSpecified = true;

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::SetRecordingDevice(AudioDeviceModule::WindowsDeviceType device)
{
    if (device == AudioDeviceModule::kDefaultDevice)
    {
    }
    else if (device == AudioDeviceModule::kDefaultCommunicationDevice)
    {
    }

    if (_recIsInitialized)
    {
        return -1;
    }

    _usingInputDeviceIndex = false;
    _inputDevice = device;
    _inputDeviceIsSpecified = true;

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::PlayoutIsAvailable(bool& available)
{

    available = false;

    
    WebRtc_Word32 res = InitPlayout();

    
    StopPlayout();

    if (res != -1)
    {
        available = true;
    }

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::RecordingIsAvailable(bool& available)
{

    available = false;

    
    WebRtc_Word32 res = InitRecording();

    
    StopRecording();

    if (res != -1)
    {
        available = true;
    }

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::InitPlayout()
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
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "InitSpeaker() failed");
    }

    
    EnumeratePlayoutDevices();

    
    
    MMRESULT res(MMSYSERR_ERROR);

    if (_hWaveOut != NULL)
    {
        res = waveOutClose(_hWaveOut);
        if (MMSYSERR_NOERROR != res)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "waveOutClose() failed (err=%d)", res);
            TraceWaveOutError(res);
        }
    }

    
    
    WAVEFORMATEX waveFormat;

    waveFormat.wFormatTag      = WAVE_FORMAT_PCM;
    waveFormat.nChannels       = _playChannels;  
    waveFormat.nSamplesPerSec  = N_PLAY_SAMPLES_PER_SEC;
    waveFormat.wBitsPerSample  = 16;
    waveFormat.nBlockAlign     = waveFormat.nChannels * (waveFormat.wBitsPerSample/8);
    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
    waveFormat.cbSize          = 0;

    
    
    HWAVEOUT hWaveOut(NULL);

    if (IsUsingOutputDeviceIndex())
    {
        
        res = waveOutOpen(NULL, _outputDeviceIndex, &waveFormat, 0, 0, CALLBACK_NULL | WAVE_FORMAT_QUERY);
        if (MMSYSERR_NOERROR == res)
        {
            
            res = waveOutOpen(&hWaveOut, _outputDeviceIndex, &waveFormat, 0, 0, CALLBACK_NULL);
            WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "opening output device corresponding to device ID %u", _outputDeviceIndex);
        }
    }
    else
    {
        if (_outputDevice == AudioDeviceModule::kDefaultCommunicationDevice)
        {
            
            res = waveOutOpen(NULL, WAVE_MAPPER, &waveFormat, 0, 0, CALLBACK_NULL | WAVE_MAPPED_DEFAULT_COMMUNICATION_DEVICE | WAVE_FORMAT_QUERY);
            if (MMSYSERR_NOERROR == res)
            {
                
                res = waveOutOpen(&hWaveOut, WAVE_MAPPER, &waveFormat, 0, 0, CALLBACK_NULL |  WAVE_MAPPED_DEFAULT_COMMUNICATION_DEVICE);
                WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "opening default communication device");
            }
            else
            {
                
                res = waveOutOpen(&hWaveOut, WAVE_MAPPER, &waveFormat, 0, 0, CALLBACK_NULL);
                WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "unable to open default communication device => using default instead");
            }
        }
        else if (_outputDevice == AudioDeviceModule::kDefaultDevice)
        {
            
            res = waveOutOpen(NULL, WAVE_MAPPER, &waveFormat, 0, 0, CALLBACK_NULL | WAVE_FORMAT_QUERY);
            if (MMSYSERR_NOERROR == res)
            {
                res = waveOutOpen(&hWaveOut, WAVE_MAPPER, &waveFormat, 0, 0, CALLBACK_NULL);
                WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "opening default output device");
            }
        }
    }

    if (MMSYSERR_NOERROR != res)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "waveOutOpen() failed (err=%d)", res);
        TraceWaveOutError(res);
        return -1;
    }

    
    
    WAVEOUTCAPS caps;

    res = waveOutGetDevCaps((UINT_PTR)hWaveOut, &caps, sizeof(WAVEOUTCAPS));
    if (res != MMSYSERR_NOERROR)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "waveOutGetDevCaps() failed (err=%d)", res);
        TraceWaveOutError(res);
    }

    UINT deviceID(0);
    res = waveOutGetID(hWaveOut, &deviceID);
    if (res != MMSYSERR_NOERROR)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "waveOutGetID() failed (err=%d)", res);
        TraceWaveOutError(res);
    }
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "utilized device ID : %u", deviceID);
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "product name       : %s", caps.szPname);

    
    _hWaveOut = hWaveOut;

    
    _waveFormatOut = waveFormat;

    
    
    const WebRtc_UWord8 bytesPerSample = 2*_playChannels;

    for (int n = 0; n < N_BUFFERS_OUT; n++)
    {
        
        _waveHeaderOut[n].lpData          = reinterpret_cast<LPSTR>(&_playBuffer[n]);
        _waveHeaderOut[n].dwBufferLength  = bytesPerSample*PLAY_BUF_SIZE_IN_SAMPLES;
        _waveHeaderOut[n].dwFlags         = 0;
        _waveHeaderOut[n].dwLoops         = 0;

        memset(_playBuffer[n], 0, bytesPerSample*PLAY_BUF_SIZE_IN_SAMPLES);

        
        
        
        
        res = waveOutPrepareHeader(_hWaveOut, &_waveHeaderOut[n], sizeof(WAVEHDR));
        if (MMSYSERR_NOERROR != res)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "waveOutPrepareHeader(%d) failed (err=%d)", n, res);
            TraceWaveOutError(res);
        }

        
        if (_waveHeaderOut[n].dwFlags != WHDR_PREPARED)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "waveOutPrepareHeader(%d) failed (dwFlags != WHDR_PREPARED)", n);
        }
    }

    
    _playIsInitialized = true;

    _dTcheckPlayBufDelay = 10;  
    _playBufCount = 0;          
    _playBufDelay = 80;         
    _minPlayBufDelay = 25;      
    _MAX_minBuffer = 65;        
    _intro = 1;                 
    _waitCounter = 1700;        
    _erZeroCounter = 0;         
    _useHeader = 0;             

    _writtenSamples = 0;
    _writtenSamplesOld = 0;
    _playedSamplesOld = 0;
    _sndCardPlayDelay = 0;
    _sndCardRecDelay = 0;

    WEBRTC_TRACE(kTraceInfo, kTraceUtility, _id,"initial playout status: _playBufDelay=%d, _minPlayBufDelay=%d",
        _playBufDelay, _minPlayBufDelay);

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::InitRecording()
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

    _avgCPULoad = 0;
    _playAcc  = 0;

    
    if (InitMicrophone() == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "InitMicrophone() failed");
    }

    
    EnumerateRecordingDevices();

    
    
    MMRESULT res(MMSYSERR_ERROR);

    if (_hWaveIn != NULL)
    {
        res = waveInClose(_hWaveIn);
        if (MMSYSERR_NOERROR != res)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "waveInClose() failed (err=%d)", res);
            TraceWaveInError(res);
        }
    }

    
    
    WAVEFORMATEX waveFormat;

    waveFormat.wFormatTag      = WAVE_FORMAT_PCM;
    waveFormat.nChannels       = _recChannels;  
    waveFormat.nSamplesPerSec  = N_REC_SAMPLES_PER_SEC;
    waveFormat.wBitsPerSample  = 16;
    waveFormat.nBlockAlign     = waveFormat.nChannels * (waveFormat.wBitsPerSample/8);
    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
    waveFormat.cbSize          = 0;

    
    
    HWAVEIN hWaveIn(NULL);

    if (IsUsingInputDeviceIndex())
    {
        
        res = waveInOpen(NULL, _inputDeviceIndex, &waveFormat, 0, 0, CALLBACK_NULL | WAVE_FORMAT_QUERY);
        if (MMSYSERR_NOERROR == res)
        {
            
            res = waveInOpen(&hWaveIn, _inputDeviceIndex, &waveFormat, 0, 0, CALLBACK_NULL);
            WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "opening input device corresponding to device ID %u", _inputDeviceIndex);
        }
    }
    else
    {
        if (_inputDevice == AudioDeviceModule::kDefaultCommunicationDevice)
        {
            
            res = waveInOpen(NULL, WAVE_MAPPER, &waveFormat, 0, 0, CALLBACK_NULL | WAVE_MAPPED_DEFAULT_COMMUNICATION_DEVICE | WAVE_FORMAT_QUERY);
            if (MMSYSERR_NOERROR == res)
            {
                
                res = waveInOpen(&hWaveIn, WAVE_MAPPER, &waveFormat, 0, 0, CALLBACK_NULL | WAVE_MAPPED_DEFAULT_COMMUNICATION_DEVICE);
                WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "opening default communication device");
            }
            else
            {
                
                res = waveInOpen(&hWaveIn, WAVE_MAPPER, &waveFormat, 0, 0, CALLBACK_NULL);
                WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "unable to open default communication device => using default instead");
            }
        }
        else if (_inputDevice == AudioDeviceModule::kDefaultDevice)
        {
            
            res = waveInOpen(NULL, WAVE_MAPPER, &waveFormat, 0, 0, CALLBACK_NULL | WAVE_FORMAT_QUERY);
            if (MMSYSERR_NOERROR == res)
            {
                res = waveInOpen(&hWaveIn, WAVE_MAPPER, &waveFormat, 0, 0, CALLBACK_NULL);
                WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "opening default input device");
            }
        }
    }

    if (MMSYSERR_NOERROR != res)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "waveInOpen() failed (err=%d)", res);
        TraceWaveInError(res);
        return -1;
    }

    
    
    WAVEINCAPS caps;

    res = waveInGetDevCaps((UINT_PTR)hWaveIn, &caps, sizeof(WAVEINCAPS));
    if (res != MMSYSERR_NOERROR)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "waveInGetDevCaps() failed (err=%d)", res);
        TraceWaveInError(res);
    }

    UINT deviceID(0);
    res = waveInGetID(hWaveIn, &deviceID);
    if (res != MMSYSERR_NOERROR)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "waveInGetID() failed (err=%d)", res);
        TraceWaveInError(res);
    }
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "utilized device ID : %u", deviceID);
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "product name       : %s", caps.szPname);

    
    _hWaveIn = hWaveIn;

    
    _waveFormatIn = waveFormat;

    
    _recIsInitialized = true;

    _recBufCount = 0;     
    _recDelayCount = 0;   

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::StartRecording()
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

    
    if (kEventTimeout == _recStartEvent.Wait(10000))
    {
        _startRec = false;
        StopRecording();
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "failed to activate recording");
        return -1;
    }

    if (_recording)
    {
        
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "failed to activate recording");
        return -1;
    }

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::StopRecording()
{

    CriticalSectionScoped lock(&_critSect);

    if (!_recIsInitialized)
    {
        return 0;
    }

    if (_hWaveIn == NULL)
    {
        return -1;
    }

    bool wasRecording = _recording;
    _recIsInitialized = false;
    _recording = false;

    MMRESULT res;

    
    
    
    
    
    res = waveInStop(_hWaveIn);
    if (MMSYSERR_NOERROR != res)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "waveInStop() failed (err=%d)", res);
        TraceWaveInError(res);
    }

    
    
    
    
    res = waveInReset(_hWaveIn);
    if (MMSYSERR_NOERROR != res)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "waveInReset() failed (err=%d)", res);
        TraceWaveInError(res);
    }

    
    
    
    if (wasRecording)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "waveInUnprepareHeader() will be performed");
        for (int n = 0; n < N_BUFFERS_IN; n++)
        {
            res = waveInUnprepareHeader(_hWaveIn, &_waveHeaderIn[n], sizeof(WAVEHDR));
            if (MMSYSERR_NOERROR != res)
            {
                WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "waveInUnprepareHeader() failed (err=%d)", res);
                TraceWaveInError(res);
            }
        }
    }

    
    
    res = waveInClose(_hWaveIn);
    if (MMSYSERR_NOERROR != res)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "waveInClose() failed (err=%d)", res);
        TraceWaveInError(res);
    }

    
    
    _hWaveIn = NULL;
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "_hWaveIn is now set to NULL");

    return 0;
}





bool AudioDeviceWindowsWave::RecordingIsInitialized() const
{
    return (_recIsInitialized);
}





bool AudioDeviceWindowsWave::Recording() const
{
    return (_recording);
}





bool AudioDeviceWindowsWave::PlayoutIsInitialized() const
{
    return (_playIsInitialized);
}





WebRtc_Word32 AudioDeviceWindowsWave::StartPlayout()
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

    
    if (kEventTimeout == _playStartEvent.Wait(10000))
    {
        _startPlay = false;
        StopPlayout();
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "failed to activate playout");
        return -1;
    }

    if (_playing)
    {
        
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "failed to activate playing");
        return -1;
    }

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::StopPlayout()
{

    CriticalSectionScoped lock(&_critSect);

    if (!_playIsInitialized)
    {
        return 0;
    }

    if (_hWaveOut == NULL)
    {
        return -1;
    }

    _playIsInitialized = false;
    _playing = false;
    _sndCardPlayDelay = 0;
    _sndCardRecDelay = 0;

    MMRESULT res;

    
    
    
    
    
    
    res = waveOutReset(_hWaveOut);
    if (MMSYSERR_NOERROR != res)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "waveOutReset() failed (err=%d)", res);
        TraceWaveOutError(res);
    }

    
    
    
    
    
    for (int n = 0; n < N_BUFFERS_OUT; n++)
    {
        res = waveOutUnprepareHeader(_hWaveOut, &_waveHeaderOut[n], sizeof(WAVEHDR));
        if (MMSYSERR_NOERROR != res)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "waveOutUnprepareHeader() failed (err=%d)", res);
            TraceWaveOutError(res);
        }
    }

    
    
    
    
    
    
    res = waveOutClose(_hWaveOut);
    if (MMSYSERR_NOERROR != res)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "waveOutClose() failed (err=%d)", res);
        TraceWaveOutError(res);
    }

    _hWaveOut = NULL;
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "_hWaveOut is now set to NULL");

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::PlayoutDelay(WebRtc_UWord16& delayMS) const
{
    CriticalSectionScoped lock(&_critSect);
    delayMS = (WebRtc_UWord16)_sndCardPlayDelay;
    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::RecordingDelay(WebRtc_UWord16& delayMS) const
{
    CriticalSectionScoped lock(&_critSect);
    delayMS = (WebRtc_UWord16)_sndCardRecDelay;
    return 0;
}





bool AudioDeviceWindowsWave::Playing() const
{
    return (_playing);
}




WebRtc_Word32 AudioDeviceWindowsWave::SetPlayoutBuffer(const AudioDeviceModule::BufferType type, WebRtc_UWord16 sizeMS)
{
    CriticalSectionScoped lock(&_critSect);
    _playBufType = type;
    if (type == AudioDeviceModule::kFixedBufferSize)
    {
        _playBufDelayFixed = sizeMS;
    }
    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::PlayoutBuffer(AudioDeviceModule::BufferType& type, WebRtc_UWord16& sizeMS) const
{
    CriticalSectionScoped lock(&_critSect);
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





WebRtc_Word32 AudioDeviceWindowsWave::CPULoad(WebRtc_UWord16& load) const
{

    load = static_cast<WebRtc_UWord16>(100*_avgCPULoad);

    return 0;
}





bool AudioDeviceWindowsWave::PlayoutWarning() const
{
    return ( _playWarning > 0);
}





bool AudioDeviceWindowsWave::PlayoutError() const
{
    return ( _playError > 0);
}





bool AudioDeviceWindowsWave::RecordingWarning() const
{
    return ( _recWarning > 0);
}





bool AudioDeviceWindowsWave::RecordingError() const
{
    return ( _recError > 0);
}





void AudioDeviceWindowsWave::ClearPlayoutWarning()
{
    _playWarning = 0;
}





void AudioDeviceWindowsWave::ClearPlayoutError()
{
    _playError = 0;
}





void AudioDeviceWindowsWave::ClearRecordingWarning()
{
    _recWarning = 0;
}





void AudioDeviceWindowsWave::ClearRecordingError()
{
    _recError = 0;
}









WebRtc_Word32 AudioDeviceWindowsWave::InputSanityCheckAfterUnlockedPeriod() const
{
    if (_hWaveIn == NULL)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "input state has been modified during unlocked period");
        return -1;
    }
    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::OutputSanityCheckAfterUnlockedPeriod() const
{
    if (_hWaveOut == NULL)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "output state has been modified during unlocked period");
        return -1;
    }
    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::EnumeratePlayoutDevices()
{

    WebRtc_UWord16 nDevices(PlayoutDevices());
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "===============================================================");
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "#output devices: %u", nDevices);

    WAVEOUTCAPS caps;
    MMRESULT res;

    for (UINT deviceID = 0; deviceID < nDevices; deviceID++)
    {
        res = waveOutGetDevCaps(deviceID, &caps, sizeof(WAVEOUTCAPS));
        if (res != MMSYSERR_NOERROR)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "waveOutGetDevCaps() failed (err=%d)", res);
        }

        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "===============================================================");
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "Device ID %u:", deviceID);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "manufacturer ID      : %u", caps.wMid);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "product ID           : %u",caps.wPid);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "version of driver    : %u.%u", HIBYTE(caps.vDriverVersion), LOBYTE(caps.vDriverVersion));
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "product name         : %s", caps.szPname);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "dwFormats            : 0x%x", caps.dwFormats);
        if (caps.dwFormats & WAVE_FORMAT_48S16)
        {
            WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "  48kHz,stereo,16bit : SUPPORTED");
        }
        else
        {
                WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, " 48kHz,stereo,16bit  : *NOT* SUPPORTED");
        }
        if (caps.dwFormats & WAVE_FORMAT_48M16)
        {
            WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "  48kHz,mono,16bit   : SUPPORTED");
        }
        else
        {
                WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, " 48kHz,mono,16bit    : *NOT* SUPPORTED");
        }
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "wChannels            : %u", caps.wChannels);
        TraceSupportFlags(caps.dwSupport);
    }

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::EnumerateRecordingDevices()
{

    WebRtc_UWord16 nDevices(RecordingDevices());
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "===============================================================");
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "#input devices: %u", nDevices);

    WAVEINCAPS caps;
    MMRESULT res;

    for (UINT deviceID = 0; deviceID < nDevices; deviceID++)
    {
        res = waveInGetDevCaps(deviceID, &caps, sizeof(WAVEINCAPS));
        if (res != MMSYSERR_NOERROR)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "waveInGetDevCaps() failed (err=%d)", res);
        }

        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "===============================================================");
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "Device ID %u:", deviceID);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "manufacturer ID      : %u", caps.wMid);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "product ID           : %u",caps.wPid);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "version of driver    : %u.%u", HIBYTE(caps.vDriverVersion), LOBYTE(caps.vDriverVersion));
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "product name         : %s", caps.szPname);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "dwFormats            : 0x%x", caps.dwFormats);
        if (caps.dwFormats & WAVE_FORMAT_48S16)
        {
            WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "  48kHz,stereo,16bit : SUPPORTED");
        }
        else
        {
                WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, " 48kHz,stereo,16bit  : *NOT* SUPPORTED");
        }
        if (caps.dwFormats & WAVE_FORMAT_48M16)
        {
            WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "  48kHz,mono,16bit   : SUPPORTED");
        }
        else
        {
                WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, " 48kHz,mono,16bit    : *NOT* SUPPORTED");
        }
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "wChannels            : %u", caps.wChannels);
    }

    return 0;
}





void AudioDeviceWindowsWave::TraceSupportFlags(DWORD dwSupport) const
{
    TCHAR buf[256];

    StringCchPrintf(buf, 128, TEXT("support flags        : 0x%x "), dwSupport);

    if (dwSupport & WAVECAPS_PITCH)
    {
        
        StringCchCat(buf, 256, TEXT("(PITCH)"));
    }
    if (dwSupport & WAVECAPS_PLAYBACKRATE)
    {
        
        StringCchCat(buf, 256, TEXT("(PLAYBACKRATE)"));
    }
    if (dwSupport & WAVECAPS_VOLUME)
    {
        
        StringCchCat(buf, 256, TEXT("(VOLUME)"));
    }
    if (dwSupport & WAVECAPS_LRVOLUME)
    {
        
        StringCchCat(buf, 256, TEXT("(LRVOLUME)"));
    }
    if (dwSupport & WAVECAPS_SYNC)
    {
        
        StringCchCat(buf, 256, TEXT("(SYNC)"));
    }
    if (dwSupport & WAVECAPS_SAMPLEACCURATE)
    {
        
        StringCchCat(buf, 256, TEXT("(SAMPLEACCURATE)"));
    }

    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "%S", buf);
}





void AudioDeviceWindowsWave::TraceWaveInError(MMRESULT error) const
{
    TCHAR buf[MAXERRORLENGTH];
    TCHAR msg[MAXERRORLENGTH];

    StringCchPrintf(buf, MAXERRORLENGTH, TEXT("Error details: "));
    waveInGetErrorText(error, msg, MAXERRORLENGTH);
    StringCchCat(buf, MAXERRORLENGTH, msg);
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "%S", buf);
}





void AudioDeviceWindowsWave::TraceWaveOutError(MMRESULT error) const
{
    TCHAR buf[MAXERRORLENGTH];
    TCHAR msg[MAXERRORLENGTH];

    StringCchPrintf(buf, MAXERRORLENGTH, TEXT("Error details: "));
    waveOutGetErrorText(error, msg, MAXERRORLENGTH);
    StringCchCat(buf, MAXERRORLENGTH, msg);
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "%S", buf);
}





WebRtc_Word32 AudioDeviceWindowsWave::PrepareStartPlayout()
{

    CriticalSectionScoped lock(&_critSect);

    if (_hWaveOut == NULL)
    {
        return -1;
    }

    
    
    int8_t zeroVec[4*PLAY_BUF_SIZE_IN_SAMPLES];  
    memset(zeroVec, 0, 4*PLAY_BUF_SIZE_IN_SAMPLES);

    {
        Write(zeroVec, PLAY_BUF_SIZE_IN_SAMPLES);
        Write(zeroVec, PLAY_BUF_SIZE_IN_SAMPLES);
        Write(zeroVec, PLAY_BUF_SIZE_IN_SAMPLES);
    }

    _playAcc = 0;
    _playWarning = 0;
    _playError = 0;
    _dc_diff_mean = 0;
    _dc_y_prev = 0;
    _dc_penalty_counter = 20;
    _dc_prevtime = 0;
    _dc_prevplay = 0;

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::PrepareStartRecording()
{

    CriticalSectionScoped lock(&_critSect);

    if (_hWaveIn == NULL)
    {
        return -1;
    }

    _playAcc = 0;
    _recordedBytes = 0;
    _recPutBackDelay = REC_PUT_BACK_DELAY;

    MMRESULT res;
    MMTIME mmtime;
    mmtime.wType = TIME_SAMPLES;

    res = waveInGetPosition(_hWaveIn, &mmtime, sizeof(mmtime));
    if (MMSYSERR_NOERROR != res)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "waveInGetPosition(TIME_SAMPLES) failed (err=%d)", res);
        TraceWaveInError(res);
    }

    _read_samples = mmtime.u.sample;
    _read_samples_old = _read_samples;
    _rec_samples_old = mmtime.u.sample;
    _wrapCounter = 0;

    for (int n = 0; n < N_BUFFERS_IN; n++)
    {
        const WebRtc_UWord8 nBytesPerSample = 2*_recChannels;

        
        _waveHeaderIn[n].lpData          = reinterpret_cast<LPSTR>(&_recBuffer[n]);
        _waveHeaderIn[n].dwBufferLength  = nBytesPerSample * REC_BUF_SIZE_IN_SAMPLES;
        _waveHeaderIn[n].dwFlags         = 0;
        _waveHeaderIn[n].dwBytesRecorded = 0;
        _waveHeaderIn[n].dwUser          = 0;

        memset(_recBuffer[n], 0, nBytesPerSample * REC_BUF_SIZE_IN_SAMPLES);

        
        res = waveInPrepareHeader(_hWaveIn, &_waveHeaderIn[n], sizeof(WAVEHDR));
        if (MMSYSERR_NOERROR != res)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "waveInPrepareHeader(%d) failed (err=%d)", n, res);
            TraceWaveInError(res);
        }

        
        res = waveInAddBuffer(_hWaveIn, &_waveHeaderIn[n], sizeof(WAVEHDR));
        if (MMSYSERR_NOERROR != res)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "waveInAddBuffer(%d) failed (err=%d)", n, res);
            TraceWaveInError(res);
        }
    }

    
    res = waveInStart(_hWaveIn);
    if (MMSYSERR_NOERROR != res)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "waveInStart() failed (err=%d)", res);
        TraceWaveInError(res);
    }

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::GetPlayoutBufferDelay(WebRtc_UWord32& writtenSamples, WebRtc_UWord32& playedSamples)
{
    int i;
    int ms_Header;
    long playedDifference;
    int msecInPlayoutBuffer(0);   

    const WebRtc_UWord16 nSamplesPerMs = (WebRtc_UWord16)(N_PLAY_SAMPLES_PER_SEC/1000);  

    MMRESULT res;
    MMTIME mmtime;

    if (!_playing)
    {
        playedSamples = 0;
        return (0);
    }

    
    
    mmtime.wType = TIME_SAMPLES;  
    res = waveOutGetPosition(_hWaveOut, &mmtime, sizeof(mmtime));
    if (MMSYSERR_NOERROR != res)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "waveOutGetPosition() failed (err=%d)", res);
        TraceWaveOutError(res);
    }

    writtenSamples = _writtenSamples;   
    playedSamples = mmtime.u.sample;    

    
    msecInPlayoutBuffer = ((writtenSamples - playedSamples)/nSamplesPerMs);
    

    playedDifference = (long) (_playedSamplesOld - playedSamples);

    if (playedDifference > 64000)
    {
        
        
        
        
        
        
        
        

        i = 31;
        while((_playedSamplesOld <= (unsigned long)POW2(i)) && (i > 14)) {
            i--;
        }

        if((i < 31) && (i > 14)) {
            
            
            
            WEBRTC_TRACE(kTraceDebug, kTraceUtility, _id, "msecleft() => wrap around occured: %d bits used by sound card)", (i+1));

            _writtenSamples = _writtenSamples - POW2(i + 1);
            writtenSamples = _writtenSamples;
            msecInPlayoutBuffer = ((writtenSamples - playedSamples)/nSamplesPerMs);
        }
    }
    else if ((_writtenSamplesOld > POW2(31)) && (writtenSamples < 96000))
    {
        
        

        i = 31;
        while (_writtenSamplesOld <= (unsigned long)POW2(i)) {
            i--;
        }

        WEBRTC_TRACE(kTraceDebug, kTraceUtility, _id, "  msecleft() (wrap around occured after having used all 32 bits)");

        _writtenSamplesOld = writtenSamples;
        _playedSamplesOld = playedSamples;
        msecInPlayoutBuffer = (int)((writtenSamples + POW2(i + 1) - playedSamples)/nSamplesPerMs);

    }
    else if ((writtenSamples < 96000) && (playedSamples > POW2(31)))
    {
        
        
        

        WEBRTC_TRACE(kTraceDebug, kTraceUtility, _id, "  msecleft() (wrap around occured: correction of output is done)");

        _writtenSamplesOld = writtenSamples;
        _playedSamplesOld = playedSamples;
        msecInPlayoutBuffer = (int)((writtenSamples + POW2(32) - playedSamples)/nSamplesPerMs);
    }

    _writtenSamplesOld = writtenSamples;
    _playedSamplesOld = playedSamples;


    
    
    
    
    
    
    

    int diff,y;
    int unsigned time =0;

    
    
    
    

    time = timeGetTime();

    if ((msecInPlayoutBuffer < 20) || (time - _dc_prevtime > 40))
    {
        _dc_penalty_counter = 100;
    }

    if ((playedSamples != 0))
    {
        y = playedSamples/48 - time;
        if ((_dc_y_prev != 0) && (_dc_penalty_counter == 0))
        {
            diff = y - _dc_y_prev;
            _dc_diff_mean = (990*_dc_diff_mean)/1000 + 10*diff;
        }
        _dc_y_prev = y;
    }

    if (_dc_penalty_counter)
    {
        _dc_penalty_counter--;
    }

    if (_dc_diff_mean < -200)
    {
        
        _dc_diff_mean = 0;

        
        
        
        

        _useHeader++;
        if (_useHeader == 1)
        {
            _minPlayBufDelay = 80;
            _playWarning = 1;   
            WEBRTC_TRACE(kTraceInfo, kTraceUtility, -1, "Modification #1: _useHeader = %d, _minPlayBufDelay = %d", _useHeader, _minPlayBufDelay);
        }
        else if (_useHeader == 2)
        {
            _minPlayBufDelay = 100;   
            WEBRTC_TRACE(kTraceInfo, kTraceUtility, -1, "Modification #2: _useHeader = %d, _minPlayBufDelay = %d", _useHeader, _minPlayBufDelay);
        }
        else
        {
            
            WEBRTC_TRACE (kTraceWarning, kTraceUtility, -1, "further actions are required!");
        }
        if (_playWarning == 1)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceUtility, _id, "pending playout warning exists");
        }
        _playWarning = 1;  
        WEBRTC_TRACE(kTraceWarning, kTraceUtility, _id, "kPlayoutWarning message posted: switching to alternative playout delay method");
    }
    _dc_prevtime = time;
    _dc_prevplay = playedSamples;

    
    ms_Header = 0;
    for (i = 0; i < N_BUFFERS_OUT; i++) {
        if ((_waveHeaderOut[i].dwFlags & WHDR_INQUEUE)!=0) {
            ms_Header += 10;
        }
    }

    if ((ms_Header-50) > msecInPlayoutBuffer) {
        
        TCHAR infoStr[300];
        if (_no_of_msecleft_warnings%20==0)
        {
            StringCchPrintf(infoStr, 300, TEXT("writtenSamples=%i, playedSamples=%i, msecInPlayoutBuffer=%i, ms_Header=%i"), writtenSamples, playedSamples, msecInPlayoutBuffer, ms_Header);
            WEBRTC_TRACE(kTraceWarning, kTraceUtility, _id, "%S", infoStr);
        }
        _no_of_msecleft_warnings++;
    }

    
    if (_useHeader > 0)
    {
        return (ms_Header);
    }


    if (ms_Header < msecInPlayoutBuffer)
    {
        if (_no_of_msecleft_warnings % 100 == 0)
        {
            TCHAR str[300];
            StringCchPrintf(str, 300, TEXT("_no_of_msecleft_warnings=%i, msecInPlayoutBuffer=%i ms_Header=%i (minBuffer=%i buffersize=%i writtenSamples=%i playedSamples=%i)"),
                _no_of_msecleft_warnings, msecInPlayoutBuffer, ms_Header, _minPlayBufDelay, _playBufDelay, writtenSamples, playedSamples);
            WEBRTC_TRACE(kTraceWarning, kTraceUtility, _id, "%S", str);
        }
        _no_of_msecleft_warnings++;
        ms_Header -= 6; 

        if (ms_Header < 0)
            ms_Header = 0;

        return (ms_Header);
    }
    else
    {
        return (msecInPlayoutBuffer);
    }
}





WebRtc_Word32 AudioDeviceWindowsWave::GetRecordingBufferDelay(WebRtc_UWord32& readSamples, WebRtc_UWord32& recSamples)
{
    long recDifference;
    MMTIME mmtime;
    MMRESULT mmr;

    const WebRtc_UWord16 nSamplesPerMs = (WebRtc_UWord16)(N_REC_SAMPLES_PER_SEC/1000);  

    
    
    mmtime.wType = TIME_SAMPLES;
    mmr = waveInGetPosition(_hWaveIn, &mmtime, sizeof(mmtime));
    if (MMSYSERR_NOERROR != mmr)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "waveInGetPosition() failed (err=%d)", mmr);
        TraceWaveInError(mmr);
    }

    readSamples = _read_samples;    
    recSamples = mmtime.u.sample;   

    recDifference = (long) (_rec_samples_old - recSamples);

    if( recDifference > 64000) {
        WEBRTC_TRACE (kTraceDebug, kTraceUtility, -1,"WRAP 1 (recDifference =%d)", recDifference);
        
        
        
        
        
        
        
        
        
        int i = 31;
        while((_rec_samples_old <= (unsigned long)POW2(i)) && (i > 14))
            i--;

        if((i < 31) && (i > 14)) {
            
            
            
            _read_samples = _read_samples - POW2(i + 1);
            readSamples = _read_samples;
            _wrapCounter++;
        } else {
            WEBRTC_TRACE (kTraceWarning, kTraceUtility, -1,"AEC (_rec_samples_old %d recSamples %d)",_rec_samples_old, recSamples);
        }
    }

    if((_wrapCounter>200)){
        
    }
    else if((_rec_samples_old > POW2(31)) && (recSamples < 96000)) {
        WEBRTC_TRACE (kTraceDebug, kTraceUtility, -1,"WRAP 2 (_rec_samples_old %d recSamples %d)",_rec_samples_old, recSamples);
        
        _read_samples_old = readSamples;
        _rec_samples_old = recSamples;
        _wrapCounter++;
        return (int)((recSamples + POW2(32) - readSamples)/nSamplesPerMs);


    } else if((recSamples < 96000) && (readSamples > POW2(31))) {
        WEBRTC_TRACE (kTraceDebug, kTraceUtility, -1,"WRAP 3 (readSamples %d recSamples %d)",readSamples, recSamples);
        
        
        
        _read_samples_old = readSamples;
        _rec_samples_old = recSamples;
        _wrapCounter++;
        return (int)((recSamples + POW2(32) - readSamples)/nSamplesPerMs);
    }

    _read_samples_old = _read_samples;
    _rec_samples_old = recSamples;
    int res=(((int)_rec_samples_old - (int)_read_samples_old)/nSamplesPerMs);

    if((res > 2000)||(res < 0)||(_wrapCounter>200)){
        
        WEBRTC_TRACE (kTraceWarning, kTraceUtility, -1,"msec_read error (res %d wrapCounter %d)",res, _wrapCounter);
        MMTIME mmtime;
        mmtime.wType = TIME_SAMPLES;

        mmr=waveInGetPosition(_hWaveIn, &mmtime, sizeof(mmtime));
        if (mmr != MMSYSERR_NOERROR) {
            WEBRTC_TRACE (kTraceWarning, kTraceUtility, -1, "waveInGetPosition failed (mmr=%d)", mmr);
        }
        _read_samples=mmtime.u.sample;
        _read_samples_old=_read_samples;
        _rec_samples_old=mmtime.u.sample;

        
        res = 20;
    }

    _wrapCounter = 0;
    return res;
}









bool AudioDeviceWindowsWave::ThreadFunc(void* pThis)
{
    return (static_cast<AudioDeviceWindowsWave*>(pThis)->ThreadProcess());
}





bool AudioDeviceWindowsWave::ThreadProcess()
{
    WebRtc_UWord32 time(0);
    WebRtc_UWord32 playDiff(0);
    WebRtc_UWord32 recDiff(0);

    LONGLONG playTime(0);
    LONGLONG recTime(0);

    switch (_timeEvent.Wait(1000))
    {
    case kEventSignaled:
        break;
    case kEventError:
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "EventWrapper::Wait() failed => restarting timer");
        _timeEvent.StopTimer();
        _timeEvent.StartTimer(true, TIMER_PERIOD_MS);
        return true;
    case kEventTimeout:
        return true;
    }

    time = AudioDeviceUtility::GetTimeInMS();

    if (_startPlay)
    {
        if (PrepareStartPlayout() == 0)
        {
            _prevTimerCheckTime = time;
            _prevPlayTime = time;
            _startPlay = false;
            _playing = true;
            _playStartEvent.Set();
        }
    }

    if (_startRec)
    {
        if (PrepareStartRecording() == 0)
        {
            _prevTimerCheckTime = time;
            _prevRecTime = time;
            _prevRecByteCheckTime = time;
            _startRec = false;
            _recording = true;
            _recStartEvent.Set();
        }
    }

    if (_playing)
    {
        playDiff = time - _prevPlayTime;
    }

    if (_recording)
    {
        recDiff = time - _prevRecTime;
    }

    if (_playing || _recording)
    {
        RestartTimerIfNeeded(time);
    }

    if (_playing &&
        (playDiff > (WebRtc_UWord32)(_dTcheckPlayBufDelay - 1)) ||
        (playDiff < 0))
    {
        Lock();
        if (_playing)
        {
            if (PlayProc(playTime) == -1)
            {
                WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "PlayProc() failed");
            }
            _prevPlayTime = time;
            if (playTime != 0)
                _playAcc += playTime;
        }
        UnLock();
    }

    if (_playing && (playDiff > 12))
    {
        
        
        
        Lock();
        if (_playing)
        {
            if (PlayProc(playTime))
            {
                WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "PlayProc() failed");
            }
            _prevPlayTime = time;
            if (playTime != 0)
                _playAcc += playTime;
        }
        UnLock();
    }

    if (_recording &&
       (recDiff > REC_CHECK_TIME_PERIOD_MS) ||
       (recDiff < 0))
    {
        Lock();
        if (_recording)
        {
            WebRtc_Word32 nRecordedBytes(0);
            WebRtc_UWord16 maxIter(10);

            
            
            
            
            
            while ((nRecordedBytes = RecProc(recTime)) > 0)
            {
                maxIter--;
                _recordedBytes += nRecordedBytes;
                if (recTime && _perfFreq.QuadPart)
                {
                    
                    
                    
                    
                    
                    
                    
                    
                    _avgCPULoad = (float)(_avgCPULoad*.99 + (recTime+_playAcc)/(double)(_perfFreq.QuadPart));
                    _playAcc = 0;
                }
                if (maxIter == 0)
                {
                    
                    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "failed to compensate for reduced MM-timer resolution");
                }
            }

            if (nRecordedBytes == -1)
            {
                WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "RecProc() failed");
            }

            _prevRecTime = time;

            
            MonitorRecording(time);
        }
        UnLock();
    }

    if (!_recording)
    {
        _prevRecByteCheckTime = time;
        _avgCPULoad = 0;
    }

    return true;
}





WebRtc_Word32 AudioDeviceWindowsWave::RecProc(LONGLONG& consumedTime)
{
    MMRESULT res;
    WebRtc_UWord32 bufCount(0);
    WebRtc_UWord32 nBytesRecorded(0);

    consumedTime = 0;

    
    if (_recBufCount == N_BUFFERS_IN)
    {
        _recBufCount = 0;
    }

    bufCount = _recBufCount;

    
    const WebRtc_UWord16 bytesPerSample = 2*_recChannels;
    const WebRtc_UWord32 fullBufferSizeInBytes = bytesPerSample * REC_BUF_SIZE_IN_SAMPLES;

    
    nBytesRecorded = _waveHeaderIn[bufCount].dwBytesRecorded;

    if (nBytesRecorded == fullBufferSizeInBytes ||
       (nBytesRecorded > 0))
    {
        WebRtc_Word32 msecOnPlaySide;
        WebRtc_Word32 msecOnRecordSide;
        WebRtc_UWord32 writtenSamples;
        WebRtc_UWord32 playedSamples;
        WebRtc_UWord32 readSamples, recSamples;
        bool send = true;

        WebRtc_UWord32 nSamplesRecorded = (nBytesRecorded/bytesPerSample);  

        if (nBytesRecorded == fullBufferSizeInBytes)
        {
            _timesdwBytes = 0;
        }
        else
        {
            
            _timesdwBytes++;
            if (_timesdwBytes < 5)
            {
                
                return (0);
            }
            else
            {
                WEBRTC_TRACE(kTraceDebug, kTraceUtility, _id,"nBytesRecorded=%d => don't use", nBytesRecorded);
                _timesdwBytes = 0;
                send = false;
            }
        }

        
        _ptrAudioBuffer->SetRecordedBuffer(_waveHeaderIn[bufCount].lpData, nSamplesRecorded);

        
        _read_samples += nSamplesRecorded;

        
        
        
        msecOnPlaySide = GetPlayoutBufferDelay(writtenSamples, playedSamples);
        msecOnRecordSide = GetRecordingBufferDelay(readSamples, recSamples);

        
        
        WebRtc_Word32 drift = (_useHeader > 0) ? 0 : GetClockDrift(playedSamples, recSamples);

        _ptrAudioBuffer->SetVQEData(msecOnPlaySide, msecOnRecordSide, drift);

        
        _sndCardPlayDelay = msecOnPlaySide;
        _sndCardRecDelay = msecOnRecordSide;

        LARGE_INTEGER t1,t2;

        if (send)
        {
            QueryPerformanceCounter(&t1);

            
            UnLock();
            _ptrAudioBuffer->DeliverRecordedData();
            Lock();

            QueryPerformanceCounter(&t2);

            if (InputSanityCheckAfterUnlockedPeriod() == -1)
            {
                
                return -1;
            }
        }

        if (_AGC)
        {
            WebRtc_UWord32  newMicLevel = _ptrAudioBuffer->NewMicLevel();
            if (newMicLevel != 0)
            {
                
                WEBRTC_TRACE(kTraceStream, kTraceUtility, _id,"AGC change of volume: => new=%u", newMicLevel);

                
                
                _newMicLevel = newMicLevel;
                SetEvent(_hSetCaptureVolumeEvent);
            }
        }

        
        if (_recDelayCount > (_recPutBackDelay-1))
        {
            
            bufCount = (bufCount + N_BUFFERS_IN - _recPutBackDelay) % N_BUFFERS_IN;

            
            _waveHeaderIn[bufCount].dwBytesRecorded = 0;

            
            res = waveInUnprepareHeader(_hWaveIn, &(_waveHeaderIn[bufCount]), sizeof(WAVEHDR));
            if (MMSYSERR_NOERROR != res)
            {
                WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "waveInUnprepareHeader(%d) failed (err=%d)", bufCount, res);
                TraceWaveInError(res);
            }

            
            res = waveInPrepareHeader(_hWaveIn, &(_waveHeaderIn[bufCount]), sizeof(WAVEHDR));
            if (res != MMSYSERR_NOERROR)
            {
                WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "waveInPrepareHeader(%d) failed (err=%d)", bufCount, res);
                TraceWaveInError(res);
                return -1;
            }

            
            res = waveInAddBuffer(_hWaveIn, &(_waveHeaderIn[bufCount]), sizeof(WAVEHDR));
            if (res != MMSYSERR_NOERROR)
            {
                WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "waveInAddBuffer(%d) failed (err=%d)", bufCount, res);
                TraceWaveInError(res);
                if (_recPutBackDelay < 50)
                {
                    _recPutBackDelay++;
                    WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "_recPutBackDelay increased to %d", _recPutBackDelay);
                }
                else
                {
                    if (_recError == 1)
                    {
                        WEBRTC_TRACE(kTraceWarning, kTraceUtility, _id, "pending recording error exists");
                    }
                    _recError = 1;  
                    WEBRTC_TRACE(kTraceError, kTraceUtility, _id, "kRecordingError message posted: _recPutBackDelay=%u", _recPutBackDelay);
                }
            }
        }  

        if (_recDelayCount < (_recPutBackDelay+1))
        {
            _recDelayCount++;
        }

        
        _recBufCount++;

        if (send) {
            
            consumedTime = (int)(t2.QuadPart-t1.QuadPart);
            
            if ((consumedTime > _perfFreq.QuadPart) || (consumedTime < 0))
                consumedTime = 0;
        }

    }  

    return nBytesRecorded;
}





int AudioDeviceWindowsWave::PlayProc(LONGLONG& consumedTime)
{
    WebRtc_Word32 remTimeMS(0);
    int8_t playBuffer[4*PLAY_BUF_SIZE_IN_SAMPLES];
    WebRtc_UWord32 writtenSamples(0);
    WebRtc_UWord32 playedSamples(0);

    LARGE_INTEGER t1;
    LARGE_INTEGER t2;

    consumedTime = 0;
    _waitCounter++;

    
    
    remTimeMS = GetPlayoutBufferDelay(writtenSamples, playedSamples);

    
    
    
    const WebRtc_UWord16 thresholdMS =
        (_playBufType == AudioDeviceModule::kAdaptiveBufferSize) ? _playBufDelay : _playBufDelayFixed;

    if (remTimeMS < thresholdMS + 9)
    {
        _dTcheckPlayBufDelay = 5;

        if (remTimeMS == 0)
        {
            WEBRTC_TRACE(kTraceInfo, kTraceUtility, _id, "playout buffer is empty => we must adapt...");
            if (_waitCounter > 30)
            {
                _erZeroCounter++;
                if (_erZeroCounter == 2)
                {
                    _playBufDelay += 15;
                    _minPlayBufDelay += 20;
                    _waitCounter = 50;
                    WEBRTC_TRACE(kTraceDebug, kTraceUtility, _id, "New playout states (er=0,erZero=2): minPlayBufDelay=%u, playBufDelay=%u", _minPlayBufDelay, _playBufDelay);
                }
                else if (_erZeroCounter == 3)
                {
                    _erZeroCounter = 0;
                    _playBufDelay += 30;
                    _minPlayBufDelay += 25;
                    _waitCounter = 0;
                    WEBRTC_TRACE(kTraceDebug, kTraceUtility, _id, "New playout states (er=0, erZero=3): minPlayBufDelay=%u, playBufDelay=%u", _minPlayBufDelay, _playBufDelay);
                }
                else
                {
                    _minPlayBufDelay += 10;
                    _playBufDelay += 15;
                    _waitCounter = 50;
                    WEBRTC_TRACE(kTraceDebug, kTraceUtility, _id, "New playout states (er=0, erZero=1): minPlayBufDelay=%u, playBufDelay=%u", _minPlayBufDelay, _playBufDelay);
                }
            }
        }
        else if (remTimeMS < _minPlayBufDelay)
        {
            
            
            

            if (_waitCounter > 30)
            {
                _playBufDelay += 10;
                if (_intro == 0)
                    _waitCounter = 0;
                WEBRTC_TRACE(kTraceDebug, kTraceUtility, _id, "Playout threshold is increased: playBufDelay=%u", _playBufDelay);
            }
        }
        else if (remTimeMS < thresholdMS - 9)
        {
            _erZeroCounter = 0;
        }
        else
        {
            _erZeroCounter = 0;
            _dTcheckPlayBufDelay = 10;
        }

        QueryPerformanceCounter(&t1);   

        
        
        
        UnLock();
        WebRtc_UWord32 nSamples = _ptrAudioBuffer->RequestPlayoutData(PLAY_BUF_SIZE_IN_SAMPLES);
        Lock();

        if (OutputSanityCheckAfterUnlockedPeriod() == -1)
        {
            
            return -1;
        }

        nSamples = _ptrAudioBuffer->GetPlayoutData(playBuffer);
        if (nSamples != PLAY_BUF_SIZE_IN_SAMPLES)
        {
            WEBRTC_TRACE(kTraceError, kTraceUtility, _id, "invalid number of output samples(%d)", nSamples);
        }

        QueryPerformanceCounter(&t2);   
        consumedTime = (int)(t2.QuadPart - t1.QuadPart);

        Write(playBuffer, PLAY_BUF_SIZE_IN_SAMPLES);

    }  
    else if (thresholdMS + 9 < remTimeMS )
    {
        _erZeroCounter = 0;
        _dTcheckPlayBufDelay = 2;    
        WEBRTC_TRACE(kTraceDebug, kTraceUtility, _id, "Need to check playout buffer more often (dT=%u, remTimeMS=%u)", _dTcheckPlayBufDelay, remTimeMS);
    }

    
    if (_waitCounter > 2000)
    {
        _intro = 0;
        _playBufDelay--;
        _waitCounter = 1990;
        WEBRTC_TRACE(kTraceDebug, kTraceUtility, _id, "Playout threshold is decreased: playBufDelay=%u", _playBufDelay);
    }

    
    if (_playBufDelay < _minPlayBufDelay)
    {
        _playBufDelay = _minPlayBufDelay;
        WEBRTC_TRACE(kTraceDebug, kTraceUtility, _id, "Playout threshold is limited to %u", _minPlayBufDelay);
    }

    
    if (_playBufDelay > 150)
    {
        _playBufDelay = 150;
        WEBRTC_TRACE(kTraceDebug, kTraceUtility, _id, "Playout threshold is limited to %d", _playBufDelay);
    }

    
    
    if (_minPlayBufDelay > _MAX_minBuffer &&
       (_useHeader == 0))
    {
        _minPlayBufDelay = _MAX_minBuffer;
        WEBRTC_TRACE(kTraceDebug, kTraceUtility, _id, "Minimum playout threshold is limited to %d", _MAX_minBuffer);
    }

    return (0);
}





WebRtc_Word32 AudioDeviceWindowsWave::Write(int8_t* data, WebRtc_UWord16 nSamples)
{
    if (_hWaveOut == NULL)
    {
        return -1;
    }

    if (_playIsInitialized)
    {
        MMRESULT res;

        const WebRtc_UWord16 bufCount(_playBufCount);

        
        
        const WebRtc_Word16 nBytes = (2*_playChannels)*nSamples;
        memcpy(&_playBuffer[bufCount][0], &data[0], nBytes);

        
        
        
        
        
        
        
        
        res = waveOutWrite(_hWaveOut, &_waveHeaderOut[bufCount], sizeof(_waveHeaderOut[bufCount]));
        if (MMSYSERR_NOERROR != res)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "waveOutWrite(%d) failed (err=%d)", bufCount, res);
            TraceWaveOutError(res);

            _writeErrors++;
            if (_writeErrors > 10)
            {
                if (_playError == 1)
                {
                    WEBRTC_TRACE(kTraceWarning, kTraceUtility, _id, "pending playout error exists");
                }
                _playError = 1;  
                WEBRTC_TRACE(kTraceError, kTraceUtility, _id, "kPlayoutError message posted: _writeErrors=%u", _writeErrors);
            }

            return -1;
        }

        _playBufCount = (_playBufCount+1) % N_BUFFERS_OUT;  
        _writtenSamples += nSamples;                        
        _writeErrors = 0;
    }

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsWave::GetClockDrift(const WebRtc_UWord32 plSamp, const WebRtc_UWord32 rcSamp)
{
    int drift = 0;
    unsigned int plSampDiff = 0, rcSampDiff = 0;

    if (plSamp >= _plSampOld)
    {
        plSampDiff = plSamp - _plSampOld;
    }
    else
    {
        
        int i = 31;
        while(_plSampOld <= (unsigned int)POW2(i))
        {
            i--;
        }

        
        plSampDiff = plSamp +  POW2(i + 1) - _plSampOld;
    }

    if (rcSamp >= _rcSampOld)
    {
        rcSampDiff = rcSamp - _rcSampOld;
    }
    else
    {   
        int i = 31;
        while(_rcSampOld <= (unsigned int)POW2(i))
        {
            i--;
        }

        rcSampDiff = rcSamp +  POW2(i + 1) - _rcSampOld;
    }

    drift = plSampDiff - rcSampDiff;

    _plSampOld = plSamp;
    _rcSampOld = rcSamp;

    return drift;
}





WebRtc_Word32 AudioDeviceWindowsWave::MonitorRecording(const WebRtc_UWord32 time)
{
    const WebRtc_UWord16 bytesPerSample = 2*_recChannels;
    const WebRtc_UWord32 nRecordedSamples = _recordedBytes/bytesPerSample;

    if (nRecordedSamples > 5*N_REC_SAMPLES_PER_SEC)
    {
        
        if ((time - _prevRecByteCheckTime) > 5700)
        {
            
            
            
            
            
            if (_recWarning == 1)
            {
                WEBRTC_TRACE(kTraceWarning, kTraceUtility, _id, "pending recording warning exists");
            }
            _recWarning = 1;  
            WEBRTC_TRACE(kTraceWarning, kTraceUtility, _id, "kRecordingWarning message posted: time-_prevRecByteCheckTime=%d", time - _prevRecByteCheckTime);
        }

        _recordedBytes = 0;            
        _prevRecByteCheckTime = time;  
    }

    if ((time - _prevRecByteCheckTime) > 8000)
    {
        
        
        
        
        if (_recError == 1)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceUtility, _id, "pending recording error exists");
        }
        _recError = 1;  
        WEBRTC_TRACE(kTraceError, kTraceUtility, _id, "kRecordingError message posted: time-_prevRecByteCheckTime=%d", time - _prevRecByteCheckTime);

        _prevRecByteCheckTime = time;
    }

    return 0;
}







WebRtc_Word32 AudioDeviceWindowsWave::RestartTimerIfNeeded(const WebRtc_UWord32 time)
{
    const WebRtc_UWord32 diffMS = time - _prevTimerCheckTime;
    _prevTimerCheckTime = time;

    if (diffMS > 7)
    {
        
        _timerFaults++;
        if (_timerFaults > 5 && _timerRestartAttempts < 2)
        {
            
            
            
            
            
            
            WEBRTC_TRACE(kTraceWarning, kTraceUtility, _id, " timer issue detected => timer is restarted");
            _timeEvent.StopTimer();
            _timeEvent.StartTimer(true, TIMER_PERIOD_MS);
            
            _timerFaults = -20;
            _timerRestartAttempts++;
        }
    }
    else
    {
        
        _timerFaults = 0;
        _timerRestartAttempts = 0;
    }

    return 0;
}

}  












#ifndef WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_WAVE_WIN_H
#define WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_WAVE_WIN_H

#include "audio_device_generic.h"
#include "audio_mixer_manager_win.h"

#pragma comment( lib, "winmm.lib" )

namespace webrtc {
class EventWrapper;
class ThreadWrapper;

const WebRtc_UWord32 TIMER_PERIOD_MS = 2;
const WebRtc_UWord32 REC_CHECK_TIME_PERIOD_MS = 4;
const WebRtc_UWord16 REC_PUT_BACK_DELAY = 4;

const WebRtc_UWord32 N_REC_SAMPLES_PER_SEC = 48000;
const WebRtc_UWord32 N_PLAY_SAMPLES_PER_SEC = 48000;

const WebRtc_UWord32 N_REC_CHANNELS = 1;  
const WebRtc_UWord32 N_PLAY_CHANNELS = 2; 


const WebRtc_UWord32 REC_BUF_SIZE_IN_SAMPLES = (N_REC_SAMPLES_PER_SEC/100);
const WebRtc_UWord32 PLAY_BUF_SIZE_IN_SAMPLES = (N_PLAY_SAMPLES_PER_SEC/100);

enum { N_BUFFERS_IN = 200 };
enum { N_BUFFERS_OUT = 200 };

class AudioDeviceWindowsWave : public AudioDeviceGeneric
{
public:
    AudioDeviceWindowsWave(const WebRtc_Word32 id);
    ~AudioDeviceWindowsWave();

    
    virtual WebRtc_Word32 ActiveAudioLayer(AudioDeviceModule::AudioLayer& audioLayer) const;

    
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
    virtual WebRtc_Word32 SetPlayoutDevice(AudioDeviceModule::WindowsDeviceType device);
    virtual WebRtc_Word32 SetRecordingDevice(WebRtc_UWord16 index);
    virtual WebRtc_Word32 SetRecordingDevice(AudioDeviceModule::WindowsDeviceType device);

    
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

    
    virtual WebRtc_Word32 SetWaveOutVolume(WebRtc_UWord16 volumeLeft, WebRtc_UWord16 volumeRight);
    virtual WebRtc_Word32 WaveOutVolume(WebRtc_UWord16& volumeLeft, WebRtc_UWord16& volumeRight) const;

    
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
    virtual WebRtc_Word32 MicrophoneVolumeStepSize(WebRtc_UWord16& stepSize) const;

    
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

    
    virtual WebRtc_Word32 SetPlayoutBuffer(const AudioDeviceModule::BufferType type, WebRtc_UWord16 sizeMS);
    virtual WebRtc_Word32 PlayoutBuffer(AudioDeviceModule::BufferType& type, WebRtc_UWord16& sizeMS) const;
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
    void Lock() { _critSect.Enter(); };
    void UnLock() { _critSect.Leave(); };
    WebRtc_Word32 Id() {return _id;}
    bool IsUsingOutputDeviceIndex() const {return _usingOutputDeviceIndex;}
    AudioDeviceModule::WindowsDeviceType OutputDevice() const {return _outputDevice;}
    WebRtc_UWord16 OutputDeviceIndex() const {return _outputDeviceIndex;}
    bool IsUsingInputDeviceIndex() const {return _usingInputDeviceIndex;}
    AudioDeviceModule::WindowsDeviceType InputDevice() const {return _inputDevice;}
    WebRtc_UWord16 InputDeviceIndex() const {return _inputDeviceIndex;}

private:
    inline WebRtc_Word32 InputSanityCheckAfterUnlockedPeriod() const;
    inline WebRtc_Word32 OutputSanityCheckAfterUnlockedPeriod() const;

private:
    WebRtc_Word32 EnumeratePlayoutDevices();
    WebRtc_Word32 EnumerateRecordingDevices();
    void TraceSupportFlags(DWORD dwSupport) const;
    void TraceWaveInError(MMRESULT error) const;
    void TraceWaveOutError(MMRESULT error) const;
    WebRtc_Word32 PrepareStartRecording();
    WebRtc_Word32 PrepareStartPlayout();

    WebRtc_Word32 RecProc(LONGLONG& consumedTime);
    int PlayProc(LONGLONG& consumedTime);

    WebRtc_Word32 GetPlayoutBufferDelay(WebRtc_UWord32& writtenSamples, WebRtc_UWord32& playedSamples);
    WebRtc_Word32 GetRecordingBufferDelay(WebRtc_UWord32& readSamples, WebRtc_UWord32& recSamples);
    WebRtc_Word32 Write(int8_t* data, WebRtc_UWord16 nSamples);
    WebRtc_Word32 GetClockDrift(const WebRtc_UWord32 plSamp, const WebRtc_UWord32 rcSamp);
    WebRtc_Word32 MonitorRecording(const WebRtc_UWord32 time);
    WebRtc_Word32 RestartTimerIfNeeded(const WebRtc_UWord32 time);

private:
    static bool ThreadFunc(void*);
    bool ThreadProcess();

    static DWORD WINAPI GetCaptureVolumeThread(LPVOID context);
    DWORD DoGetCaptureVolumeThread();

    static DWORD WINAPI SetCaptureVolumeThread(LPVOID context);
    DWORD DoSetCaptureVolumeThread();

private:
    AudioDeviceBuffer*                      _ptrAudioBuffer;

    CriticalSectionWrapper&                 _critSect;
    EventWrapper&                           _timeEvent;
    EventWrapper&                           _recStartEvent;
    EventWrapper&                           _playStartEvent;

    HANDLE                                  _hGetCaptureVolumeThread;
    HANDLE                                  _hShutdownGetVolumeEvent;
    HANDLE                                  _hSetCaptureVolumeThread;
    HANDLE                                  _hShutdownSetVolumeEvent;
    HANDLE                                  _hSetCaptureVolumeEvent;

    ThreadWrapper*                          _ptrThread;
    WebRtc_UWord32                          _threadID;

    CriticalSectionWrapper&                 _critSectCb;

    WebRtc_Word32                           _id;

    AudioMixerManager                       _mixerManager;

    bool                                    _usingInputDeviceIndex;
    bool                                    _usingOutputDeviceIndex;
    AudioDeviceModule::WindowsDeviceType    _inputDevice;
    AudioDeviceModule::WindowsDeviceType    _outputDevice;
    WebRtc_UWord16                          _inputDeviceIndex;
    WebRtc_UWord16                          _outputDeviceIndex;
    bool                                    _inputDeviceIsSpecified;
    bool                                    _outputDeviceIsSpecified;

    WAVEFORMATEX                            _waveFormatIn;
    WAVEFORMATEX                            _waveFormatOut;

    HWAVEIN                                 _hWaveIn;
    HWAVEOUT                                _hWaveOut;

    WAVEHDR                                 _waveHeaderIn[N_BUFFERS_IN];
    WAVEHDR                                 _waveHeaderOut[N_BUFFERS_OUT];

    WebRtc_UWord8                           _recChannels;
    WebRtc_UWord8                           _playChannels;
    WebRtc_UWord16                          _recBufCount;
    WebRtc_UWord16                          _recDelayCount;
    WebRtc_UWord16                          _recPutBackDelay;

    int8_t                                  _recBuffer[N_BUFFERS_IN][4*REC_BUF_SIZE_IN_SAMPLES];
    int8_t                                  _playBuffer[N_BUFFERS_OUT][4*PLAY_BUF_SIZE_IN_SAMPLES];

    AudioDeviceModule::BufferType           _playBufType;

private:
    bool                                    _initialized;
    bool                                    _recording;
    bool                                    _playing;
    bool                                    _recIsInitialized;
    bool                                    _playIsInitialized;
    bool                                    _startRec;
    bool                                    _stopRec;
    bool                                    _startPlay;
    bool                                    _stopPlay;
    bool                                    _AGC;

private:
    WebRtc_UWord32                          _prevPlayTime;
    WebRtc_UWord32                          _prevRecTime;
    WebRtc_UWord32                          _prevTimerCheckTime;

    WebRtc_UWord16                          _playBufCount;          
    WebRtc_UWord16                          _dTcheckPlayBufDelay;   
    WebRtc_UWord16                          _playBufDelay;          
    WebRtc_UWord16                          _playBufDelayFixed;     
    WebRtc_UWord16                          _minPlayBufDelay;       
    WebRtc_UWord16                          _MAX_minBuffer;         

    WebRtc_Word32                           _erZeroCounter;         
    WebRtc_Word32                           _intro;
    WebRtc_Word32                           _waitCounter;

    WebRtc_UWord32                          _writtenSamples;
    WebRtc_UWord32                          _writtenSamplesOld;
    WebRtc_UWord32                          _playedSamplesOld;

    WebRtc_UWord32                          _sndCardPlayDelay;
    WebRtc_UWord32                          _sndCardRecDelay;

    WebRtc_UWord32                          _plSampOld;
    WebRtc_UWord32                          _rcSampOld;

    WebRtc_UWord32                          _read_samples;
    WebRtc_UWord32                          _read_samples_old;
    WebRtc_UWord32                          _rec_samples_old;

    
    WebRtc_Word32                           _dc_diff_mean;
    WebRtc_Word32                           _dc_y_prev;
    WebRtc_Word32                           _dc_penalty_counter;
    WebRtc_Word32                           _dc_prevtime;
    WebRtc_UWord32                          _dc_prevplay;

    WebRtc_UWord32                          _recordedBytes;         
    WebRtc_UWord32                          _prevRecByteCheckTime;  

    
    LARGE_INTEGER                           _perfFreq;
    LONGLONG                                _playAcc;               
    float                                   _avgCPULoad;            

    WebRtc_Word32                           _wrapCounter;

    WebRtc_Word32                           _useHeader;
    WebRtc_Word16                           _timesdwBytes;
    WebRtc_Word32                           _no_of_msecleft_warnings;
    WebRtc_Word32                           _writeErrors;
    WebRtc_Word32                           _timerFaults;
    WebRtc_Word32                           _timerRestartAttempts;

    WebRtc_UWord16                          _playWarning;
    WebRtc_UWord16                          _playError;
    WebRtc_UWord16                          _recWarning;
    WebRtc_UWord16                          _recError;

    WebRtc_UWord32                          _newMicLevel;
    WebRtc_UWord32                          _minMicVolume;
    WebRtc_UWord32                          _maxMicVolume;
};

}  

#endif  

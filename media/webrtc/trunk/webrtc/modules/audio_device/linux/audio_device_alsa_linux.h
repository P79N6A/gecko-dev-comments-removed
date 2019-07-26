









#ifndef WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_ALSA_LINUX_H
#define WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_ALSA_LINUX_H

#include "audio_device_generic.h"
#include "audio_mixer_manager_alsa_linux.h"
#include "critical_section_wrapper.h"


#include <alsa/asoundlib.h>
#include <sys/soundcard.h>
#include <sys/ioctl.h>
#include <X11/Xlib.h>


namespace webrtc
{
class EventWrapper;
class ThreadWrapper;

class AudioDeviceLinuxALSA : public AudioDeviceGeneric
{
public:
    AudioDeviceLinuxALSA(const int32_t id);
    ~AudioDeviceLinuxALSA();

    
    virtual int32_t ActiveAudioLayer(
        AudioDeviceModule::AudioLayer& audioLayer) const;

    
    virtual int32_t Init();
    virtual int32_t Terminate();
    virtual bool Initialized() const;

    
    virtual int16_t PlayoutDevices();
    virtual int16_t RecordingDevices();
    virtual int32_t PlayoutDeviceName(
        uint16_t index,
        char name[kAdmMaxDeviceNameSize],
        char guid[kAdmMaxGuidSize]);
    virtual int32_t RecordingDeviceName(
        uint16_t index,
        char name[kAdmMaxDeviceNameSize],
        char guid[kAdmMaxGuidSize]);

    
    virtual int32_t SetPlayoutDevice(uint16_t index);
    virtual int32_t SetPlayoutDevice(
        AudioDeviceModule::WindowsDeviceType device);
    virtual int32_t SetRecordingDevice(uint16_t index);
    virtual int32_t SetRecordingDevice(
        AudioDeviceModule::WindowsDeviceType device);

    
    virtual int32_t PlayoutIsAvailable(bool& available);
    virtual int32_t InitPlayout();
    virtual bool PlayoutIsInitialized() const;
    virtual int32_t RecordingIsAvailable(bool& available);
    virtual int32_t InitRecording();
    virtual bool RecordingIsInitialized() const;

    
    virtual int32_t StartPlayout();
    virtual int32_t StopPlayout();
    virtual bool Playing() const;
    virtual int32_t StartRecording();
    virtual int32_t StopRecording();
    virtual bool Recording() const;

    
    virtual int32_t SetAGC(bool enable);
    virtual bool AGC() const;

    
    virtual int32_t SetWaveOutVolume(uint16_t volumeLeft, uint16_t volumeRight);
    virtual int32_t WaveOutVolume(uint16_t& volumeLeft,
                                  uint16_t& volumeRight) const;

    
    virtual int32_t SpeakerIsAvailable(bool& available);
    virtual int32_t InitSpeaker();
    virtual bool SpeakerIsInitialized() const;
    virtual int32_t MicrophoneIsAvailable(bool& available);
    virtual int32_t InitMicrophone();
    virtual bool MicrophoneIsInitialized() const;

    
    virtual int32_t SpeakerVolumeIsAvailable(bool& available);
    virtual int32_t SetSpeakerVolume(uint32_t volume);
    virtual int32_t SpeakerVolume(uint32_t& volume) const;
    virtual int32_t MaxSpeakerVolume(uint32_t& maxVolume) const;
    virtual int32_t MinSpeakerVolume(uint32_t& minVolume) const;
    virtual int32_t SpeakerVolumeStepSize(uint16_t& stepSize) const;

    
    virtual int32_t MicrophoneVolumeIsAvailable(bool& available);
    virtual int32_t SetMicrophoneVolume(uint32_t volume);
    virtual int32_t MicrophoneVolume(uint32_t& volume) const;
    virtual int32_t MaxMicrophoneVolume(uint32_t& maxVolume) const;
    virtual int32_t MinMicrophoneVolume(uint32_t& minVolume) const;
    virtual int32_t MicrophoneVolumeStepSize(
        uint16_t& stepSize) const;

    
    virtual int32_t SpeakerMuteIsAvailable(bool& available);
    virtual int32_t SetSpeakerMute(bool enable);
    virtual int32_t SpeakerMute(bool& enabled) const;
    
    
    virtual int32_t MicrophoneMuteIsAvailable(bool& available);
    virtual int32_t SetMicrophoneMute(bool enable);
    virtual int32_t MicrophoneMute(bool& enabled) const;

    
    virtual int32_t MicrophoneBoostIsAvailable(bool& available);
    virtual int32_t SetMicrophoneBoost(bool enable);
    virtual int32_t MicrophoneBoost(bool& enabled) const;

    
    virtual int32_t StereoPlayoutIsAvailable(bool& available);
    virtual int32_t SetStereoPlayout(bool enable);
    virtual int32_t StereoPlayout(bool& enabled) const;
    virtual int32_t StereoRecordingIsAvailable(bool& available);
    virtual int32_t SetStereoRecording(bool enable);
    virtual int32_t StereoRecording(bool& enabled) const;
   
    
    virtual int32_t SetPlayoutBuffer(
        const AudioDeviceModule::BufferType type,
        uint16_t sizeMS);
    virtual int32_t PlayoutBuffer(
        AudioDeviceModule::BufferType& type,
        uint16_t& sizeMS) const;
    virtual int32_t PlayoutDelay(uint16_t& delayMS) const;
    virtual int32_t RecordingDelay(uint16_t& delayMS) const;

    
    virtual int32_t CPULoad(uint16_t& load) const;

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
    int32_t GetDevicesInfo(const int32_t function,
                           const bool playback,
                           const int32_t enumDeviceNo = 0,
                           char* enumDeviceName = NULL,
                           const int32_t ednLen = 0,
                           char* enumDeviceID = NULL,
                           const int32_t ediLen = 0) const;
    int32_t ErrorRecovery(int32_t error, snd_pcm_t* deviceHandle);

private:
    bool KeyPressed() const;

private:
    void Lock() { _critSect.Enter(); };
    void UnLock() { _critSect.Leave(); };
private:
    inline int32_t InputSanityCheckAfterUnlockedPeriod() const;
    inline int32_t OutputSanityCheckAfterUnlockedPeriod() const;

private:
    static bool RecThreadFunc(void*);
    static bool PlayThreadFunc(void*);
    bool RecThreadProcess();
    bool PlayThreadProcess();

private:
    AudioDeviceBuffer* _ptrAudioBuffer;
    
    CriticalSectionWrapper& _critSect;

    ThreadWrapper* _ptrThreadRec;
    ThreadWrapper* _ptrThreadPlay;
    uint32_t _recThreadID;
    uint32_t _playThreadID;

    int32_t _id;

    AudioMixerManagerLinuxALSA _mixerManager;

    uint16_t _inputDeviceIndex;
    uint16_t _outputDeviceIndex;
    bool _inputDeviceIsSpecified;
    bool _outputDeviceIsSpecified;

    snd_pcm_t* _handleRecord;
    snd_pcm_t* _handlePlayout;

    snd_pcm_uframes_t _recordingBuffersizeInFrame;
    snd_pcm_uframes_t _recordingPeriodSizeInFrame;
    snd_pcm_uframes_t _playoutBufferSizeInFrame;
    snd_pcm_uframes_t _playoutPeriodSizeInFrame;

    ssize_t _recordingBufferSizeIn10MS;
    ssize_t _playoutBufferSizeIn10MS;
    uint32_t _recordingFramesIn10MS;
    uint32_t _playoutFramesIn10MS;

    uint32_t _recordingFreq;
    uint32_t _playoutFreq;
    uint8_t _recChannels;
    uint8_t _playChannels;

    int8_t* _recordingBuffer; 
    int8_t* _playoutBuffer; 
    uint32_t _recordingFramesLeft;
    uint32_t _playoutFramesLeft;

    AudioDeviceModule::BufferType _playBufType;

private:
    bool _initialized;
    bool _recording;
    bool _playing;
    bool _recIsInitialized;
    bool _playIsInitialized;
    bool _AGC;

    snd_pcm_sframes_t _recordingDelay;
    snd_pcm_sframes_t _playoutDelay;

    uint16_t _playWarning;
    uint16_t _playError;
    uint16_t _recWarning;
    uint16_t _recError;

    uint16_t _playBufDelay;                 
    uint16_t _playBufDelayFixed;            

    char _oldKeyState[32];
    Display* _XDisplay;
};

}

#endif  

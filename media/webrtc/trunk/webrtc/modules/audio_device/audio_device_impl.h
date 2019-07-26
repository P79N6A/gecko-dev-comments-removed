









#ifndef WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_IMPL_H
#define WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_IMPL_H

#include "audio_device.h"
#include "audio_device_buffer.h"

namespace webrtc
{

class AudioDeviceGeneric;
class AudioDeviceUtility;
class CriticalSectionWrapper;

class AudioDeviceModuleImpl : public AudioDeviceModule
{
public:
    enum PlatformType
    {
        kPlatformNotSupported = 0,
        kPlatformWin32 = 1,
        kPlatformWinCe = 2,
        kPlatformLinux = 3,
        kPlatformMac = 4,
        kPlatformAndroid = 5,
        kPlatformIOS = 6
    };

    WebRtc_Word32 CheckPlatform();
    WebRtc_Word32 CreatePlatformSpecificObjects();
    WebRtc_Word32 AttachAudioBuffer();

    AudioDeviceModuleImpl(const WebRtc_Word32 id, const AudioLayer audioLayer);
    virtual ~AudioDeviceModuleImpl();

public: 
    virtual WebRtc_Word32 ChangeUniqueId(const WebRtc_Word32 id);
    virtual WebRtc_Word32 TimeUntilNextProcess();
    virtual WebRtc_Word32 Process();

public:
    
    static AudioDeviceModule* Create(
        const WebRtc_Word32 id,
        const AudioLayer audioLayer = kPlatformDefaultAudio);

    
    virtual WebRtc_Word32 ActiveAudioLayer(AudioLayer* audioLayer) const;

    
    virtual ErrorCode LastError() const;
    virtual WebRtc_Word32 RegisterEventObserver(
        AudioDeviceObserver* eventCallback);

    
    virtual WebRtc_Word32 RegisterAudioCallback(
        AudioTransport* audioCallback);

    
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
    virtual WebRtc_Word32 SetPlayoutDevice(WindowsDeviceType device);
    virtual WebRtc_Word32 SetRecordingDevice(WebRtc_UWord16 index);
    virtual WebRtc_Word32 SetRecordingDevice(WindowsDeviceType device);

    
    virtual WebRtc_Word32 PlayoutIsAvailable(bool* available);
    virtual WebRtc_Word32 InitPlayout();
    virtual bool PlayoutIsInitialized() const;
    virtual WebRtc_Word32 RecordingIsAvailable(bool* available);
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
    virtual WebRtc_Word32 WaveOutVolume(WebRtc_UWord16* volumeLeft,
                                        WebRtc_UWord16* volumeRight) const;

    
    virtual WebRtc_Word32 SpeakerIsAvailable(bool* available);
    virtual WebRtc_Word32 InitSpeaker();
    virtual bool SpeakerIsInitialized() const;
    virtual WebRtc_Word32 MicrophoneIsAvailable(bool* available);
    virtual WebRtc_Word32 InitMicrophone();
    virtual bool MicrophoneIsInitialized() const;

    
    virtual WebRtc_Word32 SpeakerVolumeIsAvailable(bool* available);
    virtual WebRtc_Word32 SetSpeakerVolume(WebRtc_UWord32 volume);
    virtual WebRtc_Word32 SpeakerVolume(WebRtc_UWord32* volume) const;
    virtual WebRtc_Word32 MaxSpeakerVolume(WebRtc_UWord32* maxVolume) const;
    virtual WebRtc_Word32 MinSpeakerVolume(WebRtc_UWord32* minVolume) const;
    virtual WebRtc_Word32 SpeakerVolumeStepSize(
        WebRtc_UWord16* stepSize) const;

    
    virtual WebRtc_Word32 MicrophoneVolumeIsAvailable(bool* available);
    virtual WebRtc_Word32 SetMicrophoneVolume(WebRtc_UWord32 volume);
    virtual WebRtc_Word32 MicrophoneVolume(WebRtc_UWord32* volume) const;
    virtual WebRtc_Word32 MaxMicrophoneVolume(
        WebRtc_UWord32* maxVolume) const;
    virtual WebRtc_Word32 MinMicrophoneVolume(
        WebRtc_UWord32* minVolume) const;
    virtual WebRtc_Word32 MicrophoneVolumeStepSize(
        WebRtc_UWord16* stepSize) const;

    
    virtual WebRtc_Word32 SpeakerMuteIsAvailable(bool* available);
    virtual WebRtc_Word32 SetSpeakerMute(bool enable);
    virtual WebRtc_Word32 SpeakerMute(bool* enabled) const;

    
    virtual WebRtc_Word32 MicrophoneMuteIsAvailable(bool* available);
    virtual WebRtc_Word32 SetMicrophoneMute(bool enable);
    virtual WebRtc_Word32 MicrophoneMute(bool* enabled) const;

    
    virtual WebRtc_Word32 MicrophoneBoostIsAvailable(bool* available);
    virtual WebRtc_Word32 SetMicrophoneBoost(bool enable);
    virtual WebRtc_Word32 MicrophoneBoost(bool* enabled) const;

    
    virtual WebRtc_Word32 StereoPlayoutIsAvailable(bool* available) const;
    virtual WebRtc_Word32 SetStereoPlayout(bool enable);
    virtual WebRtc_Word32 StereoPlayout(bool* enabled) const;
    virtual WebRtc_Word32 StereoRecordingIsAvailable(bool* available) const;
    virtual WebRtc_Word32 SetStereoRecording(bool enable);
    virtual WebRtc_Word32 StereoRecording(bool* enabled) const;
    virtual WebRtc_Word32 SetRecordingChannel(const ChannelType channel);
    virtual WebRtc_Word32 RecordingChannel(ChannelType* channel) const;

    
    virtual WebRtc_Word32 SetPlayoutBuffer(const BufferType type,
                                           WebRtc_UWord16 sizeMS = 0);
    virtual WebRtc_Word32 PlayoutBuffer(BufferType* type,
                                        WebRtc_UWord16* sizeMS) const;
    virtual WebRtc_Word32 PlayoutDelay(WebRtc_UWord16* delayMS) const;
    virtual WebRtc_Word32 RecordingDelay(WebRtc_UWord16* delayMS) const;

    
    virtual WebRtc_Word32 CPULoad(WebRtc_UWord16* load) const;

    
    virtual WebRtc_Word32 StartRawOutputFileRecording(
        const char pcmFileNameUTF8[kAdmMaxFileNameSize]);
    virtual WebRtc_Word32 StopRawOutputFileRecording();
    virtual WebRtc_Word32 StartRawInputFileRecording(
        const char pcmFileNameUTF8[kAdmMaxFileNameSize]);
    virtual WebRtc_Word32 StopRawInputFileRecording();

    
    virtual WebRtc_Word32 SetRecordingSampleRate(
        const WebRtc_UWord32 samplesPerSec);
    virtual WebRtc_Word32 RecordingSampleRate(
        WebRtc_UWord32* samplesPerSec) const;
    virtual WebRtc_Word32 SetPlayoutSampleRate(
        const WebRtc_UWord32 samplesPerSec);
    virtual WebRtc_Word32 PlayoutSampleRate(
        WebRtc_UWord32* samplesPerSec) const;

    
    virtual WebRtc_Word32 ResetAudioDevice();
    virtual WebRtc_Word32 SetLoudspeakerStatus(bool enable);
    virtual WebRtc_Word32 GetLoudspeakerStatus(bool* enabled) const;

    virtual int32_t EnableBuiltInAEC(bool enable);
    virtual bool BuiltInAECIsEnabled() const;

public:
    WebRtc_Word32 Id() {return _id;}

private:
    PlatformType Platform() const;
    AudioLayer PlatformAudioLayer() const;

private:
    CriticalSectionWrapper&     _critSect;
    CriticalSectionWrapper&     _critSectEventCb;
    CriticalSectionWrapper&     _critSectAudioCb;

    AudioDeviceObserver*        _ptrCbAudioDeviceObserver;

    AudioDeviceUtility*         _ptrAudioDeviceUtility;
    AudioDeviceGeneric*         _ptrAudioDevice;

    AudioDeviceBuffer           _audioDeviceBuffer;

    WebRtc_Word32               _id;
    AudioLayer                  _platformAudioLayer;
    WebRtc_UWord32              _lastProcessTime;
    PlatformType                _platformType;
    bool                        _initialized;
    mutable ErrorCode           _lastError;
};

}  

#endif  

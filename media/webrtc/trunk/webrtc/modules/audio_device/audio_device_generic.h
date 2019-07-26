









#ifndef WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_GENERIC_H
#define WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_GENERIC_H

#include "audio_device.h"
#include "audio_device_buffer.h"

namespace webrtc {

class AudioDeviceGeneric
{
 public:

	
	virtual WebRtc_Word32 ActiveAudioLayer(
        AudioDeviceModule::AudioLayer& audioLayer) const = 0;

	
    virtual WebRtc_Word32 Init() = 0;
    virtual WebRtc_Word32 Terminate() = 0;
	virtual bool Initialized() const = 0;

	
	virtual WebRtc_Word16 PlayoutDevices() = 0;
	virtual WebRtc_Word16 RecordingDevices() = 0;
	virtual WebRtc_Word32 PlayoutDeviceName(
        WebRtc_UWord16 index,
        char name[kAdmMaxDeviceNameSize],
        char guid[kAdmMaxGuidSize]) = 0;
    virtual WebRtc_Word32 RecordingDeviceName(
        WebRtc_UWord16 index,
        char name[kAdmMaxDeviceNameSize],
        char guid[kAdmMaxGuidSize]) = 0;

	
	virtual WebRtc_Word32 SetPlayoutDevice(WebRtc_UWord16 index) = 0;
	virtual WebRtc_Word32 SetPlayoutDevice(
        AudioDeviceModule::WindowsDeviceType device) = 0;
    virtual WebRtc_Word32 SetRecordingDevice(WebRtc_UWord16 index) = 0;
	virtual WebRtc_Word32 SetRecordingDevice(
        AudioDeviceModule::WindowsDeviceType device) = 0;

	
    virtual WebRtc_Word32 PlayoutIsAvailable(bool& available) = 0;
    virtual WebRtc_Word32 InitPlayout() = 0;
    virtual bool PlayoutIsInitialized() const = 0;
    virtual WebRtc_Word32 RecordingIsAvailable(bool& available) = 0;
    virtual WebRtc_Word32 InitRecording() = 0;
    virtual bool RecordingIsInitialized() const = 0;

	
    virtual WebRtc_Word32 StartPlayout() = 0;
    virtual WebRtc_Word32 StopPlayout() = 0;
    virtual bool Playing() const = 0;
	virtual WebRtc_Word32 StartRecording() = 0;
    virtual WebRtc_Word32 StopRecording() = 0;
    virtual bool Recording() const = 0;

    
    virtual WebRtc_Word32 SetAGC(bool enable) = 0;
    virtual bool AGC() const = 0;

    
    virtual WebRtc_Word32 SetWaveOutVolume(WebRtc_UWord16 volumeLeft,
                                           WebRtc_UWord16 volumeRight) = 0;
    virtual WebRtc_Word32 WaveOutVolume(WebRtc_UWord16& volumeLeft,
                                        WebRtc_UWord16& volumeRight) const = 0;

	
	virtual WebRtc_Word32 SpeakerIsAvailable(bool& available) = 0;
    virtual WebRtc_Word32 InitSpeaker() = 0;
    virtual bool SpeakerIsInitialized() const = 0;
	virtual WebRtc_Word32 MicrophoneIsAvailable(bool& available) = 0;
    virtual WebRtc_Word32 InitMicrophone() = 0;
    virtual bool MicrophoneIsInitialized() const = 0;

    
	virtual WebRtc_Word32 SpeakerVolumeIsAvailable(bool& available) = 0;
    virtual WebRtc_Word32 SetSpeakerVolume(WebRtc_UWord32 volume) = 0;
    virtual WebRtc_Word32 SpeakerVolume(WebRtc_UWord32& volume) const = 0;
    virtual WebRtc_Word32 MaxSpeakerVolume(WebRtc_UWord32& maxVolume) const = 0;
    virtual WebRtc_Word32 MinSpeakerVolume(WebRtc_UWord32& minVolume) const = 0;
    virtual WebRtc_Word32 SpeakerVolumeStepSize(
        WebRtc_UWord16& stepSize) const = 0;

    
	virtual WebRtc_Word32 MicrophoneVolumeIsAvailable(bool& available) = 0;
    virtual WebRtc_Word32 SetMicrophoneVolume(WebRtc_UWord32 volume) = 0;
    virtual WebRtc_Word32 MicrophoneVolume(WebRtc_UWord32& volume) const = 0;
    virtual WebRtc_Word32 MaxMicrophoneVolume(
        WebRtc_UWord32& maxVolume) const = 0;
    virtual WebRtc_Word32 MinMicrophoneVolume(
        WebRtc_UWord32& minVolume) const = 0;
    virtual WebRtc_Word32 MicrophoneVolumeStepSize(
        WebRtc_UWord16& stepSize) const = 0;

    
    virtual WebRtc_Word32 SpeakerMuteIsAvailable(bool& available) = 0;
    virtual WebRtc_Word32 SetSpeakerMute(bool enable) = 0;
    virtual WebRtc_Word32 SpeakerMute(bool& enabled) const = 0;

	
    virtual WebRtc_Word32 MicrophoneMuteIsAvailable(bool& available) = 0;
    virtual WebRtc_Word32 SetMicrophoneMute(bool enable) = 0;
    virtual WebRtc_Word32 MicrophoneMute(bool& enabled) const = 0;

    
    virtual WebRtc_Word32 MicrophoneBoostIsAvailable(bool& available) = 0;
	virtual WebRtc_Word32 SetMicrophoneBoost(bool enable) = 0;
    virtual WebRtc_Word32 MicrophoneBoost(bool& enabled) const = 0;

    
    virtual WebRtc_Word32 StereoPlayoutIsAvailable(bool& available) = 0;
	virtual WebRtc_Word32 SetStereoPlayout(bool enable) = 0;
    virtual WebRtc_Word32 StereoPlayout(bool& enabled) const = 0;
    virtual WebRtc_Word32 StereoRecordingIsAvailable(bool& available) = 0;
    virtual WebRtc_Word32 SetStereoRecording(bool enable) = 0;
    virtual WebRtc_Word32 StereoRecording(bool& enabled) const = 0;

    
	virtual WebRtc_Word32 SetPlayoutBuffer(
        const AudioDeviceModule::BufferType type,
        WebRtc_UWord16 sizeMS = 0) = 0;
    virtual WebRtc_Word32 PlayoutBuffer(
        AudioDeviceModule::BufferType& type, WebRtc_UWord16& sizeMS) const = 0;
    virtual WebRtc_Word32 PlayoutDelay(WebRtc_UWord16& delayMS) const = 0;
	virtual WebRtc_Word32 RecordingDelay(WebRtc_UWord16& delayMS) const = 0;

    
    virtual WebRtc_Word32 CPULoad(WebRtc_UWord16& load) const = 0;
    
    
	virtual WebRtc_Word32 SetRecordingSampleRate(
        const WebRtc_UWord32 samplesPerSec);
	virtual WebRtc_Word32 SetPlayoutSampleRate(
        const WebRtc_UWord32 samplesPerSec);

    
    virtual WebRtc_Word32 SetLoudspeakerStatus(bool enable);
    virtual WebRtc_Word32 GetLoudspeakerStatus(bool& enable) const;
    
    
    virtual WebRtc_Word32 ResetAudioDevice();

    
    virtual WebRtc_Word32 SoundDeviceControl(unsigned int par1 = 0,
                                             unsigned int par2 = 0,
                                             unsigned int par3 = 0,
                                             unsigned int par4 = 0);

    
    virtual int32_t EnableBuiltInAEC(bool enable);
    virtual bool BuiltInAECIsEnabled() const;

public:
    virtual bool PlayoutWarning() const = 0;
    virtual bool PlayoutError() const = 0;
    virtual bool RecordingWarning() const = 0;
    virtual bool RecordingError() const = 0;
    virtual void ClearPlayoutWarning() = 0;
    virtual void ClearPlayoutError() = 0;
    virtual void ClearRecordingWarning() = 0;
    virtual void ClearRecordingError() = 0;

public:
    virtual void AttachAudioBuffer(AudioDeviceBuffer* audioBuffer) = 0;

    virtual ~AudioDeviceGeneric() {}
};

}  

#endif  


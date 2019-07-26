









#ifndef WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_GENERIC_H
#define WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_GENERIC_H

#include "webrtc/modules/audio_device/audio_device_buffer.h"
#include "webrtc/modules/audio_device/include/audio_device.h"

namespace webrtc {

class AudioDeviceGeneric
{
 public:

	
	virtual int32_t ActiveAudioLayer(
        AudioDeviceModule::AudioLayer& audioLayer) const = 0;

	
    virtual int32_t Init() = 0;
    virtual int32_t Terminate() = 0;
	virtual bool Initialized() const = 0;

	
	virtual int16_t PlayoutDevices() = 0;
	virtual int16_t RecordingDevices() = 0;
	virtual int32_t PlayoutDeviceName(
        uint16_t index,
        char name[kAdmMaxDeviceNameSize],
        char guid[kAdmMaxGuidSize]) = 0;
    virtual int32_t RecordingDeviceName(
        uint16_t index,
        char name[kAdmMaxDeviceNameSize],
        char guid[kAdmMaxGuidSize]) = 0;

	
	virtual int32_t SetPlayoutDevice(uint16_t index) = 0;
	virtual int32_t SetPlayoutDevice(
        AudioDeviceModule::WindowsDeviceType device) = 0;
    virtual int32_t SetRecordingDevice(uint16_t index) = 0;
	virtual int32_t SetRecordingDevice(
        AudioDeviceModule::WindowsDeviceType device) = 0;

	
    virtual int32_t PlayoutIsAvailable(bool& available) = 0;
    virtual int32_t InitPlayout() = 0;
    virtual bool PlayoutIsInitialized() const = 0;
    virtual int32_t RecordingIsAvailable(bool& available) = 0;
    virtual int32_t InitRecording() = 0;
    virtual bool RecordingIsInitialized() const = 0;

	
    virtual int32_t StartPlayout() = 0;
    virtual int32_t StopPlayout() = 0;
    virtual bool Playing() const = 0;
	virtual int32_t StartRecording() = 0;
    virtual int32_t StopRecording() = 0;
    virtual bool Recording() const = 0;

    
    virtual int32_t SetAGC(bool enable) = 0;
    virtual bool AGC() const = 0;

    
    virtual int32_t SetWaveOutVolume(uint16_t volumeLeft,
                                     uint16_t volumeRight) = 0;
    virtual int32_t WaveOutVolume(uint16_t& volumeLeft,
                                  uint16_t& volumeRight) const = 0;

	
	virtual int32_t SpeakerIsAvailable(bool& available) = 0;
    virtual int32_t InitSpeaker() = 0;
    virtual bool SpeakerIsInitialized() const = 0;
	virtual int32_t MicrophoneIsAvailable(bool& available) = 0;
    virtual int32_t InitMicrophone() = 0;
    virtual bool MicrophoneIsInitialized() const = 0;

    
	virtual int32_t SpeakerVolumeIsAvailable(bool& available) = 0;
    virtual int32_t SetSpeakerVolume(uint32_t volume) = 0;
    virtual int32_t SpeakerVolume(uint32_t& volume) const = 0;
    virtual int32_t MaxSpeakerVolume(uint32_t& maxVolume) const = 0;
    virtual int32_t MinSpeakerVolume(uint32_t& minVolume) const = 0;
    virtual int32_t SpeakerVolumeStepSize(
        uint16_t& stepSize) const = 0;

    
	virtual int32_t MicrophoneVolumeIsAvailable(bool& available) = 0;
    virtual int32_t SetMicrophoneVolume(uint32_t volume) = 0;
    virtual int32_t MicrophoneVolume(uint32_t& volume) const = 0;
    virtual int32_t MaxMicrophoneVolume(
        uint32_t& maxVolume) const = 0;
    virtual int32_t MinMicrophoneVolume(
        uint32_t& minVolume) const = 0;
    virtual int32_t MicrophoneVolumeStepSize(
        uint16_t& stepSize) const = 0;

    
    virtual int32_t SpeakerMuteIsAvailable(bool& available) = 0;
    virtual int32_t SetSpeakerMute(bool enable) = 0;
    virtual int32_t SpeakerMute(bool& enabled) const = 0;

	
    virtual int32_t MicrophoneMuteIsAvailable(bool& available) = 0;
    virtual int32_t SetMicrophoneMute(bool enable) = 0;
    virtual int32_t MicrophoneMute(bool& enabled) const = 0;

    
    virtual int32_t MicrophoneBoostIsAvailable(bool& available) = 0;
	virtual int32_t SetMicrophoneBoost(bool enable) = 0;
    virtual int32_t MicrophoneBoost(bool& enabled) const = 0;

    
    virtual int32_t StereoPlayoutIsAvailable(bool& available) = 0;
	virtual int32_t SetStereoPlayout(bool enable) = 0;
    virtual int32_t StereoPlayout(bool& enabled) const = 0;
    virtual int32_t StereoRecordingIsAvailable(bool& available) = 0;
    virtual int32_t SetStereoRecording(bool enable) = 0;
    virtual int32_t StereoRecording(bool& enabled) const = 0;

    
	virtual int32_t SetPlayoutBuffer(
        const AudioDeviceModule::BufferType type,
        uint16_t sizeMS = 0) = 0;
    virtual int32_t PlayoutBuffer(
        AudioDeviceModule::BufferType& type, uint16_t& sizeMS) const = 0;
    virtual int32_t PlayoutDelay(uint16_t& delayMS) const = 0;
	virtual int32_t RecordingDelay(uint16_t& delayMS) const = 0;

    
    virtual int32_t CPULoad(uint16_t& load) const = 0;
    
    
	virtual int32_t SetRecordingSampleRate(
        const uint32_t samplesPerSec);
	virtual int32_t SetPlayoutSampleRate(
        const uint32_t samplesPerSec);

    
    virtual int32_t SetLoudspeakerStatus(bool enable);
    virtual int32_t GetLoudspeakerStatus(bool& enable) const;
    
    
    virtual int32_t ResetAudioDevice();

    
    virtual int32_t SoundDeviceControl(unsigned int par1 = 0,
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

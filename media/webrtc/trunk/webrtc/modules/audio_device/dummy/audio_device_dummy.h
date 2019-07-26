









#ifndef WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_DUMMY_H
#define WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_DUMMY_H

#include <stdio.h>

#include "webrtc/modules/audio_device/audio_device_generic.h"

namespace webrtc {

class AudioDeviceDummy : public AudioDeviceGeneric {
 public:
  AudioDeviceDummy(const int32_t id) {}
  virtual ~AudioDeviceDummy() {}

  
  virtual int32_t ActiveAudioLayer(
      AudioDeviceModule::AudioLayer& audioLayer) const OVERRIDE;

  
  virtual int32_t Init() OVERRIDE;
  virtual int32_t Terminate() OVERRIDE;
  virtual bool Initialized() const OVERRIDE;

  
  virtual int16_t PlayoutDevices() OVERRIDE;
  virtual int16_t RecordingDevices() OVERRIDE;
  virtual int32_t PlayoutDeviceName(uint16_t index,
                                    char name[kAdmMaxDeviceNameSize],
                                    char guid[kAdmMaxGuidSize]) OVERRIDE;
  virtual int32_t RecordingDeviceName(uint16_t index,
                                      char name[kAdmMaxDeviceNameSize],
                                      char guid[kAdmMaxGuidSize]) OVERRIDE;

  
  virtual int32_t SetPlayoutDevice(uint16_t index) OVERRIDE;
  virtual int32_t SetPlayoutDevice(
      AudioDeviceModule::WindowsDeviceType device) OVERRIDE;
  virtual int32_t SetRecordingDevice(uint16_t index) OVERRIDE;
  virtual int32_t SetRecordingDevice(
      AudioDeviceModule::WindowsDeviceType device) OVERRIDE;

  
  virtual int32_t PlayoutIsAvailable(bool& available) OVERRIDE;
  virtual int32_t InitPlayout() OVERRIDE;
  virtual bool PlayoutIsInitialized() const OVERRIDE;
  virtual int32_t RecordingIsAvailable(bool& available) OVERRIDE;
  virtual int32_t InitRecording() OVERRIDE;
  virtual bool RecordingIsInitialized() const OVERRIDE;

  
  virtual int32_t StartPlayout() OVERRIDE;
  virtual int32_t StopPlayout() OVERRIDE;
  virtual bool Playing() const OVERRIDE;
  virtual int32_t StartRecording() OVERRIDE;
  virtual int32_t StopRecording() OVERRIDE;
  virtual bool Recording() const OVERRIDE;

  
  virtual int32_t SetAGC(bool enable) OVERRIDE;
  virtual bool AGC() const OVERRIDE;

  
  virtual int32_t SetWaveOutVolume(uint16_t volumeLeft,
                                   uint16_t volumeRight) OVERRIDE;
  virtual int32_t WaveOutVolume(uint16_t& volumeLeft,
                                uint16_t& volumeRight) const OVERRIDE;

  
  virtual int32_t SpeakerIsAvailable(bool& available) OVERRIDE;
  virtual int32_t InitSpeaker() OVERRIDE;
  virtual bool SpeakerIsInitialized() const OVERRIDE;
  virtual int32_t MicrophoneIsAvailable(bool& available) OVERRIDE;
  virtual int32_t InitMicrophone() OVERRIDE;
  virtual bool MicrophoneIsInitialized() const OVERRIDE;

  
  virtual int32_t SpeakerVolumeIsAvailable(bool& available) OVERRIDE;
  virtual int32_t SetSpeakerVolume(uint32_t volume) OVERRIDE;
  virtual int32_t SpeakerVolume(uint32_t& volume) const OVERRIDE;
  virtual int32_t MaxSpeakerVolume(uint32_t& maxVolume) const OVERRIDE;
  virtual int32_t MinSpeakerVolume(uint32_t& minVolume) const OVERRIDE;
  virtual int32_t SpeakerVolumeStepSize(uint16_t& stepSize) const OVERRIDE;

  
  virtual int32_t MicrophoneVolumeIsAvailable(bool& available) OVERRIDE;
  virtual int32_t SetMicrophoneVolume(uint32_t volume) OVERRIDE;
  virtual int32_t MicrophoneVolume(uint32_t& volume) const OVERRIDE;
  virtual int32_t MaxMicrophoneVolume(uint32_t& maxVolume) const OVERRIDE;
  virtual int32_t MinMicrophoneVolume(uint32_t& minVolume) const OVERRIDE;
  virtual int32_t MicrophoneVolumeStepSize(uint16_t& stepSize) const OVERRIDE;

  
  virtual int32_t SpeakerMuteIsAvailable(bool& available) OVERRIDE;
  virtual int32_t SetSpeakerMute(bool enable) OVERRIDE;
  virtual int32_t SpeakerMute(bool& enabled) const OVERRIDE;

  
  virtual int32_t MicrophoneMuteIsAvailable(bool& available) OVERRIDE;
  virtual int32_t SetMicrophoneMute(bool enable) OVERRIDE;
  virtual int32_t MicrophoneMute(bool& enabled) const OVERRIDE;

  
  virtual int32_t MicrophoneBoostIsAvailable(bool& available) OVERRIDE;
  virtual int32_t SetMicrophoneBoost(bool enable) OVERRIDE;
  virtual int32_t MicrophoneBoost(bool& enabled) const OVERRIDE;

  
  virtual int32_t StereoPlayoutIsAvailable(bool& available) OVERRIDE;
  virtual int32_t SetStereoPlayout(bool enable) OVERRIDE;
  virtual int32_t StereoPlayout(bool& enabled) const OVERRIDE;
  virtual int32_t StereoRecordingIsAvailable(bool& available) OVERRIDE;
  virtual int32_t SetStereoRecording(bool enable) OVERRIDE;
  virtual int32_t StereoRecording(bool& enabled) const OVERRIDE;

  
  virtual int32_t SetPlayoutBuffer(const AudioDeviceModule::BufferType type,
                                   uint16_t sizeMS) OVERRIDE;
  virtual int32_t PlayoutBuffer(AudioDeviceModule::BufferType& type,
                                uint16_t& sizeMS) const OVERRIDE;
  virtual int32_t PlayoutDelay(uint16_t& delayMS) const OVERRIDE;
  virtual int32_t RecordingDelay(uint16_t& delayMS) const OVERRIDE;

  
  virtual int32_t CPULoad(uint16_t& load) const OVERRIDE;

  virtual bool PlayoutWarning() const OVERRIDE;
  virtual bool PlayoutError() const OVERRIDE;
  virtual bool RecordingWarning() const OVERRIDE;
  virtual bool RecordingError() const OVERRIDE;
  virtual void ClearPlayoutWarning() OVERRIDE;
  virtual void ClearPlayoutError() OVERRIDE;
  virtual void ClearRecordingWarning() OVERRIDE;
  virtual void ClearRecordingError() OVERRIDE;

  virtual void AttachAudioBuffer(AudioDeviceBuffer* audioBuffer) OVERRIDE;
};

}  

#endif  

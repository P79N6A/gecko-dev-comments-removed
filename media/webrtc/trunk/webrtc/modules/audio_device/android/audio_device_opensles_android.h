









#ifndef WEBRTC_MODULES_AUDIO_DEVICE_ANDROID_AUDIO_DEVICE_OPENSLES_ANDROID_H_
#define WEBRTC_MODULES_AUDIO_DEVICE_ANDROID_AUDIO_DEVICE_OPENSLES_ANDROID_H_

#include "webrtc/modules/audio_device/audio_device_generic.h"
#include "webrtc/modules/audio_device/android/opensles_input.h"
#include "webrtc/modules/audio_device/android/opensles_output.h"

namespace webrtc {



class AudioDeviceAndroidOpenSLES : public AudioDeviceGeneric {
 public:
  explicit AudioDeviceAndroidOpenSLES(const int32_t id);
  virtual ~AudioDeviceAndroidOpenSLES();

  
  virtual int32_t ActiveAudioLayer(
      AudioDeviceModule::AudioLayer& audioLayer) const;  

  
  virtual int32_t Init();
  virtual int32_t Terminate() ;
  virtual bool Initialized() const;

  
  virtual int16_t PlayoutDevices();
  virtual int16_t RecordingDevices();
  virtual int32_t PlayoutDeviceName(uint16_t index,
                                    char name[kAdmMaxDeviceNameSize],
                                    char guid[kAdmMaxGuidSize]);
  virtual int32_t RecordingDeviceName(uint16_t index,
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

  
  virtual int32_t SetWaveOutVolume(uint16_t volumeLeft,
                                   uint16_t volumeRight);
  virtual int32_t WaveOutVolume(
      uint16_t& volumeLeft,  
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

  
  virtual int32_t SetPlayoutBuffer(const AudioDeviceModule::BufferType type,
                                   uint16_t sizeMS);
  virtual int32_t PlayoutBuffer(AudioDeviceModule::BufferType& type,
                                uint16_t& sizeMS) const;
  virtual int32_t PlayoutDelay(uint16_t& delayMS) const;
  virtual int32_t RecordingDelay(uint16_t& delayMS) const;

  
  virtual int32_t CPULoad(uint16_t& load) const;  

  
  virtual bool PlayoutWarning() const;
  virtual bool PlayoutError() const;
  virtual bool RecordingWarning() const;
  virtual bool RecordingError() const;
  virtual void ClearPlayoutWarning();
  virtual void ClearPlayoutError();
  virtual void ClearRecordingWarning();
  virtual void ClearRecordingError();

  
  virtual void AttachAudioBuffer(AudioDeviceBuffer* audioBuffer);

  
  virtual int32_t SetLoudspeakerStatus(bool enable);
  virtual int32_t GetLoudspeakerStatus(bool& enable) const;

 private:
  OpenSlesOutput output_;
  OpenSlesInput input_;
};

}  

#endif  

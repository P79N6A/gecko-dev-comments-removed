













#ifndef WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_JNI_ANDROID_H
#define WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_JNI_ANDROID_H

#include "webrtc/modules/audio_device/audio_device_generic.h"
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"

#include <jni.h> 

#define AudioCaptureClass "org/webrtc/voiceengine/WebRTCAudioDevice"

namespace webrtc
{
class EventWrapper;

class ThreadWrapper;

class AudioDeviceAndroidJni : public AudioDeviceGeneric {
 public:
  AudioDeviceAndroidJni(const int32_t id);
  ~AudioDeviceAndroidJni();

  static int32_t SetAndroidAudioDeviceObjects(void* javaVM,
                                              void* context);

  static int32_t SetAndroidAudioDeviceObjects(void* javaVM,
                                              void* env,
                                              void* context);

  virtual int32_t ActiveAudioLayer(
      AudioDeviceModule::AudioLayer& audioLayer) const;

  virtual int32_t Init();
  virtual int32_t Terminate();
  virtual bool Initialized() const;

  virtual int16_t PlayoutDevices();
  virtual int16_t RecordingDevices();
  virtual int32_t PlayoutDeviceName(uint16_t index,
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
      const AudioDeviceModule::BufferType type, uint16_t sizeMS);
  virtual int32_t PlayoutBuffer(
      AudioDeviceModule::BufferType& type, uint16_t& sizeMS) const;
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

  virtual int32_t SetRecordingSampleRate(
      const uint32_t samplesPerSec);
  virtual int32_t SetPlayoutSampleRate(
      const uint32_t samplesPerSec);

  virtual int32_t SetLoudspeakerStatus(bool enable);
  virtual int32_t GetLoudspeakerStatus(bool& enable) const;

  static const uint32_t N_REC_SAMPLES_PER_SEC = 16000; 
  static const uint32_t N_PLAY_SAMPLES_PER_SEC = 16000; 

  static const uint32_t N_REC_CHANNELS = 1; 
  static const uint32_t N_PLAY_CHANNELS = 1; 

  static const uint32_t REC_BUF_SIZE_IN_SAMPLES = 480; 

 private:
  
  void Lock() {
    _critSect.Enter();
  };
  void UnLock() {
    _critSect.Leave();
  };

  
  int32_t InitJavaResources();
  int32_t InitSampleRate();

  
  static bool RecThreadFunc(void*);
  static bool PlayThreadFunc(void*);
  bool RecThreadProcess();
  bool PlayThreadProcess();

  
  AudioDeviceBuffer* _ptrAudioBuffer;
  CriticalSectionWrapper& _critSect;
  int32_t _id;

  
  EventWrapper& _timeEventRec;
  EventWrapper& _timeEventPlay;
  EventWrapper& _recStartStopEvent;
  EventWrapper& _playStartStopEvent;

  
  ThreadWrapper* _ptrThreadPlay;
  ThreadWrapper* _ptrThreadRec;
  uint32_t _recThreadID;
  uint32_t _playThreadID;
  bool _playThreadIsInitialized;
  bool _recThreadIsInitialized;
  bool _shutdownPlayThread;
  bool _shutdownRecThread;

  
  int8_t _recBuffer[2 * REC_BUF_SIZE_IN_SAMPLES];

  
  bool _recordingDeviceIsSpecified;
  bool _playoutDeviceIsSpecified;
  bool _initialized;
  bool _recording;
  bool _playing;
  bool _recIsInitialized;
  bool _playIsInitialized;
  bool _micIsInitialized;
  bool _speakerIsInitialized;

  
  bool _startRec;
  bool _startPlay;

  
  uint16_t _playWarning;
  uint16_t _playError;
  uint16_t _recWarning;
  uint16_t _recError;

  
  uint16_t _delayPlayout;
  uint16_t _delayRecording;

  
  bool _AGC;

  
  uint16_t _samplingFreqIn; 
  uint16_t _samplingFreqOut; 
  uint32_t _maxSpeakerVolume; 
  bool _loudSpeakerOn;
  
  int _recAudioSource;

  
  JavaVM* _javaVM; 

  JNIEnv* _jniEnvPlay; 
  JNIEnv* _jniEnvRec; 

  jclass _javaScClass; 
  jobject _javaScObj; 

  
  jobject _javaPlayBuffer;
  
  jobject _javaRecBuffer;
  void* _javaDirectPlayBuffer; 
  void* _javaDirectRecBuffer; 
  jmethodID _javaMidPlayAudio; 
  jmethodID _javaMidRecAudio; 

  
  
  
  static JavaVM* globalJvm;
  static JNIEnv* globalJNIEnv;
  static jobject globalContext;
  static jclass globalScClass;
};

}  

#endif  

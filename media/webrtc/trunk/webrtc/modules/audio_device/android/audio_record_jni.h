









#ifndef WEBRTC_MODULES_AUDIO_DEVICE_ANDROID_AUDIO_RECORD_JNI_H_
#define WEBRTC_MODULES_AUDIO_DEVICE_ANDROID_AUDIO_RECORD_JNI_H_

#include <jni.h>

#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/modules/audio_device/include/audio_device_defines.h"
#include "webrtc/modules/audio_device/audio_device_generic.h"

namespace webrtc {

class EventWrapper;
class ThreadWrapper;
class PlayoutDelayProvider;

const uint32_t N_REC_SAMPLES_PER_SEC = 16000; 
const uint32_t N_REC_CHANNELS = 1; 
const uint32_t REC_BUF_SIZE_IN_SAMPLES = 480; 

class AudioRecordJni {
 public:
  static int32_t SetAndroidAudioDeviceObjects(void* javaVM, void* env,
                                              void* context);
  static void ClearAndroidAudioDeviceObjects();

  AudioRecordJni(const int32_t id, PlayoutDelayProvider* delay_provider);
  ~AudioRecordJni();

  
  int32_t Init();
  int32_t Terminate();
  bool Initialized() const { return _initialized; }

  
  int16_t RecordingDevices() { return 1; }  
  int32_t RecordingDeviceName(uint16_t index,
                              char name[kAdmMaxDeviceNameSize],
                              char guid[kAdmMaxGuidSize]);

  
  int32_t SetRecordingDevice(uint16_t index);
  int32_t SetRecordingDevice(
      AudioDeviceModule::WindowsDeviceType device);

  
  int32_t RecordingIsAvailable(bool& available);  
  int32_t InitRecording();
  bool RecordingIsInitialized() const { return _recIsInitialized; }

  
  int32_t StartRecording();
  int32_t StopRecording();
  bool Recording() const { return _recording; }

  
  int32_t SetAGC(bool enable);
  bool AGC() const { return _AGC; }

  
  int32_t MicrophoneIsAvailable(bool& available);  
  int32_t InitMicrophone();
  bool MicrophoneIsInitialized() const { return _micIsInitialized; }

  
  int32_t MicrophoneVolumeIsAvailable(bool& available);  
  
  
  int32_t SetMicrophoneVolume(uint32_t volume);
  int32_t MicrophoneVolume(uint32_t& volume) const;  
  int32_t MaxMicrophoneVolume(uint32_t& maxVolume) const;  
  int32_t MinMicrophoneVolume(uint32_t& minVolume) const;  
  int32_t MicrophoneVolumeStepSize(
      uint16_t& stepSize) const;  

  
  int32_t MicrophoneMuteIsAvailable(bool& available);  
  int32_t SetMicrophoneMute(bool enable);
  int32_t MicrophoneMute(bool& enabled) const;  

  
  int32_t MicrophoneBoostIsAvailable(bool& available);  
  int32_t SetMicrophoneBoost(bool enable);
  int32_t MicrophoneBoost(bool& enabled) const;  

  
  int32_t StereoRecordingIsAvailable(bool& available);  
  int32_t SetStereoRecording(bool enable);
  int32_t StereoRecording(bool& enabled) const;  

  
  int32_t RecordingDelay(uint16_t& delayMS) const;  

  bool RecordingWarning() const;
  bool RecordingError() const;
  void ClearRecordingWarning();
  void ClearRecordingError();

  
  void AttachAudioBuffer(AudioDeviceBuffer* audioBuffer);

  int32_t SetRecordingSampleRate(const uint32_t samplesPerSec);

 private:
  void Lock() EXCLUSIVE_LOCK_FUNCTION(_critSect) {
    _critSect.Enter();
  }
  void UnLock() UNLOCK_FUNCTION(_critSect) {
    _critSect.Leave();
  }

  int32_t InitJavaResources();
  int32_t InitSampleRate();

  static bool RecThreadFunc(void*);
  bool RecThreadProcess();

  
  
  
  static JavaVM* globalJvm;
  static JNIEnv* globalJNIEnv;
  static jobject globalContext;
  static jclass globalScClass;

  JavaVM* _javaVM; 
  JNIEnv* _jniEnvRec; 
  jclass _javaScClass; 
  jobject _javaScObj; 
  jobject _javaRecBuffer;
  void* _javaDirectRecBuffer; 
  jmethodID _javaMidRecAudio; 

  AudioDeviceBuffer* _ptrAudioBuffer;
  CriticalSectionWrapper& _critSect;
  int32_t _id;
  PlayoutDelayProvider* _delay_provider;
  bool _initialized;

  EventWrapper& _timeEventRec;
  EventWrapper& _recStartStopEvent;
  ThreadWrapper* _ptrThreadRec;
  uint32_t _recThreadID;
  bool _recThreadIsInitialized;
  bool _shutdownRecThread;

  int8_t _recBuffer[2 * REC_BUF_SIZE_IN_SAMPLES];
  bool _recordingDeviceIsSpecified;

  bool _recording;
  bool _recIsInitialized;
  bool _micIsInitialized;

  bool _startRec;

  uint16_t _recWarning;
  uint16_t _recError;

  uint16_t _delayRecording;

  bool _AGC;

  uint16_t _samplingFreqIn; 
  int _recAudioSource;

};

}  

#endif  

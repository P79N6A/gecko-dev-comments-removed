









#ifndef WEBRTC_MODULES_AUDIO_DEVICE_ANDROID_AUDIO_TRACK_JNI_H_
#define WEBRTC_MODULES_AUDIO_DEVICE_ANDROID_AUDIO_TRACK_JNI_H_

#include <jni.h>

#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/modules/audio_device/android/audio_common.h"
#include "webrtc/modules/audio_device/include/audio_device_defines.h"
#include "webrtc/modules/audio_device/audio_device_generic.h"

namespace webrtc {

class EventWrapper;
class ThreadWrapper;

const uint32_t N_PLAY_SAMPLES_PER_SEC = 16000; 
const uint32_t N_PLAY_CHANNELS = 1; 

class AudioTrackJni : public PlayoutDelayProvider {
 public:
  static int32_t SetAndroidAudioDeviceObjects(void* javaVM, void* env,
                                              void* context);
  static void ClearAndroidAudioDeviceObjects();
  explicit AudioTrackJni(const int32_t id);
  virtual ~AudioTrackJni();

  
  int32_t Init();
  int32_t Terminate();
  bool Initialized() const { return _initialized; }

  
  int16_t PlayoutDevices() { return 1; }  

  int32_t PlayoutDeviceName(uint16_t index,
                            char name[kAdmMaxDeviceNameSize],
                            char guid[kAdmMaxGuidSize]);

  
  int32_t SetPlayoutDevice(uint16_t index);
  int32_t SetPlayoutDevice(
      AudioDeviceModule::WindowsDeviceType device);

  
  int32_t PlayoutIsAvailable(bool& available);  
  int32_t InitPlayout();
  bool PlayoutIsInitialized() const { return _playIsInitialized; }

  
  int32_t StartPlayout();
  int32_t StopPlayout();
  bool Playing() const { return _playing; }

  
  int32_t SpeakerIsAvailable(bool& available);  
  int32_t InitSpeaker();
  bool SpeakerIsInitialized() const { return _speakerIsInitialized; }

  
  int32_t SpeakerVolumeIsAvailable(bool& available);  
  int32_t SetSpeakerVolume(uint32_t volume);
  int32_t SpeakerVolume(uint32_t& volume) const;  
  int32_t MaxSpeakerVolume(uint32_t& maxVolume) const;  
  int32_t MinSpeakerVolume(uint32_t& minVolume) const;  
  int32_t SpeakerVolumeStepSize(uint16_t& stepSize) const;  

  
  int32_t SpeakerMuteIsAvailable(bool& available);  
  int32_t SetSpeakerMute(bool enable);
  int32_t SpeakerMute(bool& enabled) const;  


  
  int32_t StereoPlayoutIsAvailable(bool& available);  
  int32_t SetStereoPlayout(bool enable);
  int32_t StereoPlayout(bool& enabled) const;  

  
  int32_t SetPlayoutBuffer(const AudioDeviceModule::BufferType type,
                           uint16_t sizeMS);
  int32_t PlayoutBuffer(AudioDeviceModule::BufferType& type,  
                        uint16_t& sizeMS) const;
  int32_t PlayoutDelay(uint16_t& delayMS) const;  

  
  void AttachAudioBuffer(AudioDeviceBuffer* audioBuffer);

  int32_t SetPlayoutSampleRate(const uint32_t samplesPerSec);

  
  bool PlayoutWarning() const;
  bool PlayoutError() const;
  void ClearPlayoutWarning();
  void ClearPlayoutError();

  
  int32_t SetLoudspeakerStatus(bool enable);
  int32_t GetLoudspeakerStatus(bool& enable) const;  

 protected:
  virtual int PlayoutDelayMs() { return 0; }

 private:
  void Lock() EXCLUSIVE_LOCK_FUNCTION(_critSect) {
    _critSect.Enter();
  }
  void UnLock() UNLOCK_FUNCTION(_critSect) {
    _critSect.Leave();
  }

  int32_t InitJavaResources();
  int32_t InitSampleRate();

  static bool PlayThreadFunc(void*);
  bool PlayThreadProcess();

  
  
  
  static JavaVM* globalJvm;
  static JNIEnv* globalJNIEnv;
  static jobject globalContext;
  static jclass globalScClass;

  JavaVM* _javaVM; 
  JNIEnv* _jniEnvPlay; 
  jclass _javaScClass; 
  jobject _javaScObj; 
  jobject _javaPlayBuffer;
  void* _javaDirectPlayBuffer; 
  jmethodID _javaMidPlayAudio; 

  AudioDeviceBuffer* _ptrAudioBuffer;
  CriticalSectionWrapper& _critSect;
  int32_t _id;
  bool _initialized;

  EventWrapper& _timeEventPlay;
  EventWrapper& _playStartStopEvent;
  ThreadWrapper* _ptrThreadPlay;
  uint32_t _playThreadID;
  bool _playThreadIsInitialized;
  bool _shutdownPlayThread;
  bool _playoutDeviceIsSpecified;

  bool _playing;
  bool _playIsInitialized;
  bool _speakerIsInitialized;

  bool _startPlay;

  uint16_t _playWarning;
  uint16_t _playError;

  uint16_t _delayPlayout;

  uint16_t _samplingFreqOut; 
  uint32_t _maxSpeakerVolume; 
  bool _loudSpeakerOn;

};

}  

#endif  

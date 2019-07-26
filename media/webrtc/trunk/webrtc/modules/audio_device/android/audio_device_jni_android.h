













#ifndef WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_JNI_ANDROID_H
#define WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_JNI_ANDROID_H

#include "audio_device_generic.h"
#include "critical_section_wrapper.h"

#include <jni.h> 

namespace webrtc
{
class EventWrapper;

const WebRtc_UWord32 N_REC_SAMPLES_PER_SEC = 16000; 
const WebRtc_UWord32 N_PLAY_SAMPLES_PER_SEC = 16000; 

const WebRtc_UWord32 N_REC_CHANNELS = 1; 
const WebRtc_UWord32 N_PLAY_CHANNELS = 1; 

const WebRtc_UWord32 REC_BUF_SIZE_IN_SAMPLES = 480; 


class ThreadWrapper;

class AudioDeviceAndroidJni : public AudioDeviceGeneric {
 public:
  AudioDeviceAndroidJni(const WebRtc_Word32 id);
  ~AudioDeviceAndroidJni();

  static WebRtc_Word32 SetAndroidAudioDeviceObjects(void* javaVM,
                                                    void* env,
                                                    void* context);

  virtual WebRtc_Word32 ActiveAudioLayer(
      AudioDeviceModule::AudioLayer& audioLayer) const;

  virtual WebRtc_Word32 Init();
  virtual WebRtc_Word32 Terminate();
  virtual bool Initialized() const;

  virtual WebRtc_Word16 PlayoutDevices();
  virtual WebRtc_Word16 RecordingDevices();
  virtual WebRtc_Word32 PlayoutDeviceName(WebRtc_UWord16 index,
                                          char name[kAdmMaxDeviceNameSize],
                                          char guid[kAdmMaxGuidSize]);
  virtual WebRtc_Word32 RecordingDeviceName(
      WebRtc_UWord16 index,
      char name[kAdmMaxDeviceNameSize],
      char guid[kAdmMaxGuidSize]);

  virtual WebRtc_Word32 SetPlayoutDevice(WebRtc_UWord16 index);
  virtual WebRtc_Word32 SetPlayoutDevice(
      AudioDeviceModule::WindowsDeviceType device);
  virtual WebRtc_Word32 SetRecordingDevice(WebRtc_UWord16 index);
  virtual WebRtc_Word32 SetRecordingDevice(
      AudioDeviceModule::WindowsDeviceType device);

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

  virtual WebRtc_Word32 SetWaveOutVolume(WebRtc_UWord16 volumeLeft,
                                         WebRtc_UWord16 volumeRight);
  virtual WebRtc_Word32 WaveOutVolume(WebRtc_UWord16& volumeLeft,
                                      WebRtc_UWord16& volumeRight) const;

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
  virtual WebRtc_Word32 MicrophoneVolumeStepSize(
      WebRtc_UWord16& stepSize) const;

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

  virtual WebRtc_Word32 SetPlayoutBuffer(
      const AudioDeviceModule::BufferType type, WebRtc_UWord16 sizeMS);
  virtual WebRtc_Word32 PlayoutBuffer(
      AudioDeviceModule::BufferType& type, WebRtc_UWord16& sizeMS) const;
  virtual WebRtc_Word32 PlayoutDelay(WebRtc_UWord16& delayMS) const;
  virtual WebRtc_Word32 RecordingDelay(WebRtc_UWord16& delayMS) const;

  virtual WebRtc_Word32 CPULoad(WebRtc_UWord16& load) const;

  virtual bool PlayoutWarning() const;
  virtual bool PlayoutError() const;
  virtual bool RecordingWarning() const;
  virtual bool RecordingError() const;
  virtual void ClearPlayoutWarning();
  virtual void ClearPlayoutError();
  virtual void ClearRecordingWarning();
  virtual void ClearRecordingError();

  virtual void AttachAudioBuffer(AudioDeviceBuffer* audioBuffer);

  virtual WebRtc_Word32 SetRecordingSampleRate(
      const WebRtc_UWord32 samplesPerSec);
  virtual WebRtc_Word32 SetPlayoutSampleRate(
      const WebRtc_UWord32 samplesPerSec);

  virtual WebRtc_Word32 SetLoudspeakerStatus(bool enable);
  virtual WebRtc_Word32 GetLoudspeakerStatus(bool& enable) const;

 private:
  
  void Lock() {
    _critSect.Enter();
  };
  void UnLock() {
    _critSect.Leave();
  };

  
  WebRtc_Word32 InitJavaResources();
  WebRtc_Word32 InitSampleRate();

  
  static bool RecThreadFunc(void*);
  static bool PlayThreadFunc(void*);
  bool RecThreadProcess();
  bool PlayThreadProcess();

  
  AudioDeviceBuffer* _ptrAudioBuffer;
  CriticalSectionWrapper& _critSect;
  WebRtc_Word32 _id;

  
  EventWrapper& _timeEventRec;
  EventWrapper& _timeEventPlay;
  EventWrapper& _recStartStopEvent;
  EventWrapper& _playStartStopEvent;

  
  ThreadWrapper* _ptrThreadPlay;
  ThreadWrapper* _ptrThreadRec;
  WebRtc_UWord32 _recThreadID;
  WebRtc_UWord32 _playThreadID;
  bool _playThreadIsInitialized;
  bool _recThreadIsInitialized;
  bool _shutdownPlayThread;
  bool _shutdownRecThread;

  
  WebRtc_Word8 _recBuffer[2 * REC_BUF_SIZE_IN_SAMPLES];

  
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
  bool _stopRec;
  bool _startPlay;
  bool _stopPlay;

  
  WebRtc_UWord16 _playWarning;
  WebRtc_UWord16 _playError;
  WebRtc_UWord16 _recWarning;
  WebRtc_UWord16 _recError;

  
  WebRtc_UWord16 _delayPlayout;
  WebRtc_UWord16 _delayRecording;

  
  bool _AGC;

  
  WebRtc_UWord16 _samplingFreqIn; 
  WebRtc_UWord16 _samplingFreqOut; 
  WebRtc_UWord32 _maxSpeakerVolume; 
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

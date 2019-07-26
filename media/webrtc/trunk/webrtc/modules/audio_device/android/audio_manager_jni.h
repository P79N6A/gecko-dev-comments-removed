












#ifndef WEBRTC_MODULES_AUDIO_DEVICE_ANDROID_AUDIO_MANAGER_JNI_H_
#define WEBRTC_MODULES_AUDIO_DEVICE_ANDROID_AUDIO_MANAGER_JNI_H_

#include <jni.h>

namespace webrtc {

class AudioManagerJni {
 public:
  AudioManagerJni();
  ~AudioManagerJni() {}

  
  
  
  
  
  
  
  
  
  
  
  static void SetAndroidAudioDeviceObjects(void* jvm, void* env,
                                           void* context);
  
  
  
  static void ClearAndroidAudioDeviceObjects();

  bool low_latency_supported() const { return low_latency_supported_; }
  int native_output_sample_rate() const { return native_output_sample_rate_; }
  int native_buffer_size() const { return native_buffer_size_; }

 private:
  bool HasDeviceObjects();

  
  void SetLowLatencySupported(JNIEnv* env);
  void SetNativeOutputSampleRate(JNIEnv* env);
  void SetNativeFrameSize(JNIEnv* env);

  jmethodID LookUpMethodId(JNIEnv* env, const char* method_name,
                           const char* method_signature);

  void CreateInstance(JNIEnv* env);

  
  
  
  bool low_latency_supported_;
  int native_output_sample_rate_;
  int native_buffer_size_;
};

}  

#endif  

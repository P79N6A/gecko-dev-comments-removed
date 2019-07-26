









#ifndef WEBRTC_MODULES_AUDIO_DEVICE_ANDROID_OPENSLES_INPUT_H_
#define WEBRTC_MODULES_AUDIO_DEVICE_ANDROID_OPENSLES_INPUT_H_

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <SLES/OpenSLES_AndroidConfiguration.h>

#include "webrtc/modules/audio_device/android/audio_manager_jni.h"
#include "webrtc/modules/audio_device/android/low_latency_event.h"
#include "webrtc/modules/audio_device/android/opensles_common.h"
#include "webrtc/modules/audio_device/include/audio_device.h"
#include "webrtc/modules/audio_device/include/audio_device_defines.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

class AudioDeviceBuffer;
class CriticalSectionWrapper;
class PlayoutDelayProvider;
class SingleRwFifo;
class ThreadWrapper;





class OpenSlesInput {
 public:
  OpenSlesInput(const int32_t id,
                webrtc_opensl::PlayoutDelayProvider* delay_provider);
  ~OpenSlesInput();

  
  int32_t Init();
  int32_t Terminate();
  bool Initialized() const { return initialized_; }

  
  int16_t RecordingDevices() { return 1; }
  int32_t RecordingDeviceName(uint16_t index,
                              char name[kAdmMaxDeviceNameSize],
                              char guid[kAdmMaxGuidSize]);

  
  int32_t SetRecordingDevice(uint16_t index);
  int32_t SetRecordingDevice(
      AudioDeviceModule::WindowsDeviceType device) { return -1; }

  
  int32_t RecordingIsAvailable(bool& available);  
  int32_t InitRecording();
  bool RecordingIsInitialized() const { return rec_initialized_; }

  
  int32_t StartRecording();
  int32_t StopRecording();
  bool Recording() const { return recording_; }

  
  int32_t SetAGC(bool enable);
  bool AGC() const { return agc_enabled_; }

  
  int32_t MicrophoneIsAvailable(bool& available);  
  int32_t InitMicrophone();
  bool MicrophoneIsInitialized() const { return mic_initialized_; }

  
  int32_t MicrophoneVolumeIsAvailable(bool& available);  
  
  
  int32_t SetMicrophoneVolume(uint32_t volume) { return 0; }
  int32_t MicrophoneVolume(uint32_t& volume) const { return -1; }  
  int32_t MaxMicrophoneVolume(
      uint32_t& maxVolume) const { return 0; }  
  int32_t MinMicrophoneVolume(uint32_t& minVolume) const;  
  int32_t MicrophoneVolumeStepSize(
      uint16_t& stepSize) const;  

  
  int32_t MicrophoneMuteIsAvailable(bool& available);  
  int32_t SetMicrophoneMute(bool enable) { return -1; }
  int32_t MicrophoneMute(bool& enabled) const { return -1; }  

  
  int32_t MicrophoneBoostIsAvailable(bool& available);  
  int32_t SetMicrophoneBoost(bool enable);
  int32_t MicrophoneBoost(bool& enabled) const;  

  
  int32_t StereoRecordingIsAvailable(bool& available);  
  int32_t SetStereoRecording(bool enable) { return -1; }
  int32_t StereoRecording(bool& enabled) const;  

  
  int32_t RecordingDelay(uint16_t& delayMS) const;  

  bool RecordingWarning() const { return false; }
  bool RecordingError() const  { return false; }
  void ClearRecordingWarning() {}
  void ClearRecordingError() {}

  
  void AttachAudioBuffer(AudioDeviceBuffer* audioBuffer);

 private:
  enum {
    kNumInterfaces = 2,
    
    
    kNumOpenSlBuffers = 2,
    kNum10MsToBuffer = 3,
  };

  int InitSampleRate();
  int buffer_size_samples() const;
  int buffer_size_bytes() const;
  void UpdateRecordingDelay();
  void UpdateSampleRate();
  void CalculateNumFifoBuffersNeeded();
  void AllocateBuffers();
  int TotalBuffersUsed() const;
  bool EnqueueAllBuffers();
  
  
  bool CreateAudioRecorder();
  void DestroyAudioRecorder();

  
  
  
  
  
  
  
  
  
  bool HandleOverrun(int event_id, int event_msg);

  static void RecorderSimpleBufferQueueCallback(
      SLAndroidSimpleBufferQueueItf queueItf,
      void* pContext);
  
  
  
  
  void RecorderSimpleBufferQueueCallbackHandler(
      SLAndroidSimpleBufferQueueItf queueItf);

  bool StartCbThreads();
  void StopCbThreads();
  static bool CbThread(void* context);
  
  
  
  bool CbThreadImpl();

  
  AudioManagerJni audio_manager_;

  int id_;
  webrtc_opensl::PlayoutDelayProvider* delay_provider_;
  bool initialized_;
  bool mic_initialized_;
  bool rec_initialized_;

  
  
  scoped_ptr<ThreadWrapper> rec_thread_;  
  scoped_ptr<CriticalSectionWrapper> crit_sect_;
  
  
  bool recording_;

  
  
  
  scoped_ptr<SingleRwFifo> fifo_;
  int num_fifo_buffers_needed_;
  LowLatencyEvent event_;
  int number_overruns_;

  
  SLObjectItf sles_engine_;
  SLEngineItf sles_engine_itf_;
  SLObjectItf sles_recorder_;
  SLRecordItf sles_recorder_itf_;
  SLAndroidSimpleBufferQueueItf sles_recorder_sbq_itf_;

  
  AudioDeviceBuffer* audio_buffer_;
  
  scoped_array<scoped_array<int8_t> > rec_buf_;
  
  
  
  int active_queue_;

  
  uint32_t rec_sampling_rate_;
  bool agc_enabled_;

  
  uint16_t recording_delay_;
};

}  

#endif

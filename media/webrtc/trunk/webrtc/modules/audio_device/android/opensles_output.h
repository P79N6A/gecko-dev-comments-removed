









#ifndef WEBRTC_MODULES_AUDIO_DEVICE_ANDROID_OPENSLES_OUTPUT_H_
#define WEBRTC_MODULES_AUDIO_DEVICE_ANDROID_OPENSLES_OUTPUT_H_

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <SLES/OpenSLES_AndroidConfiguration.h>

#include "webrtc/modules/audio_device/android/audio_manager_jni.h"
#include "webrtc/modules/audio_device/android/low_latency_event.h"
#include "webrtc/modules/audio_device/android/opensles_common.h"
#include "webrtc/modules/audio_device/include/audio_device_defines.h"
#include "webrtc/modules/audio_device/include/audio_device.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

class AudioDeviceBuffer;
class CriticalSectionWrapper;
class FineAudioBuffer;
class SingleRwFifo;
class ThreadWrapper;




class OpenSlesOutput : public webrtc_opensl::PlayoutDelayProvider {
 public:
  explicit OpenSlesOutput(const int32_t id);
  virtual ~OpenSlesOutput();

  
  int32_t Init();
  int32_t Terminate();
  bool Initialized() const { return initialized_; }

  
  int16_t PlayoutDevices() { return 1; }

  int32_t PlayoutDeviceName(uint16_t index,
                            char name[kAdmMaxDeviceNameSize],
                            char guid[kAdmMaxGuidSize]);

  
  int32_t SetPlayoutDevice(uint16_t index);
  int32_t SetPlayoutDevice(
      AudioDeviceModule::WindowsDeviceType device) { return 0; }

  
  int32_t PlayoutIsAvailable(bool& available);  
  int32_t InitPlayout();
  bool PlayoutIsInitialized() const { return play_initialized_; }

  
  int32_t StartPlayout();
  int32_t StopPlayout();
  bool Playing() const { return playing_; }

  
  int32_t SpeakerIsAvailable(bool& available);  
  int32_t InitSpeaker();
  bool SpeakerIsInitialized() const { return speaker_initialized_; }

  
  int32_t SpeakerVolumeIsAvailable(bool& available);  
  int32_t SetSpeakerVolume(uint32_t volume);
  int32_t SpeakerVolume(uint32_t& volume) const { return 0; }  
  int32_t MaxSpeakerVolume(uint32_t& maxVolume) const;  
  int32_t MinSpeakerVolume(uint32_t& minVolume) const;  
  int32_t SpeakerVolumeStepSize(uint16_t& stepSize) const;  

  
  int32_t SpeakerMuteIsAvailable(bool& available);  
  int32_t SetSpeakerMute(bool enable) { return -1; }
  int32_t SpeakerMute(bool& enabled) const { return -1; }  


  
  int32_t StereoPlayoutIsAvailable(bool& available);  
  int32_t SetStereoPlayout(bool enable);
  int32_t StereoPlayout(bool& enabled) const;  

  
  int32_t SetPlayoutBuffer(const AudioDeviceModule::BufferType type,
                                   uint16_t sizeMS) { return -1; }
  int32_t PlayoutBuffer(AudioDeviceModule::BufferType& type,  
                        uint16_t& sizeMS) const;
  int32_t PlayoutDelay(uint16_t& delayMS) const;  


  
  bool PlayoutWarning() const { return false; }
  bool PlayoutError() const { return false; }
  void ClearPlayoutWarning() {}
  void ClearPlayoutError() {}

  
  void AttachAudioBuffer(AudioDeviceBuffer* audioBuffer);

  
  int32_t SetLoudspeakerStatus(bool enable);
  int32_t GetLoudspeakerStatus(bool& enable) const;  

 protected:
  virtual int PlayoutDelayMs();

 private:
  enum {
    kNumInterfaces = 3,
    
    
    
    
    
    
    
    
    kNumOpenSlBuffers = 2,
    
    
    
    
    
    
    
    
    kNum10MsToBuffer = 4,
  };

  bool InitSampleRate();
  bool SetLowLatency();
  void UpdatePlayoutDelay();
  
  
  
  
  void CalculateNumFifoBuffersNeeded();
  void AllocateBuffers();
  int TotalBuffersUsed() const;
  bool EnqueueAllBuffers();
  
  
  bool CreateAudioPlayer();
  void DestroyAudioPlayer();

  
  
  
  
  
  
  
  
  
  bool HandleUnderrun(int event_id, int event_msg);

  static void PlayerSimpleBufferQueueCallback(
      SLAndroidSimpleBufferQueueItf queueItf,
      void* pContext);
  
  
  
  
  void PlayerSimpleBufferQueueCallbackHandler(
      SLAndroidSimpleBufferQueueItf queueItf);

  bool StartCbThreads();
  void StopCbThreads();
  static bool CbThread(void* context);
  
  
  
  bool CbThreadImpl();

  
  AudioManagerJni audio_manager_;

  int id_;
  bool initialized_;
  bool speaker_initialized_;
  bool play_initialized_;

  
  
  scoped_ptr<ThreadWrapper> play_thread_;  
  scoped_ptr<CriticalSectionWrapper> crit_sect_;
  
  
  bool playing_;

  
  
  
  scoped_ptr<SingleRwFifo> fifo_;
  int num_fifo_buffers_needed_;
  LowLatencyEvent event_;
  int number_underruns_;

  
  SLObjectItf sles_engine_;
  SLEngineItf sles_engine_itf_;
  SLObjectItf sles_player_;
  SLPlayItf sles_player_itf_;
  SLAndroidSimpleBufferQueueItf sles_player_sbq_itf_;
  SLObjectItf sles_output_mixer_;

  
  AudioDeviceBuffer* audio_buffer_;
  scoped_ptr<FineAudioBuffer> fine_buffer_;
  scoped_array<scoped_array<int8_t> > play_buf_;
  
  
  
  int active_queue_;

  
  uint32_t speaker_sampling_rate_;
  int buffer_size_samples_;
  int buffer_size_bytes_;

  
  uint16_t playout_delay_;
};

}  

#endif

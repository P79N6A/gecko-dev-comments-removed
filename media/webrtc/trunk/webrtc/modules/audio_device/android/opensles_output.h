









#ifndef WEBRTC_MODULES_AUDIO_DEVICE_ANDROID_OPENSLES_OUTPUT_H_
#define WEBRTC_MODULES_AUDIO_DEVICE_ANDROID_OPENSLES_OUTPUT_H_

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <SLES/OpenSLES_AndroidConfiguration.h>

#if !defined(WEBRTC_GONK)
#include "webrtc/modules/audio_device/android/audio_manager_jni.h"
#endif
#include "webrtc/modules/audio_device/android/low_latency_event.h"
#include "webrtc/modules/audio_device/android/audio_common.h"
#include "webrtc/modules/audio_device/include/audio_device_defines.h"
#include "webrtc/modules/audio_device/include/audio_device.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

class AudioDeviceBuffer;
class CriticalSectionWrapper;
class FineAudioBuffer;
class SingleRwFifo;
class ThreadWrapper;

#ifdef WEBRTC_ANDROID_OPENSLES_OUTPUT





class OpenSlesOutput : public PlayoutDelayProvider {
 public:
  explicit OpenSlesOutput(const int32_t id);
  virtual ~OpenSlesOutput();

  static int32_t SetAndroidAudioDeviceObjects(void* javaVM,
                                              void* env,
                                              void* context);
  static void ClearAndroidAudioDeviceObjects();

  
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

  
  int32_t SetPlayoutSampleRate(uint32_t sample_rate_hz) { return 0; }

  
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
    
    
    
    
    
    
    
    
    kNum10MsToBuffer = 6,
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

#if !defined(WEBRTC_GONK)
  
  AudioManagerJni audio_manager_;
#endif

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

  
  void *opensles_lib_;
  typedef SLresult (*slCreateEngine_t)(SLObjectItf *,
                                       SLuint32,
                                       const SLEngineOption *,
                                       SLuint32,
                                       const SLInterfaceID *,
                                       const SLboolean *);
  slCreateEngine_t f_slCreateEngine;
  SLInterfaceID SL_IID_ENGINE_;
  SLInterfaceID SL_IID_BUFFERQUEUE_;
  SLInterfaceID SL_IID_ANDROIDCONFIGURATION_;
  SLInterfaceID SL_IID_PLAY_;
  SLInterfaceID SL_IID_ANDROIDSIMPLEBUFFERQUEUE_;
  SLInterfaceID SL_IID_VOLUME_;
};

#else


class OpenSlesOutput : public PlayoutDelayProvider {
 public:
  explicit OpenSlesOutput(const int32_t id) :
    initialized_(false), speaker_initialized_(false),
    play_initialized_(false), playing_(false)
  {}
  virtual ~OpenSlesOutput() {}

  static int32_t SetAndroidAudioDeviceObjects(void* javaVM,
                                              void* env,
                                              void* context) { return 0; }
  static void ClearAndroidAudioDeviceObjects() {}

  
  int32_t Init() { initialized_ = true; return 0; }
  int32_t Terminate() { initialized_ = false; return 0; }
  bool Initialized() const { return initialized_; }

  
  int16_t PlayoutDevices() { return 1; }

  int32_t PlayoutDeviceName(uint16_t index,
                            char name[kAdmMaxDeviceNameSize],
                            char guid[kAdmMaxGuidSize])
  {
    assert(index == 0);
    
    name[0] = '\0';
    guid[0] = '\0';
    return 0;
  }

  
  int32_t SetPlayoutDevice(uint16_t index)
  {
    assert(index == 0);
    return 0;
  }
  int32_t SetPlayoutDevice(
      AudioDeviceModule::WindowsDeviceType device) { return 0; }

  
  int32_t SetPlayoutSampleRate(uint32_t sample_rate_hz) { return 0; }

  
  int32_t PlayoutIsAvailable(bool& available)  
  {
    available = true;
    return 0;
  }
  int32_t InitPlayout()
  {
    assert(initialized_);
    play_initialized_ = true;
    return 0;
  }
  bool PlayoutIsInitialized() const { return play_initialized_; }

  
  int32_t StartPlayout()
  {
    assert(play_initialized_);
    assert(!playing_);
    playing_ = true;
    return 0;
  }

  int32_t StopPlayout()
  {
    playing_ = false;
    return 0;
  }

  bool Playing() const { return playing_; }

  
  int32_t SpeakerIsAvailable(bool& available)  
  {
    available = true;
    return 0;
  }
  int32_t InitSpeaker()
  {
    assert(!playing_);
    speaker_initialized_ = true;
    return 0;
  }
  bool SpeakerIsInitialized() const { return speaker_initialized_; }

  
  int32_t SpeakerVolumeIsAvailable(bool& available)  
  {
    available = true;
    return 0;
  }
  int32_t SetSpeakerVolume(uint32_t volume)
  {
    assert(speaker_initialized_);
    assert(initialized_);
    return 0;
  }
  int32_t SpeakerVolume(uint32_t& volume) const { return 0; }  
  int32_t MaxSpeakerVolume(uint32_t& maxVolume) const  
  {
    assert(speaker_initialized_);
    assert(initialized_);
    maxVolume = 0;
    return 0;
  }
  int32_t MinSpeakerVolume(uint32_t& minVolume) const  
  {
    assert(speaker_initialized_);
    assert(initialized_);
    minVolume = 0;
    return 0;
  }
  int32_t SpeakerVolumeStepSize(uint16_t& stepSize) const  
  {
    assert(speaker_initialized_);
    assert(initialized_);
    stepSize = 0;
    return 0;
  }

  
  int32_t SpeakerMuteIsAvailable(bool& available)  
  {
    available = true;
    return 0;
  }
  int32_t SetSpeakerMute(bool enable) { return -1; }
  int32_t SpeakerMute(bool& enabled) const { return -1; }  


  
  int32_t StereoPlayoutIsAvailable(bool& available)  
  {
    available = true;
    return 0;
  }
  int32_t SetStereoPlayout(bool enable)
  {
    return 0;
  }
  int32_t StereoPlayout(bool& enabled) const  
  {
    enabled = kNumChannels == 2;
    return 0;
  }

  
  int32_t SetPlayoutBuffer(const AudioDeviceModule::BufferType type,
                                   uint16_t sizeMS) { return -1; }
  int32_t PlayoutBuffer(AudioDeviceModule::BufferType& type,  
                        uint16_t& sizeMS) const
  {
    type = AudioDeviceModule::kAdaptiveBufferSize;
    sizeMS = 40;
    return 0;
  }
  int32_t PlayoutDelay(uint16_t& delayMS) const  
  {
    delayMS = 0;
    return 0;
  }


  
  bool PlayoutWarning() const { return false; }
  bool PlayoutError() const { return false; }
  void ClearPlayoutWarning() {}
  void ClearPlayoutError() {}

  
  void AttachAudioBuffer(AudioDeviceBuffer* audioBuffer) {}

  
  int32_t SetLoudspeakerStatus(bool enable) { return 0; }
  int32_t GetLoudspeakerStatus(bool& enable) const { enable = true; return 0; }  

 protected:
  virtual int PlayoutDelayMs() { return 40; }

 private:
  bool initialized_;
  bool speaker_initialized_;
  bool play_initialized_;
  bool playing_;
};
#endif

}  

#endif  

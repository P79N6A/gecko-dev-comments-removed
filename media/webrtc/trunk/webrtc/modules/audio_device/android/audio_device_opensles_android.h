









#ifndef SRC_MODULES_AUDIO_DEVICE_ANDROID_AUDIO_DEVICE_OPENSLES_ANDROID_H_
#define SRC_MODULES_AUDIO_DEVICE_ANDROID_AUDIO_DEVICE_OPENSLES_ANDROID_H_

#include <jni.h>
#include <stdio.h>
#include <stdlib.h>

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <SLES/OpenSLES_AndroidConfiguration.h>

#include <queue>

#include "modules/audio_device/audio_device_generic.h"
#include "system_wrappers/interface/critical_section_wrapper.h"

namespace webrtc {

class EventWrapper;

const WebRtc_UWord32 N_MAX_INTERFACES = 3;
const WebRtc_UWord32 N_MAX_OUTPUT_DEVICES = 6;
const WebRtc_UWord32 N_MAX_INPUT_DEVICES = 3;

const WebRtc_UWord32 N_REC_SAMPLES_PER_SEC = 16000;  
const WebRtc_UWord32 N_PLAY_SAMPLES_PER_SEC = 16000;  

const WebRtc_UWord32 N_REC_CHANNELS = 1;
const WebRtc_UWord32 N_PLAY_CHANNELS = 1;

const WebRtc_UWord32 REC_BUF_SIZE_IN_SAMPLES = 480;
const WebRtc_UWord32 PLAY_BUF_SIZE_IN_SAMPLES = 480;

const WebRtc_UWord32 REC_MAX_TEMP_BUF_SIZE_PER_10ms =
    N_REC_CHANNELS * REC_BUF_SIZE_IN_SAMPLES * sizeof(int16_t);

const WebRtc_UWord32 PLAY_MAX_TEMP_BUF_SIZE_PER_10ms =
    N_PLAY_CHANNELS * PLAY_BUF_SIZE_IN_SAMPLES * sizeof(int16_t);


const WebRtc_UWord16 N_PLAY_QUEUE_BUFFERS = 8;


const WebRtc_UWord16 N_REC_QUEUE_BUFFERS = 16;






class ThreadWrapper;

class AudioDeviceAndroidOpenSLES: public AudioDeviceGeneric {
 public:
  explicit AudioDeviceAndroidOpenSLES(const WebRtc_Word32 id);
  ~AudioDeviceAndroidOpenSLES();

  
  virtual WebRtc_Word32
  ActiveAudioLayer(AudioDeviceModule::AudioLayer& audioLayer) const;  

  
  virtual WebRtc_Word32 Init();
  virtual WebRtc_Word32 Terminate();
  virtual bool Initialized() const;

  
  virtual WebRtc_Word16 PlayoutDevices();
  virtual WebRtc_Word16 RecordingDevices();
  virtual WebRtc_Word32
  PlayoutDeviceName(WebRtc_UWord16 index,
                    char name[kAdmMaxDeviceNameSize],
                    char guid[kAdmMaxGuidSize]);
  virtual WebRtc_Word32
  RecordingDeviceName(WebRtc_UWord16 index,
                      char name[kAdmMaxDeviceNameSize],
                      char guid[kAdmMaxGuidSize]);

  
  virtual WebRtc_Word32 SetPlayoutDevice(WebRtc_UWord16 index);
  virtual WebRtc_Word32
  SetPlayoutDevice(AudioDeviceModule::WindowsDeviceType device);
  virtual WebRtc_Word32 SetRecordingDevice(WebRtc_UWord16 index);
  virtual WebRtc_Word32
  SetRecordingDevice(AudioDeviceModule::WindowsDeviceType device);

  
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
  virtual WebRtc_Word32 WaveOutVolume(
      WebRtc_UWord16& volumeLeft,  
      WebRtc_UWord16& volumeRight) const;  

  
  virtual WebRtc_Word32 SpeakerIsAvailable(bool& available);  
  virtual WebRtc_Word32 InitSpeaker();
  virtual bool SpeakerIsInitialized() const;
  virtual WebRtc_Word32 MicrophoneIsAvailable(
      bool& available);
  virtual WebRtc_Word32 InitMicrophone();
  virtual bool MicrophoneIsInitialized() const;

  
  virtual WebRtc_Word32 SpeakerVolumeIsAvailable(
      bool& available);  
  virtual WebRtc_Word32 SetSpeakerVolume(WebRtc_UWord32 volume);
  virtual WebRtc_Word32 SpeakerVolume(
      WebRtc_UWord32& volume) const;  
  virtual WebRtc_Word32 MaxSpeakerVolume(
      WebRtc_UWord32& maxVolume) const;  
  virtual WebRtc_Word32 MinSpeakerVolume(
      WebRtc_UWord32& minVolume) const;  
  virtual WebRtc_Word32 SpeakerVolumeStepSize(
      WebRtc_UWord16& stepSize) const;  

  
  virtual WebRtc_Word32 MicrophoneVolumeIsAvailable(
      bool& available);  
  virtual WebRtc_Word32 SetMicrophoneVolume(WebRtc_UWord32 volume);
  virtual WebRtc_Word32 MicrophoneVolume(
      WebRtc_UWord32& volume) const;  
  virtual WebRtc_Word32 MaxMicrophoneVolume(
      WebRtc_UWord32& maxVolume) const;  
  virtual WebRtc_Word32 MinMicrophoneVolume(
      WebRtc_UWord32& minVolume) const;  
  virtual WebRtc_Word32
  MicrophoneVolumeStepSize(WebRtc_UWord16& stepSize) const;  

  
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

  
  virtual WebRtc_Word32
  SetPlayoutBuffer(const AudioDeviceModule::BufferType type,
                   WebRtc_UWord16 sizeMS);
  virtual WebRtc_Word32 PlayoutBuffer(
      AudioDeviceModule::BufferType& type,  
      WebRtc_UWord16& sizeMS) const;
  virtual WebRtc_Word32 PlayoutDelay(
      WebRtc_UWord16& delayMS) const;  
  virtual WebRtc_Word32 RecordingDelay(
      WebRtc_UWord16& delayMS) const;  

  
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

  
  virtual WebRtc_Word32 SetLoudspeakerStatus(bool enable);
  virtual WebRtc_Word32 GetLoudspeakerStatus(bool& enable) const;  

 private:
  
  void Lock() {
    crit_sect_.Enter();
  };
  void UnLock() {
    crit_sect_.Leave();
  };

  static void PlayerSimpleBufferQueueCallback(
      SLAndroidSimpleBufferQueueItf queueItf,
      void *pContext);
  static void RecorderSimpleBufferQueueCallback(
      SLAndroidSimpleBufferQueueItf queueItf,
      void *pContext);
  void PlayerSimpleBufferQueueCallbackHandler(
      SLAndroidSimpleBufferQueueItf queueItf);
  void RecorderSimpleBufferQueueCallbackHandler(
      SLAndroidSimpleBufferQueueItf queueItf);
  void CheckErr(SLresult res);

  
  void UpdateRecordingDelay();
  void UpdatePlayoutDelay(WebRtc_UWord32 nSamplePlayed);

  
  WebRtc_Word32 InitSampleRate();

  
  AudioDeviceBuffer* voe_audio_buffer_;
  CriticalSectionWrapper& crit_sect_;
  WebRtc_Word32 id_;

  
  SLObjectItf sles_engine_;

  
  SLObjectItf sles_player_;
  SLEngineItf sles_engine_itf_;
  SLPlayItf sles_player_itf_;
  SLAndroidSimpleBufferQueueItf sles_player_sbq_itf_;
  SLObjectItf sles_output_mixer_;
  SLVolumeItf sles_speaker_volume_;

  
  SLObjectItf sles_recorder_;
  SLRecordItf sles_recorder_itf_;
  SLAndroidSimpleBufferQueueItf sles_recorder_sbq_itf_;
  SLDeviceVolumeItf sles_mic_volume_;
  WebRtc_UWord32 mic_dev_id_;

  WebRtc_UWord32 play_warning_, play_error_;
  WebRtc_UWord32 rec_warning_, rec_error_;

  
  bool is_recording_dev_specified_;
  bool is_playout_dev_specified_;
  bool is_initialized_;
  bool is_recording_;
  bool is_playing_;
  bool is_rec_initialized_;
  bool is_play_initialized_;
  bool is_mic_initialized_;
  bool is_speaker_initialized_;

  
  WebRtc_UWord16 playout_delay_;
  WebRtc_UWord16 recording_delay_;

  
  bool agc_enabled_;

  
  ThreadWrapper* rec_thread_;
  WebRtc_UWord32 rec_thread_id_;
  static bool RecThreadFunc(void* context);
  bool RecThreadFuncImpl();
  EventWrapper& rec_timer_;

  WebRtc_UWord32 mic_sampling_rate_;
  WebRtc_UWord32 speaker_sampling_rate_;
  WebRtc_UWord32 max_speaker_vol_;
  WebRtc_UWord32 min_speaker_vol_;
  bool loundspeaker_on_;

  SLDataFormat_PCM player_pcm_;
  SLDataFormat_PCM record_pcm_;

  std::queue<WebRtc_Word8*> rec_queue_;
  std::queue<WebRtc_Word8*> rec_voe_audio_queue_;
  std::queue<WebRtc_Word8*> rec_voe_ready_queue_;
  WebRtc_Word8 rec_buf_[N_REC_QUEUE_BUFFERS][
      N_REC_CHANNELS * sizeof(int16_t) * REC_BUF_SIZE_IN_SAMPLES];
  WebRtc_Word8 rec_voe_buf_[N_REC_QUEUE_BUFFERS][
      N_REC_CHANNELS * sizeof(int16_t) * REC_BUF_SIZE_IN_SAMPLES];

  std::queue<WebRtc_Word8*> play_queue_;
  WebRtc_Word8 play_buf_[N_PLAY_QUEUE_BUFFERS][
      N_PLAY_CHANNELS * sizeof(int16_t) * PLAY_BUF_SIZE_IN_SAMPLES];
};

}  

#endif  

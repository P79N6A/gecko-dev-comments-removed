









#include "webrtc/modules/audio_device/android/opensles_output.h"

#include <assert.h>

#include "webrtc/modules/audio_device/android/opensles_common.h"
#include "webrtc/modules/audio_device/android/fine_audio_buffer.h"
#include "webrtc/modules/audio_device/android/single_rw_fifo.h"
#include "webrtc/modules/audio_device/audio_device_buffer.h"
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/system_wrappers/interface/thread_wrapper.h"
#include "webrtc/system_wrappers/interface/trace.h"

#define VOID_RETURN
#define OPENSL_RETURN_ON_FAILURE(op, ret_val)                    \
  do {                                                           \
    SLresult err = (op);                                         \
    if (err != SL_RESULT_SUCCESS) {                              \
      WEBRTC_TRACE(kTraceError, kTraceAudioDevice, id_,          \
                   "OpenSL error: %d", err);                     \
      assert(false);                                             \
      return ret_val;                                            \
    }                                                            \
  } while (0)

static const SLEngineOption kOption[] = {
  { SL_ENGINEOPTION_THREADSAFE, static_cast<SLuint32>(SL_BOOLEAN_TRUE) },
};

enum {
  kNoUnderrun,
  kUnderrun,
};

namespace webrtc {

OpenSlesOutput::OpenSlesOutput(const int32_t id)
    : id_(id),
      initialized_(false),
      speaker_initialized_(false),
      play_initialized_(false),
      crit_sect_(CriticalSectionWrapper::CreateCriticalSection()),
      playing_(false),
      num_fifo_buffers_needed_(0),
      number_underruns_(0),
      sles_engine_(NULL),
      sles_engine_itf_(NULL),
      sles_player_(NULL),
      sles_player_itf_(NULL),
      sles_player_sbq_itf_(NULL),
      sles_output_mixer_(NULL),
      audio_buffer_(NULL),
      active_queue_(0),
      speaker_sampling_rate_(kDefaultSampleRate),
      buffer_size_samples_(0),
      buffer_size_bytes_(0),
      playout_delay_(0) {
}

OpenSlesOutput::~OpenSlesOutput() {
}

int32_t OpenSlesOutput::SetAndroidAudioDeviceObjects(void* javaVM,
                                                     void* env,
                                                     void* context) {
  AudioManagerJni::SetAndroidAudioDeviceObjects(javaVM, env, context);
  return 0;
}

void OpenSlesOutput::ClearAndroidAudioDeviceObjects() {
  AudioManagerJni::ClearAndroidAudioDeviceObjects();
}

int32_t OpenSlesOutput::Init() {
  assert(!initialized_);

  
  OPENSL_RETURN_ON_FAILURE(slCreateEngine(&sles_engine_, 1, kOption, 0,
                                          NULL, NULL),
                           -1);
  OPENSL_RETURN_ON_FAILURE((*sles_engine_)->Realize(sles_engine_,
                                                    SL_BOOLEAN_FALSE),
                           -1);
  OPENSL_RETURN_ON_FAILURE((*sles_engine_)->GetInterface(sles_engine_,
                                                         SL_IID_ENGINE,
                                                         &sles_engine_itf_),
                           -1);
  
  OPENSL_RETURN_ON_FAILURE(
      (*sles_engine_itf_)->CreateOutputMix(sles_engine_itf_,
                                           &sles_output_mixer_,
                                           0,
                                           NULL,
                                           NULL),
      -1);
  OPENSL_RETURN_ON_FAILURE(
      (*sles_output_mixer_)->Realize(sles_output_mixer_,
                                     SL_BOOLEAN_FALSE),
      -1);

  if (!InitSampleRate()) {
    return -1;
  }
  AllocateBuffers();
  initialized_ = true;
  return 0;
}

int32_t OpenSlesOutput::Terminate() {
  
  assert(!playing_);
  (*sles_output_mixer_)->Destroy(sles_output_mixer_);
  (*sles_engine_)->Destroy(sles_engine_);
  initialized_ = false;
  speaker_initialized_ = false;
  play_initialized_ = false;
  return 0;
}

int32_t OpenSlesOutput::PlayoutDeviceName(uint16_t index,
                                          char name[kAdmMaxDeviceNameSize],
                                          char guid[kAdmMaxGuidSize]) {
  assert(index == 0);
  
  name[0] = '\0';
  guid[0] = '\0';
  return 0;
}

int32_t OpenSlesOutput::SetPlayoutDevice(uint16_t index) {
  assert(index == 0);
  return 0;
}

int32_t OpenSlesOutput::PlayoutIsAvailable(bool& available) {  
  available = true;
  return 0;
}

int32_t OpenSlesOutput::InitPlayout() {
  assert(initialized_);
  play_initialized_ = true;
  return 0;
}

int32_t OpenSlesOutput::StartPlayout() {
  assert(play_initialized_);
  assert(!playing_);
  if (!CreateAudioPlayer()) {
    return -1;
  }

  
  OPENSL_RETURN_ON_FAILURE(
      (*sles_player_sbq_itf_)->RegisterCallback(sles_player_sbq_itf_,
                                                PlayerSimpleBufferQueueCallback,
                                                this),
      -1);
  if (!EnqueueAllBuffers()) {
    return -1;
  }

  {
    
    
    CriticalSectionScoped lock(crit_sect_.get());
    playing_ = true;
  }
  if (!StartCbThreads()) {
    playing_ = false;
  }
  return 0;
}

int32_t OpenSlesOutput::StopPlayout() {
  StopCbThreads();
  DestroyAudioPlayer();
  playing_ = false;
  return 0;
}

int32_t OpenSlesOutput::SpeakerIsAvailable(bool& available) {  
  available = true;
  return 0;
}

int32_t OpenSlesOutput::InitSpeaker() {
  assert(!playing_);
  speaker_initialized_ = true;
  return 0;
}

int32_t OpenSlesOutput::SpeakerVolumeIsAvailable(bool& available) {  
  available = true;
  return 0;
}

int32_t OpenSlesOutput::SetSpeakerVolume(uint32_t volume) {
  assert(speaker_initialized_);
  assert(initialized_);
  
  return 0;
}

int32_t OpenSlesOutput::MaxSpeakerVolume(uint32_t& maxVolume) const {  
  assert(speaker_initialized_);
  assert(initialized_);
  
  maxVolume = 0;
  return 0;
}

int32_t OpenSlesOutput::MinSpeakerVolume(uint32_t& minVolume) const {  
  assert(speaker_initialized_);
  assert(initialized_);
  
  minVolume = 0;
  return 0;
}

int32_t OpenSlesOutput::SpeakerVolumeStepSize(
    uint16_t& stepSize) const {  
  assert(speaker_initialized_);
  stepSize = 1;
  return 0;
}

int32_t OpenSlesOutput::SpeakerMuteIsAvailable(bool& available) {  
  available = false;
  return 0;
}

int32_t OpenSlesOutput::StereoPlayoutIsAvailable(bool& available) {  
  available = false;
  return 0;
}

int32_t OpenSlesOutput::SetStereoPlayout(bool enable) {
  if (enable) {
    assert(false);
    return -1;
  }
  return 0;
}

int32_t OpenSlesOutput::StereoPlayout(bool& enabled) const {  
  enabled = kNumChannels == 2;
  return 0;
}

int32_t OpenSlesOutput::PlayoutBuffer(
    AudioDeviceModule::BufferType& type,  
    uint16_t& sizeMS) const {  
  type = AudioDeviceModule::kAdaptiveBufferSize;
  sizeMS = playout_delay_;
  return 0;
}

int32_t OpenSlesOutput::PlayoutDelay(uint16_t& delayMS) const {  
  delayMS = playout_delay_;
  return 0;
}

void OpenSlesOutput::AttachAudioBuffer(AudioDeviceBuffer* audioBuffer) {
  audio_buffer_ = audioBuffer;
}

int32_t OpenSlesOutput::SetLoudspeakerStatus(bool enable) {
  return 0;
}

int32_t OpenSlesOutput::GetLoudspeakerStatus(bool& enabled) const {  
  enabled = true;
  return 0;
}

int OpenSlesOutput::PlayoutDelayMs() {
  return playout_delay_;
}

bool OpenSlesOutput::InitSampleRate() {
  if (!SetLowLatency()) {
    speaker_sampling_rate_ = kDefaultSampleRate;
    
    buffer_size_samples_ = speaker_sampling_rate_ * 10 / 1000;
  }
  if (audio_buffer_->SetPlayoutSampleRate(speaker_sampling_rate_) < 0) {
    return false;
  }
  if (audio_buffer_->SetPlayoutChannels(kNumChannels) < 0) {
    return false;
  }
  UpdatePlayoutDelay();
  return true;
}

void OpenSlesOutput::UpdatePlayoutDelay() {
  
  
  int outstanding_samples = (TotalBuffersUsed() - 0.5) * buffer_size_samples_;
  playout_delay_ = outstanding_samples / (speaker_sampling_rate_ / 1000);
}

bool OpenSlesOutput::SetLowLatency() {
  if (!audio_manager_.low_latency_supported()) {
    return false;
  }
  buffer_size_samples_ = audio_manager_.native_buffer_size();
  assert(buffer_size_samples_ > 0);
  speaker_sampling_rate_ = audio_manager_.native_output_sample_rate();
  assert(speaker_sampling_rate_ > 0);
  return true;
}

void OpenSlesOutput::CalculateNumFifoBuffersNeeded() {
  int number_of_bytes_needed =
      (speaker_sampling_rate_ * kNumChannels * sizeof(int16_t)) * 10 / 1000;

  
  int buffers_per_10_ms =
      1 + ((number_of_bytes_needed - 1) / buffer_size_bytes_);
  
  num_fifo_buffers_needed_ = kNum10MsToBuffer * buffers_per_10_ms;
}

void OpenSlesOutput::AllocateBuffers() {
  
  buffer_size_bytes_ = buffer_size_samples_ * kNumChannels * sizeof(int16_t);
  fine_buffer_.reset(new FineAudioBuffer(audio_buffer_, buffer_size_bytes_,
                                         speaker_sampling_rate_));

  
  
  CalculateNumFifoBuffersNeeded();  
  assert(num_fifo_buffers_needed_ > 0);
  fifo_.reset(new SingleRwFifo(num_fifo_buffers_needed_));

  
  play_buf_.reset(new scoped_array<int8_t>[TotalBuffersUsed()]);
  int required_buffer_size = fine_buffer_->RequiredBufferSizeBytes();
  for (int i = 0; i < TotalBuffersUsed(); ++i) {
    play_buf_[i].reset(new int8_t[required_buffer_size]);
  }
}

int OpenSlesOutput::TotalBuffersUsed() const {
  return num_fifo_buffers_needed_ + kNumOpenSlBuffers;
}

bool OpenSlesOutput::EnqueueAllBuffers() {
  active_queue_ = 0;
  number_underruns_ = 0;
  for (int i = 0; i < kNumOpenSlBuffers; ++i) {
    memset(play_buf_[i].get(), 0, buffer_size_bytes_);
    OPENSL_RETURN_ON_FAILURE(
        (*sles_player_sbq_itf_)->Enqueue(
            sles_player_sbq_itf_,
            reinterpret_cast<void*>(play_buf_[i].get()),
            buffer_size_bytes_),
        false);
  }
  
  
  while (fifo_->size() != 0) {
    
    fifo_->Pop();
  }
  for (int i = kNumOpenSlBuffers; i < TotalBuffersUsed(); ++i) {
    memset(play_buf_[i].get(), 0, buffer_size_bytes_);
    fifo_->Push(play_buf_[i].get());
  }
  return true;
}

bool OpenSlesOutput::CreateAudioPlayer() {
  if (!event_.Start()) {
    assert(false);
    return false;
  }
  SLDataLocator_AndroidSimpleBufferQueue simple_buf_queue = {
    SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
    static_cast<SLuint32>(kNumOpenSlBuffers)
  };
  SLDataFormat_PCM configuration =
      webrtc_opensl::CreatePcmConfiguration(speaker_sampling_rate_);
  SLDataSource audio_source = { &simple_buf_queue, &configuration };

  SLDataLocator_OutputMix locator_outputmix;
  
  locator_outputmix.locatorType = SL_DATALOCATOR_OUTPUTMIX;
  locator_outputmix.outputMix = sles_output_mixer_;
  SLDataSink audio_sink = { &locator_outputmix, NULL };

  
  
  
  SLInterfaceID ids[kNumInterfaces] = {
    SL_IID_BUFFERQUEUE, SL_IID_VOLUME, SL_IID_ANDROIDCONFIGURATION };
  SLboolean req[kNumInterfaces] = {
    SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE };
  OPENSL_RETURN_ON_FAILURE(
      (*sles_engine_itf_)->CreateAudioPlayer(sles_engine_itf_, &sles_player_,
                                             &audio_source, &audio_sink,
                                             kNumInterfaces, ids, req),
      false);
  
  OPENSL_RETURN_ON_FAILURE((*sles_player_)->Realize(sles_player_,
                                                    SL_BOOLEAN_FALSE),
                           false);
  OPENSL_RETURN_ON_FAILURE(
      (*sles_player_)->GetInterface(sles_player_, SL_IID_PLAY,
                                    &sles_player_itf_),
      false);
  OPENSL_RETURN_ON_FAILURE(
      (*sles_player_)->GetInterface(sles_player_, SL_IID_BUFFERQUEUE,
                                    &sles_player_sbq_itf_),
      false);
  return true;
}

void OpenSlesOutput::DestroyAudioPlayer() {
  SLAndroidSimpleBufferQueueItf sles_player_sbq_itf = sles_player_sbq_itf_;
  {
    CriticalSectionScoped lock(crit_sect_.get());
    sles_player_sbq_itf_ = NULL;
    sles_player_itf_ = NULL;
  }
  event_.Stop();
  if (sles_player_sbq_itf) {
    
    OPENSL_RETURN_ON_FAILURE(
        (*sles_player_sbq_itf)->Clear(sles_player_sbq_itf),
        VOID_RETURN);
  }

  if (sles_player_) {
    (*sles_player_)->Destroy(sles_player_);
    sles_player_ = NULL;
  }
}

bool OpenSlesOutput::HandleUnderrun(int event_id, int event_msg) {
  if (!playing_) {
    return false;
  }
  if (event_id == kNoUnderrun) {
    return false;
  }
  WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, id_, "Audio underrun");
  assert(event_id == kUnderrun);
  assert(event_msg > 0);
  
  if (event_msg != kNumOpenSlBuffers) {
    return true;
  }
  
  
  
  OPENSL_RETURN_ON_FAILURE(
      (*sles_player_itf_)->SetPlayState(sles_player_itf_,
                                        SL_PLAYSTATE_STOPPED),
      true);
  EnqueueAllBuffers();
  OPENSL_RETURN_ON_FAILURE(
      (*sles_player_itf_)->SetPlayState(sles_player_itf_,
                                        SL_PLAYSTATE_PLAYING),
      true);
  return true;
}

void OpenSlesOutput::PlayerSimpleBufferQueueCallback(
    SLAndroidSimpleBufferQueueItf sles_player_sbq_itf,
    void* p_context) {
  OpenSlesOutput* audio_device = reinterpret_cast<OpenSlesOutput*>(p_context);
  audio_device->PlayerSimpleBufferQueueCallbackHandler(sles_player_sbq_itf);
}

void OpenSlesOutput::PlayerSimpleBufferQueueCallbackHandler(
    SLAndroidSimpleBufferQueueItf sles_player_sbq_itf) {
  if (fifo_->size() <= 0 || number_underruns_ > 0) {
    ++number_underruns_;
    event_.SignalEvent(kUnderrun, number_underruns_);
    return;
  }
  int8_t* audio = fifo_->Pop();
  if (audio)
  OPENSL_RETURN_ON_FAILURE(
      (*sles_player_sbq_itf)->Enqueue(sles_player_sbq_itf,
                                      audio,
                                      buffer_size_bytes_),
      VOID_RETURN);
  event_.SignalEvent(kNoUnderrun, 0);
}

bool OpenSlesOutput::StartCbThreads() {
  play_thread_.reset(ThreadWrapper::CreateThread(CbThread,
                                                 this,
                                                 kRealtimePriority,
                                                 "opensl_play_thread"));
  assert(play_thread_.get());
  OPENSL_RETURN_ON_FAILURE(
      (*sles_player_itf_)->SetPlayState(sles_player_itf_,
                                        SL_PLAYSTATE_PLAYING),
      false);

  unsigned int thread_id = 0;
  if (!play_thread_->Start(thread_id)) {
    assert(false);
    return false;
  }
  return true;
}

void OpenSlesOutput::StopCbThreads() {
  {
    CriticalSectionScoped lock(crit_sect_.get());
    playing_ = false;
  }
  if (sles_player_itf_) {
    OPENSL_RETURN_ON_FAILURE(
        (*sles_player_itf_)->SetPlayState(sles_player_itf_,
                                          SL_PLAYSTATE_STOPPED),
        VOID_RETURN);
  }
  if (play_thread_.get() == NULL) {
    return;
  }
  event_.Stop();
  if (play_thread_->Stop()) {
    play_thread_.reset();
  } else {
    assert(false);
  }
}

bool OpenSlesOutput::CbThread(void* context) {
  return reinterpret_cast<OpenSlesOutput*>(context)->CbThreadImpl();
}

bool OpenSlesOutput::CbThreadImpl() {
  assert(fine_buffer_.get() != NULL);
  int event_id;
  int event_msg;
  
  event_.WaitOnEvent(&event_id, &event_msg);

  CriticalSectionScoped lock(crit_sect_.get());
  if (HandleUnderrun(event_id, event_msg)) {
    return playing_;
  }
  
  while (fifo_->size() < num_fifo_buffers_needed_ && playing_) {
    int8_t* audio = play_buf_[active_queue_].get();
    fine_buffer_->GetBufferData(audio);
    fifo_->Push(audio);
    active_queue_ = (active_queue_ + 1) % TotalBuffersUsed();
  }
  return playing_;
}

}  

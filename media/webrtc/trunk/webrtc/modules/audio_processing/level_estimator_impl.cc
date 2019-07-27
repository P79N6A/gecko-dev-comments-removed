









#include "webrtc/modules/audio_processing/level_estimator_impl.h"

#include "webrtc/modules/audio_processing/audio_buffer.h"
#include "webrtc/modules/audio_processing/include/audio_processing.h"
#include "webrtc/modules/audio_processing/rms_level.h"
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"

namespace webrtc {

LevelEstimatorImpl::LevelEstimatorImpl(const AudioProcessing* apm,
                                       CriticalSectionWrapper* crit)
    : ProcessingComponent(),
      crit_(crit) {}

LevelEstimatorImpl::~LevelEstimatorImpl() {}

int LevelEstimatorImpl::ProcessStream(AudioBuffer* audio) {
  if (!is_component_enabled()) {
    return AudioProcessing::kNoError;
  }

  RMSLevel* rms_level = static_cast<RMSLevel*>(handle(0));
  for (int i = 0; i < audio->num_channels(); ++i) {
    rms_level->Process(audio->data(i), audio->samples_per_channel());
  }

  return AudioProcessing::kNoError;
}

int LevelEstimatorImpl::Enable(bool enable) {
  CriticalSectionScoped crit_scoped(crit_);
  return EnableComponent(enable);
}

bool LevelEstimatorImpl::is_enabled() const {
  return is_component_enabled();
}

int LevelEstimatorImpl::RMS() {
  if (!is_component_enabled()) {
    return AudioProcessing::kNotEnabledError;
  }

  RMSLevel* rms_level = static_cast<RMSLevel*>(handle(0));
  return rms_level->RMS();
}



void* LevelEstimatorImpl::CreateHandle() const {
  return new RMSLevel;
}

void LevelEstimatorImpl::DestroyHandle(void* handle) const {
  delete static_cast<RMSLevel*>(handle);
}

int LevelEstimatorImpl::InitializeHandle(void* handle) const {
  static_cast<RMSLevel*>(handle)->Reset();
  return AudioProcessing::kNoError;
}

int LevelEstimatorImpl::ConfigureHandle(void* ) const {
  return AudioProcessing::kNoError;
}

int LevelEstimatorImpl::num_handles_required() const {
  return 1;
}

int LevelEstimatorImpl::GetHandleError(void* ) const {
  return AudioProcessing::kUnspecifiedError;
}

}  

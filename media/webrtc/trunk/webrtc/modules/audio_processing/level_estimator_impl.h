









#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_LEVEL_ESTIMATOR_IMPL_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_LEVEL_ESTIMATOR_IMPL_H_

#include "webrtc/modules/audio_processing/include/audio_processing.h"
#include "webrtc/modules/audio_processing/processing_component.h"
#include "webrtc/modules/audio_processing/rms_level.h"

namespace webrtc {

class AudioBuffer;
class CriticalSectionWrapper;

class LevelEstimatorImpl : public LevelEstimator,
                           public ProcessingComponent {
 public:
  LevelEstimatorImpl(const AudioProcessing* apm,
                     CriticalSectionWrapper* crit);
  virtual ~LevelEstimatorImpl();

  int ProcessStream(AudioBuffer* audio);

  
  virtual bool is_enabled() const OVERRIDE;

 private:
  
  virtual int Enable(bool enable) OVERRIDE;
  virtual int RMS() OVERRIDE;

  
  virtual void* CreateHandle() const OVERRIDE;
  virtual int InitializeHandle(void* handle) const OVERRIDE;
  virtual int ConfigureHandle(void* handle) const OVERRIDE;
  virtual void DestroyHandle(void* handle) const OVERRIDE;
  virtual int num_handles_required() const OVERRIDE;
  virtual int GetHandleError(void* handle) const OVERRIDE;

  CriticalSectionWrapper* crit_;
};

}  

#endif  











#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_LEVEL_ESTIMATOR_IMPL_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_LEVEL_ESTIMATOR_IMPL_H_

#include "webrtc/modules/audio_processing/include/audio_processing.h"
#include "webrtc/modules/audio_processing/processing_component.h"

namespace webrtc {
class AudioProcessingImpl;
class AudioBuffer;

class LevelEstimatorImpl : public LevelEstimator,
                           public ProcessingComponent {
 public:
  explicit LevelEstimatorImpl(const AudioProcessingImpl* apm);
  virtual ~LevelEstimatorImpl();

  int ProcessStream(AudioBuffer* audio);

  
  virtual bool is_enabled() const OVERRIDE;

 private:
  
  virtual int Enable(bool enable) OVERRIDE;
  virtual int RMS() OVERRIDE;

  
  virtual void* CreateHandle() const OVERRIDE;
  virtual int InitializeHandle(void* handle) const OVERRIDE;
  virtual int ConfigureHandle(void* handle) const OVERRIDE;
  virtual int DestroyHandle(void* handle) const OVERRIDE;
  virtual int num_handles_required() const OVERRIDE;
  virtual int GetHandleError(void* handle) const OVERRIDE;

  const AudioProcessingImpl* apm_;
};
}  

#endif  

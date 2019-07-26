









#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_LEVEL_ESTIMATOR_IMPL_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_LEVEL_ESTIMATOR_IMPL_H_

#include "audio_processing.h"
#include "processing_component.h"

namespace webrtc {
class AudioProcessingImpl;
class AudioBuffer;

class LevelEstimatorImpl : public LevelEstimator,
                           public ProcessingComponent {
 public:
  explicit LevelEstimatorImpl(const AudioProcessingImpl* apm);
  virtual ~LevelEstimatorImpl();

  int ProcessStream(AudioBuffer* audio);

  
  virtual bool is_enabled() const;

 private:
  
  virtual int Enable(bool enable);
  virtual int RMS();

  
  virtual void* CreateHandle() const;
  virtual int InitializeHandle(void* handle) const;
  virtual int ConfigureHandle(void* handle) const;
  virtual int DestroyHandle(void* handle) const;
  virtual int num_handles_required() const;
  virtual int GetHandleError(void* handle) const;

  const AudioProcessingImpl* apm_;
};
}  

#endif  

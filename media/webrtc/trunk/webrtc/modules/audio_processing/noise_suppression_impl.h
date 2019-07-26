









#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_NOISE_SUPPRESSION_IMPL_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_NOISE_SUPPRESSION_IMPL_H_

#include "webrtc/modules/audio_processing/include/audio_processing.h"
#include "webrtc/modules/audio_processing/processing_component.h"

namespace webrtc {
class AudioProcessingImpl;
class AudioBuffer;

class NoiseSuppressionImpl : public NoiseSuppression,
                             public ProcessingComponent {
 public:
  explicit NoiseSuppressionImpl(const AudioProcessingImpl* apm);
  virtual ~NoiseSuppressionImpl();

  int ProcessCaptureAudio(AudioBuffer* audio);

  
  virtual bool is_enabled() const OVERRIDE;
  virtual float speech_probability() const OVERRIDE;

 private:
  
  virtual int Enable(bool enable) OVERRIDE;
  virtual int set_level(Level level) OVERRIDE;
  virtual Level level() const OVERRIDE;

  
  virtual void* CreateHandle() const OVERRIDE;
  virtual int InitializeHandle(void* handle) const OVERRIDE;
  virtual int ConfigureHandle(void* handle) const OVERRIDE;
  virtual int DestroyHandle(void* handle) const OVERRIDE;
  virtual int num_handles_required() const OVERRIDE;
  virtual int GetHandleError(void* handle) const OVERRIDE;

  const AudioProcessingImpl* apm_;
  Level level_;
};
}  

#endif  

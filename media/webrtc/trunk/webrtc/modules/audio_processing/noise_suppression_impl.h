









#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_NOISE_SUPPRESSION_IMPL_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_NOISE_SUPPRESSION_IMPL_H_

#include "audio_processing.h"
#include "processing_component.h"

namespace webrtc {
class AudioProcessingImpl;
class AudioBuffer;

class NoiseSuppressionImpl : public NoiseSuppression,
                             public ProcessingComponent {
 public:
  explicit NoiseSuppressionImpl(const AudioProcessingImpl* apm);
  virtual ~NoiseSuppressionImpl();

  int ProcessCaptureAudio(AudioBuffer* audio);

  
  virtual bool is_enabled() const;
  float speech_probability() const;

 private:
  
  virtual int Enable(bool enable);
  virtual int set_level(Level level);
  virtual Level level() const;

  
  virtual void* CreateHandle() const;
  virtual int InitializeHandle(void* handle) const;
  virtual int ConfigureHandle(void* handle) const;
  virtual int DestroyHandle(void* handle) const;
  virtual int num_handles_required() const;
  virtual int GetHandleError(void* handle) const;

  const AudioProcessingImpl* apm_;
  Level level_;
};
}  

#endif  

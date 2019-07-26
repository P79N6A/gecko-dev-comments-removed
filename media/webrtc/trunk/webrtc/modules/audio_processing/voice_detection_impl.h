









#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_VOICE_DETECTION_IMPL_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_VOICE_DETECTION_IMPL_H_

#include "webrtc/modules/audio_processing/include/audio_processing.h"
#include "webrtc/modules/audio_processing/processing_component.h"

namespace webrtc {
class AudioProcessingImpl;
class AudioBuffer;

class VoiceDetectionImpl : public VoiceDetection,
                           public ProcessingComponent {
 public:
  explicit VoiceDetectionImpl(const AudioProcessingImpl* apm);
  virtual ~VoiceDetectionImpl();

  int ProcessCaptureAudio(AudioBuffer* audio);

  
  virtual bool is_enabled() const OVERRIDE;

  
  virtual int Initialize() OVERRIDE;

 private:
  
  virtual int Enable(bool enable) OVERRIDE;
  virtual int set_stream_has_voice(bool has_voice) OVERRIDE;
  virtual bool stream_has_voice() const OVERRIDE;
  virtual int set_likelihood(Likelihood likelihood) OVERRIDE;
  virtual Likelihood likelihood() const OVERRIDE;
  virtual int set_frame_size_ms(int size) OVERRIDE;
  virtual int frame_size_ms() const OVERRIDE;

  
  virtual void* CreateHandle() const OVERRIDE;
  virtual int InitializeHandle(void* handle) const OVERRIDE;
  virtual int ConfigureHandle(void* handle) const OVERRIDE;
  virtual int DestroyHandle(void* handle) const OVERRIDE;
  virtual int num_handles_required() const OVERRIDE;
  virtual int GetHandleError(void* handle) const OVERRIDE;

  const AudioProcessingImpl* apm_;
  bool stream_has_voice_;
  bool using_external_vad_;
  Likelihood likelihood_;
  int frame_size_ms_;
  int frame_size_samples_;
};
}  

#endif  











#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_VOICE_DETECTION_IMPL_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_VOICE_DETECTION_IMPL_H_

#include "audio_processing.h"
#include "processing_component.h"

namespace webrtc {
class AudioProcessingImpl;
class AudioBuffer;

class VoiceDetectionImpl : public VoiceDetection,
                           public ProcessingComponent {
 public:
  explicit VoiceDetectionImpl(const AudioProcessingImpl* apm);
  virtual ~VoiceDetectionImpl();

  int ProcessCaptureAudio(AudioBuffer* audio);

  
  virtual bool is_enabled() const;

  
  virtual int Initialize();

 private:
  
  virtual int Enable(bool enable);
  virtual int set_stream_has_voice(bool has_voice);
  virtual bool stream_has_voice() const;
  virtual int set_likelihood(Likelihood likelihood);
  virtual Likelihood likelihood() const;
  virtual int set_frame_size_ms(int size);
  virtual int frame_size_ms() const;

  
  virtual void* CreateHandle() const;
  virtual int InitializeHandle(void* handle) const;
  virtual int ConfigureHandle(void* handle) const;
  virtual int DestroyHandle(void* handle) const;
  virtual int num_handles_required() const;
  virtual int GetHandleError(void* handle) const;

  const AudioProcessingImpl* apm_;
  bool stream_has_voice_;
  bool using_external_vad_;
  Likelihood likelihood_;
  int frame_size_ms_;
  int frame_size_samples_;
};
}  

#endif  

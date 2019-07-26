









#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_ECHO_CONTROL_MOBILE_IMPL_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_ECHO_CONTROL_MOBILE_IMPL_H_

#include "audio_processing.h"
#include "processing_component.h"

namespace webrtc {
class AudioProcessingImpl;
class AudioBuffer;

class EchoControlMobileImpl : public EchoControlMobile,
                              public ProcessingComponent {
 public:
  explicit EchoControlMobileImpl(const AudioProcessingImpl* apm);
  virtual ~EchoControlMobileImpl();

  int ProcessRenderAudio(const AudioBuffer* audio);
  int ProcessCaptureAudio(AudioBuffer* audio);

  
  virtual bool is_enabled() const;

  
  virtual int Initialize();

 private:
  
  virtual int Enable(bool enable);
  virtual int set_routing_mode(RoutingMode mode);
  virtual RoutingMode routing_mode() const;
  virtual int enable_comfort_noise(bool enable);
  virtual bool is_comfort_noise_enabled() const;
  virtual int SetEchoPath(const void* echo_path, size_t size_bytes);
  virtual int GetEchoPath(void* echo_path, size_t size_bytes) const;

  
  virtual void* CreateHandle() const;
  virtual int InitializeHandle(void* handle) const;
  virtual int ConfigureHandle(void* handle) const;
  virtual int DestroyHandle(void* handle) const;
  virtual int num_handles_required() const;
  virtual int GetHandleError(void* handle) const;

  const AudioProcessingImpl* apm_;
  RoutingMode routing_mode_;
  bool comfort_noise_enabled_;
  unsigned char* external_echo_path_;
};
}  

#endif  

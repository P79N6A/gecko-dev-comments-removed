









#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_ECHO_CONTROL_MOBILE_IMPL_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_ECHO_CONTROL_MOBILE_IMPL_H_

#include "webrtc/modules/audio_processing/include/audio_processing.h"
#include "webrtc/modules/audio_processing/processing_component.h"

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

  
  virtual bool is_enabled() const OVERRIDE;

  
  virtual int Initialize() OVERRIDE;

 private:
  
  virtual int Enable(bool enable) OVERRIDE;
  virtual int set_routing_mode(RoutingMode mode) OVERRIDE;
  virtual RoutingMode routing_mode() const OVERRIDE;
  virtual int enable_comfort_noise(bool enable) OVERRIDE;
  virtual bool is_comfort_noise_enabled() const OVERRIDE;
  virtual int SetEchoPath(const void* echo_path, size_t size_bytes) OVERRIDE;
  virtual int GetEchoPath(void* echo_path, size_t size_bytes) const OVERRIDE;

  
  virtual void* CreateHandle() const OVERRIDE;
  virtual int InitializeHandle(void* handle) const OVERRIDE;
  virtual int ConfigureHandle(void* handle) const OVERRIDE;
  virtual int DestroyHandle(void* handle) const OVERRIDE;
  virtual int num_handles_required() const OVERRIDE;
  virtual int GetHandleError(void* handle) const OVERRIDE;

  const AudioProcessingImpl* apm_;
  RoutingMode routing_mode_;
  bool comfort_noise_enabled_;
  unsigned char* external_echo_path_;
};
}  

#endif  

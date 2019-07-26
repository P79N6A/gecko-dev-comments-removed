









#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_ECHO_CANCELLATION_IMPL_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_ECHO_CANCELLATION_IMPL_H_

#include "audio_processing.h"
#include "processing_component.h"

namespace webrtc {
class AudioProcessingImpl;
class AudioBuffer;

class EchoCancellationImpl : public EchoCancellation,
                             public ProcessingComponent {
 public:
  explicit EchoCancellationImpl(const AudioProcessingImpl* apm);
  virtual ~EchoCancellationImpl();

  int ProcessRenderAudio(const AudioBuffer* audio);
  int ProcessCaptureAudio(AudioBuffer* audio);

  
  virtual bool is_enabled() const;
  virtual int device_sample_rate_hz() const;
  virtual int stream_drift_samples() const;

  
  virtual int Initialize();

 private:
  
  virtual int Enable(bool enable);
  virtual int enable_drift_compensation(bool enable);
  virtual bool is_drift_compensation_enabled() const;
  virtual int set_device_sample_rate_hz(int rate);
  virtual int set_stream_drift_samples(int drift);
  virtual int set_suppression_level(SuppressionLevel level);
  virtual SuppressionLevel suppression_level() const;
  virtual int enable_metrics(bool enable);
  virtual bool are_metrics_enabled() const;
  virtual bool stream_has_echo() const;
  virtual int GetMetrics(Metrics* metrics);
  virtual int enable_delay_logging(bool enable);
  virtual bool is_delay_logging_enabled() const;
  virtual int GetDelayMetrics(int* median, int* std);

  
  virtual void* CreateHandle() const;
  virtual int InitializeHandle(void* handle) const;
  virtual int ConfigureHandle(void* handle) const;
  virtual int DestroyHandle(void* handle) const;
  virtual int num_handles_required() const;
  virtual int GetHandleError(void* handle) const;

  const AudioProcessingImpl* apm_;
  bool drift_compensation_enabled_;
  bool metrics_enabled_;
  SuppressionLevel suppression_level_;
  int device_sample_rate_hz_;
  int stream_drift_samples_;
  bool was_stream_drift_set_;
  bool stream_has_echo_;
  bool delay_logging_enabled_;
};
}  

#endif  

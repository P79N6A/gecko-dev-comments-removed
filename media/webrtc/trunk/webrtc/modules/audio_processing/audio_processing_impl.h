









#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_MAIN_SOURCE_AUDIO_PROCESSING_IMPL_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_MAIN_SOURCE_AUDIO_PROCESSING_IMPL_H_

#include "audio_processing.h"

#include <list>
#include <string>

#include "scoped_ptr.h"

namespace webrtc {
class AudioBuffer;
class CriticalSectionWrapper;
class EchoCancellationImpl;
class EchoControlMobileImpl;
class FileWrapper;
class GainControlImpl;
class HighPassFilterImpl;
class LevelEstimatorImpl;
class NoiseSuppressionImpl;
class ProcessingComponent;
class VoiceDetectionImpl;

#ifdef WEBRTC_AUDIOPROC_DEBUG_DUMP
namespace audioproc {

class Event;

}  
#endif

class AudioProcessingImpl : public AudioProcessing {
 public:
  enum {
    kSampleRate8kHz = 8000,
    kSampleRate16kHz = 16000,
    kSampleRate32kHz = 32000
  };

  explicit AudioProcessingImpl(int id);
  virtual ~AudioProcessingImpl();

  CriticalSectionWrapper* crit() const;

  int split_sample_rate_hz() const;
  bool was_stream_delay_set() const;

  
  virtual int Initialize();
  virtual int InitializeLocked();
  virtual int set_sample_rate_hz(int rate);
  virtual int sample_rate_hz() const;
  virtual int set_num_channels(int input_channels, int output_channels);
  virtual int num_input_channels() const;
  virtual int num_output_channels() const;
  virtual int set_num_reverse_channels(int channels);
  virtual int num_reverse_channels() const;
  virtual int ProcessStream(AudioFrame* frame);
  virtual int AnalyzeReverseStream(AudioFrame* frame);
  virtual int set_stream_delay_ms(int delay);
  virtual int stream_delay_ms() const;
  virtual void set_delay_offset_ms(int offset);
  virtual int delay_offset_ms() const;
  virtual int StartDebugRecording(const char filename[kMaxFilenameSize]);
  virtual int StopDebugRecording();
  virtual EchoCancellation* echo_cancellation() const;
  virtual EchoControlMobile* echo_control_mobile() const;
  virtual GainControl* gain_control() const;
  virtual HighPassFilter* high_pass_filter() const;
  virtual LevelEstimator* level_estimator() const;
  virtual NoiseSuppression* noise_suppression() const;
  virtual VoiceDetection* voice_detection() const;

  
  virtual WebRtc_Word32 ChangeUniqueId(const WebRtc_Word32 id);

 private:
  bool is_data_processed() const;
  bool interleave_needed(bool is_data_processed) const;
  bool synthesis_needed(bool is_data_processed) const;
  bool analysis_needed(bool is_data_processed) const;

  int id_;

  EchoCancellationImpl* echo_cancellation_;
  EchoControlMobileImpl* echo_control_mobile_;
  GainControlImpl* gain_control_;
  HighPassFilterImpl* high_pass_filter_;
  LevelEstimatorImpl* level_estimator_;
  NoiseSuppressionImpl* noise_suppression_;
  VoiceDetectionImpl* voice_detection_;

  std::list<ProcessingComponent*> component_list_;
  CriticalSectionWrapper* crit_;
  AudioBuffer* render_audio_;
  AudioBuffer* capture_audio_;
#ifdef WEBRTC_AUDIOPROC_DEBUG_DUMP
  
  
  int WriteMessageToDebugFile();
  int WriteInitMessage();
  scoped_ptr<FileWrapper> debug_file_;
  scoped_ptr<audioproc::Event> event_msg_; 
  std::string event_str_; 
#endif

  int sample_rate_hz_;
  int split_sample_rate_hz_;
  int samples_per_channel_;
  int stream_delay_ms_;
  int delay_offset_ms_;
  bool was_stream_delay_set_;

  int num_reverse_channels_;
  int num_input_channels_;
  int num_output_channels_;
};
}  

#endif

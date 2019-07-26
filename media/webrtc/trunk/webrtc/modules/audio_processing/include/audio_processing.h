









#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_INCLUDE_AUDIO_PROCESSING_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_INCLUDE_AUDIO_PROCESSING_H_

#include <stddef.h>  

#include "webrtc/modules/interface/module.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class AudioFrame;
class EchoCancellation;
class EchoControlMobile;
class GainControl;
class HighPassFilter;
class LevelEstimator;
class NoiseSuppression;
class VoiceDetection;















































































class AudioProcessing : public Module {
 public:
  
  
  
  
  
  static AudioProcessing* Create(int id);
  virtual ~AudioProcessing() {}

  
  
  
  static void Destroy(AudioProcessing* apm);

  
  
  
  
  
  
  
  
  virtual int Initialize() = 0;

  
  
  virtual int set_sample_rate_hz(int rate) = 0;
  virtual int sample_rate_hz() const = 0;

  
  
  
  virtual int set_num_channels(int input_channels, int output_channels) = 0;
  virtual int num_input_channels() const = 0;
  virtual int num_output_channels() const = 0;

  
  
  virtual int set_num_reverse_channels(int channels) = 0;
  virtual int num_reverse_channels() const = 0;

  
  
  
  
  
  
  
  
  
  
  virtual int ProcessStream(AudioFrame* frame) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int AnalyzeReverseStream(AudioFrame* frame) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int set_stream_delay_ms(int delay) = 0;
  virtual int stream_delay_ms() const = 0;

  
  
  
  
  
  virtual void set_delay_offset_ms(int offset) = 0;
  virtual int delay_offset_ms() const = 0;

  
  
  
  
  static const size_t kMaxFilenameSize = 1024;
  virtual int StartDebugRecording(const char filename[kMaxFilenameSize]) = 0;

  
  
  virtual int StopDebugRecording() = 0;

  
  
  
  virtual EchoCancellation* echo_cancellation() const = 0;
  virtual EchoControlMobile* echo_control_mobile() const = 0;
  virtual GainControl* gain_control() const = 0;
  virtual HighPassFilter* high_pass_filter() const = 0;
  virtual LevelEstimator* level_estimator() const = 0;
  virtual NoiseSuppression* noise_suppression() const = 0;
  virtual VoiceDetection* voice_detection() const = 0;

  struct Statistic {
    int instant;  
    int average;  
    int maximum;  
    int minimum;  
  };

  enum Error {
    
    kNoError = 0,
    kUnspecifiedError = -1,
    kCreationFailedError = -2,
    kUnsupportedComponentError = -3,
    kUnsupportedFunctionError = -4,
    kNullPointerError = -5,
    kBadParameterError = -6,
    kBadSampleRateError = -7,
    kBadDataLengthError = -8,
    kBadNumberChannelsError = -9,
    kFileError = -10,
    kStreamParameterNotSetError = -11,
    kNotEnabledError = -12,

    
    
    
    kBadStreamParameterWarning = -13
  };

  
  virtual WebRtc_Word32 TimeUntilNextProcess() { return -1; }
  virtual WebRtc_Word32 Process() { return -1; }
};







class EchoCancellation {
 public:
  
  
  virtual int Enable(bool enable) = 0;
  virtual bool is_enabled() const = 0;

  
  
  
  
  
  
  virtual int enable_drift_compensation(bool enable) = 0;
  virtual bool is_drift_compensation_enabled() const = 0;

  
  
  
  virtual int set_device_sample_rate_hz(int rate) = 0;
  virtual int device_sample_rate_hz() const = 0;

  
  
  
  virtual int set_stream_drift_samples(int drift) = 0;
  virtual int stream_drift_samples() const = 0;

  enum SuppressionLevel {
    kLowSuppression,
    kModerateSuppression,
    kHighSuppression
  };

  
  
  virtual int set_suppression_level(SuppressionLevel level) = 0;
  virtual SuppressionLevel suppression_level() const = 0;

  
  
  virtual bool stream_has_echo() const = 0;

  
  
  virtual int enable_metrics(bool enable) = 0;
  virtual bool are_metrics_enabled() const = 0;

  
  
  
  
  
  
  struct Metrics {
    
    AudioProcessing::Statistic residual_echo_return_loss;

    
    AudioProcessing::Statistic echo_return_loss;

    
    AudioProcessing::Statistic echo_return_loss_enhancement;

    
    AudioProcessing::Statistic a_nlp;
  };

  
  virtual int GetMetrics(Metrics* metrics) = 0;

  
  
  virtual int enable_delay_logging(bool enable) = 0;
  virtual bool is_delay_logging_enabled() const = 0;

  
  
  
  virtual int GetDelayMetrics(int* median, int* std) = 0;

 protected:
  virtual ~EchoCancellation() {}
};





class EchoControlMobile {
 public:
  
  
  virtual int Enable(bool enable) = 0;
  virtual bool is_enabled() const = 0;

  
  
  
  enum RoutingMode {
    kQuietEarpieceOrHeadset,
    kEarpiece,
    kLoudEarpiece,
    kSpeakerphone,
    kLoudSpeakerphone
  };

  
  
  virtual int set_routing_mode(RoutingMode mode) = 0;
  virtual RoutingMode routing_mode() const = 0;

  
  
  virtual int enable_comfort_noise(bool enable) = 0;
  virtual bool is_comfort_noise_enabled() const = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int SetEchoPath(const void* echo_path, size_t size_bytes) = 0;
  virtual int GetEchoPath(void* echo_path, size_t size_bytes) const = 0;

  
  
  static size_t echo_path_size_bytes();

 protected:
  virtual ~EchoControlMobile() {}
};






class GainControl {
 public:
  virtual int Enable(bool enable) = 0;
  virtual bool is_enabled() const = 0;

  
  
  
  virtual int set_stream_analog_level(int level) = 0;

  
  
  
  virtual int stream_analog_level() = 0;

  enum Mode {
    
    
    
    
    
    
    
    kAdaptiveAnalog,

    
    
    
    
    kAdaptiveDigital,

    
    
    
    
    
    
    
    
    
    kFixedDigital
  };

  virtual int set_mode(Mode mode) = 0;
  virtual Mode mode() const = 0;

  
  
  
  
  
  
  
  virtual int set_target_level_dbfs(int level) = 0;
  virtual int target_level_dbfs() const = 0;

  
  
  
  virtual int set_compression_gain_db(int gain) = 0;
  virtual int compression_gain_db() const = 0;

  
  
  
  virtual int enable_limiter(bool enable) = 0;
  virtual bool is_limiter_enabled() const = 0;

  
  
  virtual int set_analog_level_limits(int minimum,
                                      int maximum) = 0;
  virtual int analog_level_minimum() const = 0;
  virtual int analog_level_maximum() const = 0;

  
  
  
  
  
  
  virtual bool stream_is_saturated() const = 0;

 protected:
  virtual ~GainControl() {}
};



class HighPassFilter {
 public:
  virtual int Enable(bool enable) = 0;
  virtual bool is_enabled() const = 0;

 protected:
  virtual ~HighPassFilter() {}
};


class LevelEstimator {
 public:
  virtual int Enable(bool enable) = 0;
  virtual bool is_enabled() const = 0;

  
  
  
  
  
  
  
  
  
  
  
  virtual int RMS() = 0;

 protected:
  virtual ~LevelEstimator() {}
};





class NoiseSuppression {
 public:
  virtual int Enable(bool enable) = 0;
  virtual bool is_enabled() const = 0;

  
  
  enum Level {
    kLow,
    kModerate,
    kHigh,
    kVeryHigh
  };

  virtual int set_level(Level level) = 0;
  virtual Level level() const = 0;

  
  
  
  virtual float speech_probability() const = 0;

 protected:
  virtual ~NoiseSuppression() {}
};








class VoiceDetection {
 public:
  virtual int Enable(bool enable) = 0;
  virtual bool is_enabled() const = 0;

  
  
  virtual bool stream_has_voice() const = 0;

  
  
  
  
  
  
  
  virtual int set_stream_has_voice(bool has_voice) = 0;

  
  
  
  enum Likelihood {
    kVeryLowLikelihood,
    kLowLikelihood,
    kModerateLikelihood,
    kHighLikelihood
  };

  virtual int set_likelihood(Likelihood likelihood) = 0;
  virtual Likelihood likelihood() const = 0;

  
  
  
  
  
  virtual int set_frame_size_ms(int size) = 0;
  virtual int frame_size_ms() const = 0;

 protected:
  virtual ~VoiceDetection() {}
};
}  

#endif  

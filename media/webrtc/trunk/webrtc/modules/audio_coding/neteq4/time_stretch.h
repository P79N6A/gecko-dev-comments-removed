









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ4_TIME_STRETCH_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ4_TIME_STRETCH_H_

#include <assert.h>
#include <string.h>  

#include "webrtc/modules/audio_coding/neteq4/audio_multi_vector.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/typedefs.h"

namespace webrtc {


class BackgroundNoise;




class TimeStretch {
 public:
  enum ReturnCodes {
    kSuccess = 0,
    kSuccessLowEnergy = 1,
    kNoStretch = 2,
    kError = -1
  };

  TimeStretch(int sample_rate_hz, size_t num_channels,
              const BackgroundNoise& background_noise)
      : sample_rate_hz_(sample_rate_hz),
        fs_mult_(sample_rate_hz / 8000),
        num_channels_(static_cast<int>(num_channels)),
        master_channel_(0),  
        background_noise_(background_noise),
        max_input_value_(0) {
    assert(sample_rate_hz_ == 8000 ||
           sample_rate_hz_ == 16000 ||
           sample_rate_hz_ == 32000 ||
           sample_rate_hz_ == 48000);
    assert(num_channels_ > 0);
    assert(static_cast<int>(master_channel_) < num_channels_);
    memset(auto_correlation_, 0, sizeof(auto_correlation_));
  }

  virtual ~TimeStretch() {}

  
  
  ReturnCodes Process(const int16_t* input,
                      size_t input_len,
                      AudioMultiVector<int16_t>* output,
                      int16_t* length_change_samples);

 protected:
  
  
  
  virtual void SetParametersForPassiveSpeech(size_t input_length,
                                             int16_t* best_correlation,
                                             int* peak_index) const = 0;

  
  
  
  virtual ReturnCodes CheckCriteriaAndStretch(
      const int16_t* input, size_t input_length, size_t peak_index,
      int16_t best_correlation, bool active_speech,
      AudioMultiVector<int16_t>* output) const = 0;

  static const int kCorrelationLen = 50;
  static const int kLogCorrelationLen = 6;  
  static const int kMinLag = 10;
  static const int kMaxLag = 60;
  static const int kDownsampledLen = kCorrelationLen + kMaxLag;
  static const int kCorrelationThreshold = 14746;  

  const int sample_rate_hz_;
  const int fs_mult_;  
  const int num_channels_;
  const size_t master_channel_;
  const BackgroundNoise& background_noise_;
  int16_t max_input_value_;
  int16_t downsampled_input_[kDownsampledLen];
  
  
  int16_t auto_correlation_[kCorrelationLen + 1];

 private:
  
  
  void AutoCorrelation();

  
  bool SpeechDetection(int32_t vec1_energy, int32_t vec2_energy,
                       int peak_index, int scaling) const;

  DISALLOW_COPY_AND_ASSIGN(TimeStretch);
};

}  
#endif

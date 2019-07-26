









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ4_BACKGROUND_NOISE_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ4_BACKGROUND_NOISE_H_

#include <string.h>  

#include "webrtc/modules/audio_coding/neteq4/audio_multi_vector.h"
#include "webrtc/modules/audio_coding/neteq4/interface/neteq.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/typedefs.h"

namespace webrtc {


class PostDecodeVad;


class BackgroundNoise {
 public:
  
  
  static const int kMaxLpcOrder = 8;  

  explicit BackgroundNoise(size_t num_channels);
  virtual ~BackgroundNoise();

  void Reset();

  
  
  void Update(const AudioMultiVector& sync_buffer,
              const PostDecodeVad& vad);

  
  int32_t Energy(size_t channel) const;

  
  void SetMuteFactor(size_t channel, int16_t value);

  
  int16_t MuteFactor(size_t channel) const;

  
  const int16_t* Filter(size_t channel) const;

  
  const int16_t* FilterState(size_t channel) const;

  
  
  void SetFilterState(size_t channel, const int16_t* input, size_t length);

  
  int16_t Scale(size_t channel) const;

  
  int16_t ScaleShift(size_t channel) const;

  
  bool initialized() const { return initialized_; }
  NetEqBackgroundNoiseMode mode() const { return mode_; }

  
  
  void set_mode(NetEqBackgroundNoiseMode mode) { mode_ = mode; }

 private:
  static const int kThresholdIncrement = 229;  
  static const int kVecLen = 256;
  static const int kLogVecLen = 8;  
  static const int kResidualLength = 64;
  static const int kLogResidualLength = 6;  

  struct ChannelParameters {
    
    ChannelParameters() {
      Reset();
    }

    void Reset() {
      energy = 2500;
      max_energy = 0;
      energy_update_threshold = 500000;
      low_energy_update_threshold = 0;
      memset(filter_state, 0, sizeof(filter_state));
      memset(filter, 0, sizeof(filter));
      filter[0] = 4096;
      mute_factor = 0,
      scale = 20000;
      scale_shift = 24;
    }

    int32_t energy;
    int32_t max_energy;
    int32_t energy_update_threshold;
    int32_t low_energy_update_threshold;
    int16_t filter_state[kMaxLpcOrder];
    int16_t filter[kMaxLpcOrder + 1];
    int16_t mute_factor;
    int16_t scale;
    int16_t scale_shift;
  };

  int32_t CalculateAutoCorrelation(const int16_t* signal,
                                   int length,
                                   int32_t* auto_correlation) const;

  
  void IncrementEnergyThreshold(size_t channel, int32_t sample_energy);

  
  void SaveParameters(size_t channel,
                      const int16_t* lpc_coefficients,
                      const int16_t* filter_state,
                      int32_t sample_energy,
                      int32_t residual_energy);

  size_t num_channels_;
  scoped_array<ChannelParameters> channel_parameters_;
  bool initialized_;
  NetEqBackgroundNoiseMode mode_;

  DISALLOW_COPY_AND_ASSIGN(BackgroundNoise);
};

}  
#endif  

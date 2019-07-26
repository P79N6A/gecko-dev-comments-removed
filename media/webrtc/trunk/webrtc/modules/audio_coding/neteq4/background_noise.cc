









#include "webrtc/modules/audio_coding/neteq4/background_noise.h"

#include <assert.h>
#include <string.h>  

#include <algorithm>  

#include "webrtc/common_audio/signal_processing/include/signal_processing_library.h"
#include "webrtc/modules/audio_coding/neteq4/audio_multi_vector.h"
#include "webrtc/modules/audio_coding/neteq4/post_decode_vad.h"

namespace webrtc {

BackgroundNoise::BackgroundNoise(size_t num_channels)
    : num_channels_(num_channels),
      channel_parameters_(new ChannelParameters[num_channels_]),
      mode_(kBgnOn) {
  Reset();
}

BackgroundNoise::~BackgroundNoise() {}

void BackgroundNoise::Reset() {
  initialized_ = false;
  for (size_t channel = 0; channel < num_channels_; ++channel) {
    channel_parameters_[channel].Reset();
  }
  
}

void BackgroundNoise::Update(const AudioMultiVector& input,
                             const PostDecodeVad& vad) {
  if (vad.running() && vad.active_speech()) {
    
    
    return;
  }

  int32_t auto_correlation[kMaxLpcOrder + 1];
  int16_t fiter_output[kMaxLpcOrder + kResidualLength];
  int16_t reflection_coefficients[kMaxLpcOrder];
  int16_t lpc_coefficients[kMaxLpcOrder + 1];

  for (size_t channel_ix = 0; channel_ix < num_channels_; ++channel_ix) {
    ChannelParameters& parameters = channel_parameters_[channel_ix];
    int16_t temp_signal_array[kVecLen + kMaxLpcOrder] = {0};
    int16_t* temp_signal = &temp_signal_array[kMaxLpcOrder];
    memcpy(temp_signal,
           &input[channel_ix][input.Size() - kVecLen],
           sizeof(int16_t) * kVecLen);

    int32_t sample_energy = CalculateAutoCorrelation(temp_signal, kVecLen,
                                                     auto_correlation);

    if ((!vad.running() &&
        sample_energy < parameters.energy_update_threshold) ||
        (vad.running() && !vad.active_speech())) {
      
      if (auto_correlation[0] > 0) {
        
        
        
        if (sample_energy < parameters.energy_update_threshold) {
          
          parameters.energy_update_threshold = std::max(sample_energy, 1);
          parameters.low_energy_update_threshold = 0;
        }

        
        
        if (WebRtcSpl_LevinsonDurbin(auto_correlation, lpc_coefficients,
                                     reflection_coefficients,
                                     kMaxLpcOrder) != 1) {
          return;
        }
      } else {
        
        return;
      }

      
      WebRtcSpl_FilterMAFastQ12(temp_signal + kVecLen - kResidualLength,
                                fiter_output, lpc_coefficients,
                                kMaxLpcOrder + 1, kResidualLength);
      int32_t residual_energy = WebRtcSpl_DotProductWithScale(fiter_output,
                                                              fiter_output,
                                                              kResidualLength,
                                                              0);

      
      
      
      
      
      if ((residual_energy * 20 >= (sample_energy << 6)) &&
          (sample_energy > 0)) {
        
        
        
        
        SaveParameters(channel_ix, lpc_coefficients,
                       temp_signal + kVecLen - kMaxLpcOrder, sample_energy,
                       residual_energy);
      }
    } else {
      
      
      
      IncrementEnergyThreshold(channel_ix, sample_energy);
    }
  }
  return;
}

int32_t BackgroundNoise::Energy(size_t channel) const {
  assert(channel < num_channels_);
  return channel_parameters_[channel].energy;
}

void BackgroundNoise::SetMuteFactor(size_t channel, int16_t value) {
  assert(channel < num_channels_);
  channel_parameters_[channel].mute_factor = value;
}

int16_t BackgroundNoise::MuteFactor(size_t channel) const {
  assert(channel < num_channels_);
  return channel_parameters_[channel].mute_factor;
}

const int16_t* BackgroundNoise::Filter(size_t channel) const {
  assert(channel < num_channels_);
  return channel_parameters_[channel].filter;
}

const int16_t* BackgroundNoise::FilterState(size_t channel) const {
  assert(channel < num_channels_);
  return channel_parameters_[channel].filter_state;
}

void BackgroundNoise::SetFilterState(size_t channel, const int16_t* input,
                                     size_t length) {
  assert(channel < num_channels_);
  length = std::min(length, static_cast<size_t>(kMaxLpcOrder));
  memcpy(channel_parameters_[channel].filter_state, input,
         length * sizeof(int16_t));
}

int16_t BackgroundNoise::Scale(size_t channel) const {
  assert(channel < num_channels_);
  return channel_parameters_[channel].scale;
}
int16_t BackgroundNoise::ScaleShift(size_t channel) const {
  assert(channel < num_channels_);
  return channel_parameters_[channel].scale_shift;
}

int32_t BackgroundNoise::CalculateAutoCorrelation(
    const int16_t* signal, int length, int32_t* auto_correlation) const {
  int16_t signal_max = WebRtcSpl_MaxAbsValueW16(signal, length);
  int correlation_scale = kLogVecLen -
      WebRtcSpl_NormW32(signal_max * signal_max);
  correlation_scale = std::max(0, correlation_scale);

  static const int kCorrelationStep = -1;
  WebRtcSpl_CrossCorrelation(auto_correlation, signal, signal, length,
                             kMaxLpcOrder + 1, correlation_scale,
                             kCorrelationStep);

  
  int energy_sample_shift = kLogVecLen - correlation_scale;
  return auto_correlation[0] >> energy_sample_shift;
}

void BackgroundNoise::IncrementEnergyThreshold(size_t channel,
                                               int32_t sample_energy) {
  
  
  
  
  
  assert(channel < num_channels_);
  ChannelParameters& parameters = channel_parameters_[channel];
  int32_t temp_energy =
      WEBRTC_SPL_MUL_16_16_RSFT(kThresholdIncrement,
                                parameters.low_energy_update_threshold, 16);
  temp_energy += kThresholdIncrement *
      (parameters.energy_update_threshold & 0xFF);
  temp_energy += (kThresholdIncrement *
      ((parameters.energy_update_threshold>>8) & 0xFF)) << 8;
  parameters.low_energy_update_threshold += temp_energy;

  parameters.energy_update_threshold += kThresholdIncrement *
      (parameters.energy_update_threshold>>16);
  parameters.energy_update_threshold +=
      parameters.low_energy_update_threshold >> 16;
  parameters.low_energy_update_threshold =
      parameters.low_energy_update_threshold & 0x0FFFF;

  
  
  parameters.max_energy = parameters.max_energy -
      (parameters.max_energy >> 10);
  if (sample_energy > parameters.max_energy) {
    parameters.max_energy = sample_energy;
  }

  
  
  int32_t energy_update_threshold = (parameters.max_energy + 524288) >> 20;
  if (energy_update_threshold > parameters.energy_update_threshold) {
    parameters.energy_update_threshold = energy_update_threshold;
  }
}

void BackgroundNoise::SaveParameters(size_t channel,
                                     const int16_t* lpc_coefficients,
                                     const int16_t* filter_state,
                                     int32_t sample_energy,
                                     int32_t residual_energy) {
  assert(channel < num_channels_);
  ChannelParameters& parameters = channel_parameters_[channel];
  memcpy(parameters.filter, lpc_coefficients,
         (kMaxLpcOrder+1) * sizeof(int16_t));
  memcpy(parameters.filter_state, filter_state,
         kMaxLpcOrder * sizeof(int16_t));
  
  
  parameters.energy = std::max(sample_energy, 1);
  parameters.energy_update_threshold = parameters.energy;
  parameters.low_energy_update_threshold = 0;

  
  int norm_shift = WebRtcSpl_NormW32(residual_energy) - 1;
  if (norm_shift & 0x1) {
    norm_shift -= 1;  
  }
  assert(norm_shift >= 0);  
  residual_energy = residual_energy << norm_shift;

  
  parameters.scale = WebRtcSpl_SqrtFloor(residual_energy);
  
  
  
  parameters.scale_shift = 13 + ((kLogResidualLength + norm_shift) / 2);

  initialized_ = true;
}

}  











#include "webrtc/modules/audio_coding/neteq4/expand.h"

#include <assert.h>
#include <string.h>  

#include <algorithm>  
#include <limits>  

#include "webrtc/common_audio/signal_processing/include/signal_processing_library.h"
#include "webrtc/modules/audio_coding/neteq4/background_noise.h"
#include "webrtc/modules/audio_coding/neteq4/dsp_helper.h"
#include "webrtc/modules/audio_coding/neteq4/random_vector.h"
#include "webrtc/modules/audio_coding/neteq4/sync_buffer.h"

namespace webrtc {

void Expand::Reset() {
  first_expand_ = true;
  consecutive_expands_ = 0;
  max_lag_ = 0;
  for (size_t ix = 0; ix < num_channels_; ++ix) {
    channel_parameters_[ix].expand_vector0.Clear();
    channel_parameters_[ix].expand_vector1.Clear();
  }
}

int Expand::Process(AudioMultiVector* output) {
  int16_t random_vector[kMaxSampleRate / 8000 * 120 + 30];
  int16_t scaled_random_vector[kMaxSampleRate / 8000 * 125];
  static const int kTempDataSize = 3600;
  int16_t temp_data[kTempDataSize];  
  int16_t* voiced_vector_storage = temp_data;
  int16_t* voiced_vector = &voiced_vector_storage[overlap_length_];
  static const int kNoiseLpcOrder = BackgroundNoise::kMaxLpcOrder;
  int16_t unvoiced_array_memory[kNoiseLpcOrder + kMaxSampleRate / 8000 * 125];
  int16_t* unvoiced_vector = unvoiced_array_memory + kUnvoicedLpcOrder;
  int16_t* noise_vector = unvoiced_array_memory + kNoiseLpcOrder;

  int fs_mult = fs_hz_ / 8000;

  if (first_expand_) {
    
    AnalyzeSignal(random_vector);
    first_expand_ = false;
  } else {
    
    
    int16_t rand_length = max_lag_;
    
    
    if (rand_length <= RandomVector::kRandomTableSize) {
      random_vector_->IncreaseSeedIncrement(2);
      random_vector_->Generate(rand_length, random_vector);
    } else {
      
      assert(rand_length <= kMaxSampleRate / 8000 * 120 + 30);
      random_vector_->IncreaseSeedIncrement(2);
      random_vector_->Generate(RandomVector::kRandomTableSize, random_vector);
      random_vector_->IncreaseSeedIncrement(2);
      random_vector_->Generate(rand_length - RandomVector::kRandomTableSize,
                               &random_vector[RandomVector::kRandomTableSize]);
    }
  }


  
  UpdateLagIndex();

  
  
  size_t expansion_vector_length = max_lag_ + overlap_length_;
  size_t current_lag = expand_lags_[current_lag_index_];
  
  size_t expansion_vector_position = expansion_vector_length - current_lag -
      overlap_length_;
  size_t temp_length = current_lag + overlap_length_;
  for (size_t channel_ix = 0; channel_ix < num_channels_; ++channel_ix) {
    ChannelParameters& parameters = channel_parameters_[channel_ix];
    if (current_lag_index_ == 0) {
      
      assert(expansion_vector_position + temp_length <=
             parameters.expand_vector0.Size());
      memcpy(voiced_vector_storage,
             &parameters.expand_vector0[expansion_vector_position],
             sizeof(int16_t) * temp_length);
    } else if (current_lag_index_ == 1) {
      
      WebRtcSpl_ScaleAndAddVectorsWithRound(
          &parameters.expand_vector0[expansion_vector_position], 3,
          &parameters.expand_vector1[expansion_vector_position], 1, 2,
          voiced_vector_storage, static_cast<int>(temp_length));
    } else if (current_lag_index_ == 2) {
      
      assert(expansion_vector_position + temp_length <=
             parameters.expand_vector0.Size());
      assert(expansion_vector_position + temp_length <=
             parameters.expand_vector1.Size());
      WebRtcSpl_ScaleAndAddVectorsWithRound(
          &parameters.expand_vector0[expansion_vector_position], 1,
          &parameters.expand_vector1[expansion_vector_position], 1, 1,
          voiced_vector_storage, static_cast<int>(temp_length));
    }

    
    int16_t muting_window, muting_window_increment;
    int16_t unmuting_window, unmuting_window_increment;
    if (fs_hz_ == 8000) {
      muting_window = DspHelper::kMuteFactorStart8kHz;
      muting_window_increment = DspHelper::kMuteFactorIncrement8kHz;
      unmuting_window = DspHelper::kUnmuteFactorStart8kHz;
      unmuting_window_increment = DspHelper::kUnmuteFactorIncrement8kHz;
    } else if (fs_hz_ == 16000) {
      muting_window = DspHelper::kMuteFactorStart16kHz;
      muting_window_increment = DspHelper::kMuteFactorIncrement16kHz;
      unmuting_window = DspHelper::kUnmuteFactorStart16kHz;
      unmuting_window_increment = DspHelper::kUnmuteFactorIncrement16kHz;
    } else if (fs_hz_ == 32000) {
      muting_window = DspHelper::kMuteFactorStart32kHz;
      muting_window_increment = DspHelper::kMuteFactorIncrement32kHz;
      unmuting_window = DspHelper::kUnmuteFactorStart32kHz;
      unmuting_window_increment = DspHelper::kUnmuteFactorIncrement32kHz;
    } else {  
      muting_window = DspHelper::kMuteFactorStart48kHz;
      muting_window_increment = DspHelper::kMuteFactorIncrement48kHz;
      unmuting_window = DspHelper::kUnmuteFactorStart48kHz;
      unmuting_window_increment = DspHelper::kUnmuteFactorIncrement48kHz;
    }

    
    
    if ((parameters.mute_factor > 819) &&
        (parameters.current_voice_mix_factor > 8192)) {
      size_t start_ix = sync_buffer_->Size() - overlap_length_;
      for (size_t i = 0; i < overlap_length_; i++) {
        
        (*sync_buffer_)[channel_ix][start_ix + i] =
            (((*sync_buffer_)[channel_ix][start_ix + i] * muting_window) +
                (((parameters.mute_factor * voiced_vector_storage[i]) >> 14) *
                    unmuting_window) + 16384) >> 15;
        muting_window += muting_window_increment;
        unmuting_window += unmuting_window_increment;
      }
    } else if (parameters.mute_factor == 0) {
      
      
      
      
      
      
      




    }

    
    
    memcpy(unvoiced_vector - kUnvoicedLpcOrder, parameters.ar_filter_state,
           sizeof(int16_t) * kUnvoicedLpcOrder);
    int32_t add_constant = 0;
    if (parameters.ar_gain_scale > 0) {
      add_constant = 1 << (parameters.ar_gain_scale - 1);
    }
    WebRtcSpl_AffineTransformVector(scaled_random_vector, random_vector,
                                    parameters.ar_gain, add_constant,
                                    parameters.ar_gain_scale,
                                    static_cast<int>(current_lag));
    WebRtcSpl_FilterARFastQ12(scaled_random_vector, unvoiced_vector,
                              parameters.ar_filter, kUnvoicedLpcOrder + 1,
                              static_cast<int>(current_lag));
    memcpy(parameters.ar_filter_state,
           &(unvoiced_vector[current_lag - kUnvoicedLpcOrder]),
           sizeof(int16_t) * kUnvoicedLpcOrder);

    

    
    
    
    
    
    
    int temp_shift = (31 - WebRtcSpl_NormW32(max_lag_)) - 5;
    int16_t mix_factor_increment = 256 >> temp_shift;
    if (stop_muting_) {
      mix_factor_increment = 0;
    }

    
    temp_shift = 8 - temp_shift;  
    size_t temp_lenght = (parameters.current_voice_mix_factor -
        parameters.voice_mix_factor) >> temp_shift;
    temp_lenght = std::min(temp_lenght, current_lag);
    DspHelper::CrossFade(voiced_vector, unvoiced_vector, temp_lenght,
                         &parameters.current_voice_mix_factor,
                         mix_factor_increment, temp_data);

    
    
    if (temp_lenght < current_lag) {
      if (mix_factor_increment != 0) {
        parameters.current_voice_mix_factor = parameters.voice_mix_factor;
      }
      int temp_scale = 16384 - parameters.current_voice_mix_factor;
      WebRtcSpl_ScaleAndAddVectorsWithRound(
          voiced_vector + temp_lenght, parameters.current_voice_mix_factor,
          unvoiced_vector + temp_lenght, temp_scale, 14,
          temp_data + temp_lenght, static_cast<int>(current_lag - temp_lenght));
    }

    
    
    if (consecutive_expands_ == 3) {
      
      
      parameters.mute_slope = std::max(parameters.mute_slope,
                                       static_cast<int16_t>(1049 / fs_mult));
    }
    if (consecutive_expands_ == 7) {
      
      
      parameters.mute_slope = std::max(parameters.mute_slope,
                                       static_cast<int16_t>(2097 / fs_mult));
    }

    
    if ((consecutive_expands_ != 0) || !parameters.onset) {
      
      WebRtcSpl_AffineTransformVector(temp_data, temp_data,
                                      parameters.mute_factor, 8192,
                                      14, static_cast<int>(current_lag));

      if (!stop_muting_) {
        DspHelper::MuteSignal(temp_data, parameters.mute_slope, current_lag);

        
        
        
        int16_t gain = static_cast<int16_t>(16384 -
            (((current_lag * parameters.mute_slope) + 8192) >> 6));
        gain = ((gain * parameters.mute_factor) + 8192) >> 14;

        
        
        if ((consecutive_expands_ > 3) && (gain >= parameters.mute_factor)) {
          parameters.mute_factor = 0;
        } else {
          parameters.mute_factor = gain;
        }
      }
    }

    
    
    if (background_noise_->initialized()) {
      
      memcpy(noise_vector - kNoiseLpcOrder,
             background_noise_->FilterState(channel_ix),
             sizeof(int16_t) * kNoiseLpcOrder);

      if (background_noise_->ScaleShift(channel_ix) > 1) {
        add_constant = 1 << (background_noise_->ScaleShift(channel_ix) - 1);
      } else {
        add_constant = 0;
      }

      
      WebRtcSpl_AffineTransformVector(
          scaled_random_vector, random_vector,
          background_noise_->Scale(channel_ix), add_constant,
          background_noise_->ScaleShift(channel_ix),
          static_cast<int>(current_lag));

      WebRtcSpl_FilterARFastQ12(scaled_random_vector, noise_vector,
                                background_noise_->Filter(channel_ix),
                                kNoiseLpcOrder + 1,
                                static_cast<int>(current_lag));

      background_noise_->SetFilterState(
          channel_ix,
          &(noise_vector[current_lag - kNoiseLpcOrder]),
          kNoiseLpcOrder);

      
      int16_t bgn_mute_factor = background_noise_->MuteFactor(channel_ix);
      NetEqBackgroundNoiseMode bgn_mode = background_noise_->mode();
      if (bgn_mode == kBgnFade &&
          consecutive_expands_ >= kMaxConsecutiveExpands &&
          bgn_mute_factor > 0) {
        
        
        int16_t mute_slope;
        if (fs_hz_ == 8000) {
          mute_slope = -32;
        } else if (fs_hz_ == 16000) {
          mute_slope = -16;
        } else if (fs_hz_ == 32000) {
          mute_slope = -8;
        } else {
          mute_slope = -5;
        }
        
        
        DspHelper::UnmuteSignal(noise_vector, current_lag, &bgn_mute_factor,
                                mute_slope, noise_vector);
      } else if (bgn_mute_factor < 16384) {
        
        
        if (!stop_muting_ && bgn_mode != kBgnOff &&
            !(bgn_mode == kBgnFade &&
                consecutive_expands_ >= kMaxConsecutiveExpands)) {
          DspHelper::UnmuteSignal(noise_vector, static_cast<int>(current_lag),
                                  &bgn_mute_factor, parameters.mute_slope,
                                  noise_vector);
        } else {
          
          
          
          WebRtcSpl_AffineTransformVector(noise_vector, noise_vector,
                                          bgn_mute_factor, 8192, 14,
                                          static_cast<int>(current_lag));
        }
      }
      
      background_noise_->SetMuteFactor(channel_ix, bgn_mute_factor);
    } else {
      
      memset(noise_vector, 0, sizeof(int16_t) * current_lag);
    }

    
    for (size_t i = 0; i < current_lag; i++) {
      temp_data[i] = temp_data[i] + noise_vector[i];
    }
    if (channel_ix == 0) {
      output->AssertSize(current_lag);
    } else {
      assert(output->Size() == current_lag);
    }
    memcpy(&(*output)[channel_ix][0], temp_data,
           sizeof(temp_data[0]) * current_lag);
  }

  
  ++consecutive_expands_;
  if (consecutive_expands_ > kMaxConsecutiveExpands) {
    consecutive_expands_ = kMaxConsecutiveExpands;
  }

  return 0;
}

void Expand::SetParametersForNormalAfterExpand() {
  current_lag_index_ = 0;
  lag_index_direction_ = 0;
  stop_muting_ = true;  
}

void Expand::SetParametersForMergeAfterExpand() {
  current_lag_index_ = -1; 
  lag_index_direction_ = 1; 
  stop_muting_ = true;
}

void Expand::AnalyzeSignal(int16_t* random_vector) {
  int32_t auto_correlation[kUnvoicedLpcOrder + 1];
  int16_t reflection_coeff[kUnvoicedLpcOrder];
  int16_t correlation_vector[kMaxSampleRate / 8000 * 102];
  int best_correlation_index[kNumCorrelationCandidates];
  int16_t best_correlation[kNumCorrelationCandidates];
  int16_t best_distortion_index[kNumCorrelationCandidates];
  int16_t best_distortion[kNumCorrelationCandidates];
  int32_t correlation_vector2[(99 * kMaxSampleRate / 8000) + 1];
  int32_t best_distortion_w32[kNumCorrelationCandidates];
  static const int kNoiseLpcOrder = BackgroundNoise::kMaxLpcOrder;
  int16_t unvoiced_array_memory[kNoiseLpcOrder + kMaxSampleRate / 8000 * 125];
  int16_t* unvoiced_vector = unvoiced_array_memory + kUnvoicedLpcOrder;

  int fs_mult = fs_hz_ / 8000;

  
  int fs_mult_4 = fs_mult * 4;
  int fs_mult_20 = fs_mult * 20;
  int fs_mult_120 = fs_mult * 120;
  int fs_mult_dist_len = fs_mult * kDistortionLength;
  int fs_mult_lpc_analysis_len = fs_mult * kLpcAnalysisLength;

  const size_t signal_length = 256 * fs_mult;
  const int16_t* audio_history =
      &(*sync_buffer_)[0][sync_buffer_->Size() - signal_length];

  
  lag_index_direction_ = 1;
  current_lag_index_ = -1;
  stop_muting_ = false;
  random_vector_->set_seed_increment(1);
  consecutive_expands_ = 0;
  for (size_t ix = 0; ix < num_channels_; ++ix) {
    channel_parameters_[ix].current_voice_mix_factor = 16384;  
    channel_parameters_[ix].mute_factor = 16384;  
    
    background_noise_->SetMuteFactor(ix, 0);
  }

  
  int16_t correlation_scale;
  int correlation_length = 51;  
  
  
  Correlation(audio_history, signal_length, correlation_vector,
              &correlation_scale);

  
  DspHelper::PeakDetection(correlation_vector, correlation_length,
                           kNumCorrelationCandidates, fs_mult,
                           best_correlation_index, best_correlation);

  
  
  best_correlation_index[0] += fs_mult_20;
  best_correlation_index[1] += fs_mult_20;
  best_correlation_index[2] += fs_mult_20;

  
  int distortion_scale = 0;
  for (int i = 0; i < kNumCorrelationCandidates; i++) {
    int16_t min_index = std::max(fs_mult_20,
                                 best_correlation_index[i] - fs_mult_4);
    int16_t max_index = std::min(fs_mult_120 - 1,
                                 best_correlation_index[i] + fs_mult_4);
    best_distortion_index[i] = DspHelper::MinDistortion(
        &(audio_history[signal_length - fs_mult_dist_len]), min_index,
        max_index, fs_mult_dist_len, &best_distortion_w32[i]);
    distortion_scale = std::max(16 - WebRtcSpl_NormW32(best_distortion_w32[i]),
                                distortion_scale);
  }
  
  WebRtcSpl_VectorBitShiftW32ToW16(best_distortion, kNumCorrelationCandidates,
                                   best_distortion_w32, distortion_scale);

  
  
  int32_t best_ratio = std::numeric_limits<int32_t>::min();
  int best_index = -1;
  for (int i = 0; i < kNumCorrelationCandidates; ++i) {
    int32_t ratio;
    if (best_distortion[i] > 0) {
      ratio = (best_correlation[i] << 16) / best_distortion[i];
    } else if (best_correlation[i] == 0) {
      ratio = 0;  
    } else {
      ratio = std::numeric_limits<int32_t>::max();  
    }
    if (ratio > best_ratio) {
      best_index = i;
      best_ratio = ratio;
    }
  }

  int distortion_lag = best_distortion_index[best_index];
  int correlation_lag = best_correlation_index[best_index];
  max_lag_ = std::max(distortion_lag, correlation_lag);

  
  
  correlation_length = distortion_lag + 10;
  correlation_length = std::min(correlation_length, fs_mult_120);
  correlation_length = std::max(correlation_length, 60 * fs_mult);

  int start_index = std::min(distortion_lag, correlation_lag);
  int correlation_lags = WEBRTC_SPL_ABS_W16((distortion_lag-correlation_lag))
      + 1;
  assert(correlation_lags <= 99 * fs_mult + 1);  

  for (size_t channel_ix = 0; channel_ix < num_channels_; ++channel_ix) {
    ChannelParameters& parameters = channel_parameters_[channel_ix];
    
    int16_t signal_max = WebRtcSpl_MaxAbsValueW16(
        &audio_history[signal_length - correlation_length - start_index
                       - correlation_lags],
                       correlation_length + start_index + correlation_lags - 1);
    correlation_scale = ((31 - WebRtcSpl_NormW32(signal_max * signal_max))
        + (31 - WebRtcSpl_NormW32(correlation_length))) - 31;
    correlation_scale = std::max(static_cast<int16_t>(0), correlation_scale);

    
    WebRtcSpl_CrossCorrelation(
        correlation_vector2,
        &(audio_history[signal_length - correlation_length]),
        &(audio_history[signal_length - correlation_length - start_index]),
        correlation_length, correlation_lags, correlation_scale, -1);

    
    best_index = WebRtcSpl_MaxIndexW32(correlation_vector2, correlation_lags);
    int32_t max_correlation = correlation_vector2[best_index];
    
    best_index = best_index + start_index;

    
    int32_t energy1 = WebRtcSpl_DotProductWithScale(
        &(audio_history[signal_length - correlation_length]),
        &(audio_history[signal_length - correlation_length]),
        correlation_length, correlation_scale);
    int32_t energy2 = WebRtcSpl_DotProductWithScale(
        &(audio_history[signal_length - correlation_length - best_index]),
        &(audio_history[signal_length - correlation_length - best_index]),
        correlation_length, correlation_scale);

    
    
    int16_t corr_coefficient;
    if ((energy1 > 0) && (energy2 > 0)) {
      int energy1_scale = std::max(16 - WebRtcSpl_NormW32(energy1), 0);
      int energy2_scale = std::max(16 - WebRtcSpl_NormW32(energy2), 0);
      
      if ((energy1_scale + energy2_scale) & 1) {
        
        energy1_scale += 1;
      }
      int16_t scaled_energy1 = energy1 >> energy1_scale;
      int16_t scaled_energy2 = energy2 >> energy2_scale;
      int16_t sqrt_energy_product = WebRtcSpl_SqrtFloor(
          scaled_energy1 * scaled_energy2);
      
      int cc_shift = 14 - (energy1_scale + energy2_scale) / 2;
      max_correlation = WEBRTC_SPL_SHIFT_W32(max_correlation, cc_shift);
      corr_coefficient = WebRtcSpl_DivW32W16(max_correlation,
                                             sqrt_energy_product);
      corr_coefficient = std::min(static_cast<int16_t>(16384),
                                  corr_coefficient);  
    } else {
      corr_coefficient = 0;
    }

    
    
    int16_t expansion_length = static_cast<int16_t>(max_lag_ + overlap_length_);
    const int16_t* vector1 = &(audio_history[signal_length - expansion_length]);
    const int16_t* vector2 = vector1 - distortion_lag;
    
    energy1 = WebRtcSpl_DotProductWithScale(vector1, vector1, expansion_length,
                                            correlation_scale);
    energy2 = WebRtcSpl_DotProductWithScale(vector2, vector2, expansion_length,
                                            correlation_scale);
    
    
    int16_t amplitude_ratio;
    if ((energy1 / 4 < energy2) && (energy1 > energy2 / 4)) {
      
      
      int16_t scaled_energy2 = std::max(16 - WebRtcSpl_NormW32(energy2), 0);
      int16_t scaled_energy1 = scaled_energy2 - 13;
      
      int32_t energy_ratio = WebRtcSpl_DivW32W16(
          WEBRTC_SPL_SHIFT_W32(energy1, -scaled_energy1),
          WEBRTC_SPL_RSHIFT_W32(energy2, scaled_energy2));
      
      amplitude_ratio = WebRtcSpl_SqrtFloor(energy_ratio << 13);
      
      parameters.expand_vector0.Clear();
      parameters.expand_vector0.PushBack(vector1, expansion_length);
      parameters.expand_vector1.Clear();
      if (parameters.expand_vector1.Size() <
          static_cast<size_t>(expansion_length)) {
        parameters.expand_vector1.Extend(
            expansion_length - parameters.expand_vector1.Size());
      }
      WebRtcSpl_AffineTransformVector(&parameters.expand_vector1[0],
                                      const_cast<int16_t*>(vector2),
                                      amplitude_ratio,
                                      4096,
                                      13,
                                      expansion_length);
    } else {
      
      parameters.expand_vector0.Clear();
      parameters.expand_vector0.PushBack(vector1, expansion_length);
      
      parameters.expand_vector0.CopyFrom(&parameters.expand_vector1);
      
      if ((energy1 / 4 < energy2) || (energy2 == 0)) {
        amplitude_ratio = 4096;  
      } else {
        amplitude_ratio = 16384;  
      }
    }

    
    int lag_difference = distortion_lag - correlation_lag;
    if (lag_difference == 0) {
      
      expand_lags_[0] = distortion_lag;
      expand_lags_[1] = distortion_lag;
      expand_lags_[2] = distortion_lag;
    } else {
      
      
      
      expand_lags_[0] = distortion_lag;
      
      expand_lags_[1] = (distortion_lag + correlation_lag) / 2;
      
      if (lag_difference > 0) {
        expand_lags_[2] = (distortion_lag + correlation_lag - 1) / 2;
      } else {
        expand_lags_[2] = (distortion_lag + correlation_lag + 1) / 2;
      }
    }

    
    
    correlation_scale = WebRtcSpl_MaxAbsValueW16(
        &(audio_history[signal_length - fs_mult_lpc_analysis_len]),
        fs_mult_lpc_analysis_len);

    correlation_scale = std::min(16 - WebRtcSpl_NormW32(correlation_scale), 0);
    correlation_scale = std::max(correlation_scale * 2 + 7, 0);

    
    size_t temp_index = signal_length - fs_mult_lpc_analysis_len -
        kUnvoicedLpcOrder;
    
    int16_t* temp_signal = new int16_t[fs_mult_lpc_analysis_len
                                       + kUnvoicedLpcOrder];
    memset(temp_signal, 0,
           sizeof(int16_t) * (fs_mult_lpc_analysis_len + kUnvoicedLpcOrder));
    memcpy(&temp_signal[kUnvoicedLpcOrder],
           &audio_history[temp_index + kUnvoicedLpcOrder],
           sizeof(int16_t) * fs_mult_lpc_analysis_len);
    WebRtcSpl_CrossCorrelation(auto_correlation,
                               &temp_signal[kUnvoicedLpcOrder],
                               &temp_signal[kUnvoicedLpcOrder],
                               fs_mult_lpc_analysis_len, kUnvoicedLpcOrder + 1,
                               correlation_scale, -1);
    delete [] temp_signal;

    
    if (auto_correlation[0] > 0) {
      
      
      int16_t stability = WebRtcSpl_LevinsonDurbin(auto_correlation,
                                                   parameters.ar_filter,
                                                   reflection_coeff,
                                                   kUnvoicedLpcOrder);

      
      if (stability != 1) {
        
        parameters.ar_filter[0] = 4096;
        
        WebRtcSpl_MemSetW16(parameters.ar_filter + 1, 0, kUnvoicedLpcOrder);
      }
    }

    if (channel_ix == 0) {
      
      int16_t noise_length;
      if (distortion_lag < 40) {
        noise_length = 2 * distortion_lag + 30;
      } else {
        noise_length = distortion_lag + 30;
      }
      if (noise_length <= RandomVector::kRandomTableSize) {
        memcpy(random_vector, RandomVector::kRandomTable,
               sizeof(int16_t) * noise_length);
      } else {
        
        
        memcpy(random_vector, RandomVector::kRandomTable,
               sizeof(int16_t) * RandomVector::kRandomTableSize);
        assert(noise_length <= kMaxSampleRate / 8000 * 120 + 30);
        random_vector_->IncreaseSeedIncrement(2);
        random_vector_->Generate(
            noise_length - RandomVector::kRandomTableSize,
            &random_vector[RandomVector::kRandomTableSize]);
      }
    }

    
    memcpy(parameters.ar_filter_state,
           &(audio_history[signal_length - kUnvoicedLpcOrder]),
           sizeof(int16_t) * kUnvoicedLpcOrder);
    memcpy(unvoiced_vector - kUnvoicedLpcOrder,
           &(audio_history[signal_length - 128 - kUnvoicedLpcOrder]),
           sizeof(int16_t) * kUnvoicedLpcOrder);
    WebRtcSpl_FilterMAFastQ12(
        const_cast<int16_t*>(&audio_history[signal_length - 128]),
        unvoiced_vector, parameters.ar_filter, kUnvoicedLpcOrder + 1, 128);
    int16_t unvoiced_prescale;
    if (WebRtcSpl_MaxAbsValueW16(unvoiced_vector, 128) > 4000) {
      unvoiced_prescale = 4;
    } else {
      unvoiced_prescale = 0;
    }
    int32_t unvoiced_energy = WebRtcSpl_DotProductWithScale(unvoiced_vector,
                                                            unvoiced_vector,
                                                            128,
                                                            unvoiced_prescale);

    
    int16_t unvoiced_scale = WebRtcSpl_NormW32(unvoiced_energy) - 3;
    
    
    
    unvoiced_scale += ((unvoiced_scale & 0x1) ^ 0x1);
    unvoiced_energy = WEBRTC_SPL_SHIFT_W32(unvoiced_energy, unvoiced_scale);
    int32_t unvoiced_gain = WebRtcSpl_SqrtFloor(unvoiced_energy);
    parameters.ar_gain_scale = 13
        + (unvoiced_scale + 7 - unvoiced_prescale) / 2;
    parameters.ar_gain = unvoiced_gain;

    
    
    
    
    
    
    if (corr_coefficient > 7875) {
      int16_t x1, x2, x3;
      x1 = corr_coefficient;  
      x2 = (x1 * x1) >> 14;   
      x3 = (x1 * x2) >> 14;
      static const int kCoefficients[4] = { -5179, 19931, -16422, 5776 };
      int32_t temp_sum = kCoefficients[0] << 14;
      temp_sum += kCoefficients[1] * x1;
      temp_sum += kCoefficients[2] * x2;
      temp_sum += kCoefficients[3] * x3;
      parameters.voice_mix_factor = temp_sum / 4096;
      parameters.voice_mix_factor = std::min(parameters.voice_mix_factor,
                                             static_cast<int16_t>(16384));
      parameters.voice_mix_factor = std::max(parameters.voice_mix_factor,
                                             static_cast<int16_t>(0));
    } else {
      parameters.voice_mix_factor = 0;
    }

    
    
    int16_t slope = amplitude_ratio;
    if (slope > 12288) {
      
      
      
      
      
      
      
      int16_t temp_ratio = WebRtcSpl_DivW32W16((slope - 8192) << 12,
                                               (distortion_lag * slope) >> 8);
      if (slope > 14746) {
        
        
        parameters.mute_slope = (temp_ratio + 1) / 2;
      } else {
        
        parameters.mute_slope = (temp_ratio + 4) / 8;
      }
      parameters.onset = true;
    } else {
      
      
      parameters.mute_slope = WebRtcSpl_DivW32W16((8192 - slope) << 7,
                                                   distortion_lag);
      if (parameters.voice_mix_factor <= 13107) {
        
        
        
        parameters.mute_slope = std::max(static_cast<int16_t>(5243 / fs_mult),
                                         parameters.mute_slope);
      } else if (slope > 8028) {
        parameters.mute_slope = 0;
      }
      parameters.onset = false;
    }
  }
}

int16_t Expand::Correlation(const int16_t* input, size_t input_length,
                            int16_t* output, int16_t* output_scale) const {
  
  const int16_t* filter_coefficients;
  int16_t num_coefficients;
  int16_t downsampling_factor;
  if (fs_hz_ == 8000) {
    num_coefficients = 3;
    downsampling_factor = 2;
    filter_coefficients = DspHelper::kDownsample8kHzTbl;
  } else if (fs_hz_ == 16000) {
    num_coefficients = 5;
    downsampling_factor = 4;
    filter_coefficients = DspHelper::kDownsample16kHzTbl;
  } else if (fs_hz_ == 32000) {
    num_coefficients = 7;
    downsampling_factor = 8;
    filter_coefficients = DspHelper::kDownsample32kHzTbl;
  } else {  
    num_coefficients = 7;
    downsampling_factor = 12;
    filter_coefficients = DspHelper::kDownsample48kHzTbl;
  }

  
  
  static const int kCorrelationStartLag = 10;
  static const int kNumCorrelationLags = 54;
  static const int kCorrelationLength = 60;
  
  static const int kDownsampledLength = kCorrelationStartLag
      + kNumCorrelationLags + kCorrelationLength;
  int16_t downsampled_input[kDownsampledLength];
  static const int kFilterDelay = 0;
  WebRtcSpl_DownsampleFast(
      input + input_length - kDownsampledLength * downsampling_factor,
      kDownsampledLength * downsampling_factor, downsampled_input,
      kDownsampledLength, filter_coefficients, num_coefficients,
      downsampling_factor, kFilterDelay);

  
  int16_t max_value = WebRtcSpl_MaxAbsValueW16(downsampled_input,
                                               kDownsampledLength);
  int16_t norm_shift = 16 - WebRtcSpl_NormW32(max_value);
  WebRtcSpl_VectorBitShiftW16(downsampled_input, kDownsampledLength,
                              downsampled_input, norm_shift);

  int32_t correlation[kNumCorrelationLags];
  static const int kCorrelationShift = 6;
  WebRtcSpl_CrossCorrelation(
      correlation,
      &downsampled_input[kDownsampledLength - kCorrelationLength],
      &downsampled_input[kDownsampledLength - kCorrelationLength
          - kCorrelationStartLag],
      kCorrelationLength, kNumCorrelationLags, kCorrelationShift, -1);

  
  int32_t max_correlation = WebRtcSpl_MaxAbsValueW32(correlation,
                                                     kNumCorrelationLags);
  int16_t norm_shift2 = std::max(18 - WebRtcSpl_NormW32(max_correlation), 0);
  WebRtcSpl_VectorBitShiftW32ToW16(output, kNumCorrelationLags, correlation,
                                   norm_shift2);
  
  *output_scale = 2 * norm_shift + kCorrelationShift + norm_shift2;
  return kNumCorrelationLags;
}

void Expand::UpdateLagIndex() {
  current_lag_index_ = current_lag_index_ + lag_index_direction_;
  
  if (current_lag_index_ <= 0) {
    lag_index_direction_ = 1;
  }
  if (current_lag_index_ >= kNumLags - 1) {
    lag_index_direction_ = -1;
  }
}

Expand* ExpandFactory::Create(BackgroundNoise* background_noise,
                              SyncBuffer* sync_buffer,
                              RandomVector* random_vector,
                              int fs,
                              size_t num_channels) const {
  return new Expand(background_noise, sync_buffer, random_vector, fs,
                    num_channels);
}


}  

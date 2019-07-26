









#include "webrtc/modules/audio_coding/neteq4/merge.h"

#include <assert.h>
#include <string.h>  

#include <algorithm>  

#include "webrtc/common_audio/signal_processing/include/signal_processing_library.h"
#include "webrtc/modules/audio_coding/neteq4/audio_multi_vector.h"
#include "webrtc/modules/audio_coding/neteq4/dsp_helper.h"
#include "webrtc/modules/audio_coding/neteq4/expand.h"
#include "webrtc/modules/audio_coding/neteq4/sync_buffer.h"

namespace webrtc {

int Merge::Process(int16_t* input, size_t input_length,
                   int16_t* external_mute_factor_array,
                   AudioMultiVector* output) {
  
  assert(fs_hz_ == 8000 || fs_hz_ == 16000 || fs_hz_ ==  32000 ||
         fs_hz_ == 48000);
  assert(fs_hz_ <= kMaxSampleRate);  

  int old_length;
  int expand_period;
  
  int expanded_length = GetExpandedSignal(&old_length, &expand_period);

  
  AudioMultiVector input_vector(num_channels_);
  input_vector.PushBackInterleaved(input, input_length);
  size_t input_length_per_channel = input_vector.Size();
  assert(input_length_per_channel == input_length / num_channels_);

  int16_t best_correlation_index = 0;
  size_t output_length = 0;

  for (size_t channel = 0; channel < num_channels_; ++channel) {
    int16_t* input_channel = &input_vector[channel][0];
    int16_t* expanded_channel = &expanded_[channel][0];
    int16_t expanded_max, input_max;
    int16_t new_mute_factor = SignalScaling(
        input_channel, static_cast<int>(input_length_per_channel),
        expanded_channel, &expanded_max, &input_max);

    
    
    int16_t* external_mute_factor = &external_mute_factor_array[channel];
    *external_mute_factor =
        (*external_mute_factor * expand_->MuteFactor(channel)) >> 14;

    
    if (new_mute_factor > *external_mute_factor) {
      *external_mute_factor = std::min(new_mute_factor,
                                       static_cast<int16_t>(16384));
    }

    if (channel == 0) {
      
      
      
      Downsample(input_channel, static_cast<int>(input_length_per_channel),
                 expanded_channel, expanded_length);

      
      best_correlation_index = CorrelateAndPeakSearch(
          expanded_max, input_max, old_length,
          static_cast<int>(input_length_per_channel), expand_period);
    }

    static const int kTempDataSize = 3600;
    int16_t temp_data[kTempDataSize];  
    int16_t* decoded_output = temp_data + best_correlation_index;

    
    
    int interpolation_length = std::min(
        kMaxCorrelationLength * fs_mult_,
        expanded_length - best_correlation_index);
    interpolation_length = std::min(interpolation_length,
                                    static_cast<int>(input_length_per_channel));
    if (*external_mute_factor < 16384) {
      
      
      int increment = 4194 / fs_mult_;
      *external_mute_factor = DspHelper::RampSignal(input_channel,
                                                    interpolation_length,
                                                    *external_mute_factor,
                                                    increment);
      DspHelper::UnmuteSignal(&input_channel[interpolation_length],
                              input_length_per_channel - interpolation_length,
                              external_mute_factor, increment,
                              &decoded_output[interpolation_length]);
    } else {
      
      memmove(
          &decoded_output[interpolation_length],
          &input_channel[interpolation_length],
          sizeof(int16_t) * (input_length_per_channel - interpolation_length));
    }

    
    int increment = 16384 / (interpolation_length + 1);  
    int16_t mute_factor = 16384 - increment;
    memmove(temp_data, expanded_channel,
            sizeof(int16_t) * best_correlation_index);
    DspHelper::CrossFade(&expanded_channel[best_correlation_index],
                         input_channel, interpolation_length,
                         &mute_factor, increment, decoded_output);

    output_length = best_correlation_index + input_length_per_channel;
    if (channel == 0) {
      assert(output->Empty());  
      output->AssertSize(output_length);
    } else {
      assert(output->Size() == output_length);
    }
    memcpy(&(*output)[channel][0], temp_data,
           sizeof(temp_data[0]) * output_length);
  }

  
  
  sync_buffer_->ReplaceAtIndex(*output, old_length, sync_buffer_->next_index());
  output->PopFront(old_length);

  
  
  return static_cast<int>(output_length) - old_length;
}

int Merge::GetExpandedSignal(int* old_length, int* expand_period) {
  
  *old_length = static_cast<int>(sync_buffer_->FutureLength());
  
  assert(*old_length >= static_cast<int>(expand_->overlap_length()));
  
  expand_->SetParametersForMergeAfterExpand();

  if (*old_length >= 210 * kMaxSampleRate / 8000) {
    
    
    
    
    
    
    int16_t length_diff = *old_length - 210 * kMaxSampleRate / 8000;
    sync_buffer_->InsertZerosAtIndex(length_diff, sync_buffer_->next_index());
    *old_length = 210 * kMaxSampleRate / 8000;
    
  }
  
  assert(210 * kMaxSampleRate / 8000 - *old_length >= 0);

  AudioMultiVector expanded_temp(num_channels_);
  expand_->Process(&expanded_temp);
  *expand_period = static_cast<int>(expanded_temp.Size());  
                                                            

  expanded_.Clear();
  
  expanded_.PushBackFromIndex(*sync_buffer_, sync_buffer_->next_index());
  assert(expanded_.Size() == static_cast<size_t>(*old_length));
  assert(expanded_temp.Size() > 0);
  
  
  const int required_length = (120 + 80 + 2) * fs_mult_;
  if (expanded_.Size() < static_cast<size_t>(required_length)) {
    while (expanded_.Size() < static_cast<size_t>(required_length)) {
      
      expanded_.PushBack(expanded_temp);
    }
    
    expanded_.PopBack(expanded_.Size() - required_length);
  }
  assert(expanded_.Size() >= static_cast<size_t>(required_length));
  return required_length;
}

int16_t Merge::SignalScaling(const int16_t* input, int input_length,
                             const int16_t* expanded_signal,
                             int16_t* expanded_max, int16_t* input_max) const {
  
  const int mod_input_length = std::min(64 * fs_mult_, input_length);
  *expanded_max = WebRtcSpl_MaxAbsValueW16(expanded_signal, mod_input_length);
  *input_max = WebRtcSpl_MaxAbsValueW16(input, mod_input_length);

  
  
  int log_fs_mult = 30 - WebRtcSpl_NormW32(fs_mult_);
  int expanded_shift = 6 + log_fs_mult
      - WebRtcSpl_NormW32(*expanded_max * *expanded_max);
  expanded_shift = std::max(expanded_shift, 0);
  int32_t energy_expanded = WebRtcSpl_DotProductWithScale(expanded_signal,
                                                          expanded_signal,
                                                          mod_input_length,
                                                          expanded_shift);

  
  int input_shift = 6 + log_fs_mult -
      WebRtcSpl_NormW32(*input_max * *input_max);
  input_shift = std::max(input_shift, 0);
  int32_t energy_input = WebRtcSpl_DotProductWithScale(input, input,
                                                       mod_input_length,
                                                       input_shift);

  
  if (input_shift > expanded_shift) {
    energy_expanded = energy_expanded >> (input_shift - expanded_shift);
  } else {
    energy_input = energy_input >> (expanded_shift - input_shift);
  }

  
  int16_t mute_factor;
  if (energy_input > energy_expanded) {
    
    int16_t temp_shift = WebRtcSpl_NormW32(energy_input) - 17;
    energy_input = WEBRTC_SPL_SHIFT_W32(energy_input, temp_shift);
    
    
    energy_expanded = WEBRTC_SPL_SHIFT_W32(energy_expanded, temp_shift + 14);
    
    mute_factor = WebRtcSpl_SqrtFloor((energy_expanded / energy_input) << 14);
  } else {
    
    mute_factor = 16384;
  }

  return mute_factor;
}



void Merge::Downsample(const int16_t* input, int input_length,
                       const int16_t* expanded_signal, int expanded_length) {
  const int16_t* filter_coefficients;
  int num_coefficients;
  int decimation_factor = fs_hz_ / 4000;
  static const int kCompensateDelay = 0;
  int length_limit = fs_hz_ / 100;
  if (fs_hz_ == 8000) {
    filter_coefficients = DspHelper::kDownsample8kHzTbl;
    num_coefficients = 3;
  } else if (fs_hz_ == 16000) {
    filter_coefficients = DspHelper::kDownsample16kHzTbl;
    num_coefficients = 5;
  } else if (fs_hz_ == 32000) {
    filter_coefficients = DspHelper::kDownsample32kHzTbl;
    num_coefficients = 7;
  } else {  
    filter_coefficients = DspHelper::kDownsample48kHzTbl;
    num_coefficients = 7;
    
    length_limit = 320;
  }
  int signal_offset = num_coefficients - 1;
  WebRtcSpl_DownsampleFast(&expanded_signal[signal_offset],
                           expanded_length - signal_offset,
                           expanded_downsampled_, kExpandDownsampLength,
                           filter_coefficients, num_coefficients,
                           decimation_factor, kCompensateDelay);
  if (input_length <= length_limit) {
    
    int16_t temp_len = input_length - signal_offset;
    
    
    int16_t downsamp_temp_len = temp_len / decimation_factor;
    WebRtcSpl_DownsampleFast(&input[signal_offset], temp_len,
                             input_downsampled_, downsamp_temp_len,
                             filter_coefficients, num_coefficients,
                             decimation_factor, kCompensateDelay);
    memset(&input_downsampled_[downsamp_temp_len], 0,
           sizeof(int16_t) * (kInputDownsampLength - downsamp_temp_len));
  } else {
    WebRtcSpl_DownsampleFast(&input[signal_offset],
                             input_length - signal_offset, input_downsampled_,
                             kInputDownsampLength, filter_coefficients,
                             num_coefficients, decimation_factor,
                             kCompensateDelay);
  }
}

int16_t Merge::CorrelateAndPeakSearch(int16_t expanded_max, int16_t input_max,
                                      int start_position, int input_length,
                                      int expand_period) const {
  
  const int max_corr_length = kMaxCorrelationLength;
  int stop_position_downsamp = std::min(
      max_corr_length, expand_->max_lag() / (fs_mult_ * 2) + 1);
  int16_t correlation_shift = 0;
  if (expanded_max * input_max > 26843546) {
    correlation_shift = 3;
  }

  int32_t correlation[kMaxCorrelationLength];
  WebRtcSpl_CrossCorrelation(correlation, input_downsampled_,
                             expanded_downsampled_, kInputDownsampLength,
                             stop_position_downsamp, correlation_shift, 1);

  
  static const int kPadLength = 4;
  int16_t correlation16[kPadLength + kMaxCorrelationLength + kPadLength] = {0};
  int16_t* correlation_ptr = &correlation16[kPadLength];
  int32_t max_correlation = WebRtcSpl_MaxAbsValueW32(correlation,
                                                     stop_position_downsamp);
  int16_t norm_shift = std::max(0, 17 - WebRtcSpl_NormW32(max_correlation));
  WebRtcSpl_VectorBitShiftW32ToW16(correlation_ptr, stop_position_downsamp,
                                   correlation, norm_shift);

  
  
  
  
  
  int start_index = timestamps_per_call_ +
      static_cast<int>(expand_->overlap_length());
  start_index = std::max(start_position, start_index);
  start_index = std::max(start_index - input_length, 0);
  
  int start_index_downsamp = start_index / (fs_mult_ * 2);

  
  
  int modified_stop_pos =
      std::min(stop_position_downsamp,
               kMaxCorrelationLength + kPadLength - start_index_downsamp);
  int best_correlation_index;
  int16_t best_correlation;
  static const int kNumCorrelationCandidates = 1;
  DspHelper::PeakDetection(&correlation_ptr[start_index_downsamp],
                           modified_stop_pos, kNumCorrelationCandidates,
                           fs_mult_, &best_correlation_index,
                           &best_correlation);
  
  best_correlation_index += start_index;

  
  
  
  while ((best_correlation_index + input_length) <
      static_cast<int>(timestamps_per_call_ + expand_->overlap_length()) ||
      best_correlation_index + input_length < start_position) {
    assert(false);  
    best_correlation_index += expand_period;  
  }
  return best_correlation_index;
}

}  

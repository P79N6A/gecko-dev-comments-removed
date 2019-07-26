









#include "webrtc/modules/audio_coding/neteq4/preemptive_expand.h"

#include <algorithm>  

#include "webrtc/common_audio/signal_processing/include/signal_processing_library.h"

namespace webrtc {

PreemptiveExpand::ReturnCodes PreemptiveExpand::Process(
    const int16_t* input,
    int input_length,
    int old_data_length,
    AudioMultiVector* output,
    int16_t* length_change_samples) {
  old_data_length_per_channel_ = old_data_length;
  
  
  static const int k15ms = 120;  
  if (num_channels_ == 0 ||
      input_length / num_channels_ < (2 * k15ms - 1) * fs_mult_ ||
      old_data_length >= input_length / num_channels_ - overlap_samples_) {
    
    
    output->PushBackInterleaved(input, input_length);
    return kError;
  }
  return TimeStretch::Process(input, input_length, output,
                              length_change_samples);
}

void PreemptiveExpand::SetParametersForPassiveSpeech(size_t len,
                                                     int16_t* best_correlation,
                                                     int* peak_index) const {
  
  
  *best_correlation = 0;

  
  
  
  
  *peak_index = std::min(*peak_index,
                         static_cast<int>(len - old_data_length_per_channel_));
}

PreemptiveExpand::ReturnCodes PreemptiveExpand::CheckCriteriaAndStretch(
    const int16_t *input, size_t input_length, size_t peak_index,
    int16_t best_correlation, bool active_speech,
    AudioMultiVector* output) const {
  
  
  int fs_mult_120 = fs_mult_ * 120;
  assert(old_data_length_per_channel_ >= 0);  
  
  
  if (((best_correlation > kCorrelationThreshold) &&
      (old_data_length_per_channel_ <= fs_mult_120)) ||
      !active_speech) {
    

    
    size_t unmodified_length = std::max(old_data_length_per_channel_,
                                        fs_mult_120);
    
    output->PushBackInterleaved(
        input, (unmodified_length + peak_index) * num_channels_);
    
    AudioMultiVector temp_vector(num_channels_);
    temp_vector.PushBackInterleaved(
        &input[(unmodified_length - peak_index) * num_channels_],
        peak_index * num_channels_);
    
    output->CrossFade(temp_vector, peak_index);
    
    output->PushBackInterleaved(
        &input[unmodified_length * num_channels_],
        input_length - unmodified_length * num_channels_);

    if (active_speech) {
      return kSuccess;
    } else {
      return kSuccessLowEnergy;
    }
  } else {
    
    output->PushBackInterleaved(input, input_length);
    return kNoStretch;
  }
}

PreemptiveExpand* PreemptiveExpandFactory::Create(
    int sample_rate_hz,
    size_t num_channels,
    const BackgroundNoise& background_noise) const {
  return new PreemptiveExpand(sample_rate_hz, num_channels, background_noise);
}

}  

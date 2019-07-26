









#include "webrtc/modules/audio_coding/neteq4/accelerate.h"

#include "webrtc/common_audio/signal_processing/include/signal_processing_library.h"

namespace webrtc {

Accelerate::ReturnCodes Accelerate::Process(
    const int16_t* input,
    size_t input_length,
    AudioMultiVector* output,
    int16_t* length_change_samples) {
  
  static const int k15ms = 120;  
  if (num_channels_ == 0 || static_cast<int>(input_length) / num_channels_ <
      (2 * k15ms - 1) * fs_mult_) {
    
    
    output->PushBackInterleaved(input, input_length);
    return kError;
  }
  return TimeStretch::Process(input, input_length, output,
                              length_change_samples);
}

void Accelerate::SetParametersForPassiveSpeech(size_t ,
                                               int16_t* best_correlation,
                                               int* ) const {
  
  
  *best_correlation = 0;
}

Accelerate::ReturnCodes Accelerate::CheckCriteriaAndStretch(
    const int16_t* input, size_t input_length, size_t peak_index,
    int16_t best_correlation, bool active_speech,
    AudioMultiVector* output) const {
  
  if ((best_correlation > kCorrelationThreshold) || !active_speech) {
    

    
    
    size_t fs_mult_120 = fs_mult_ * 120;

    assert(fs_mult_120 >= peak_index);  
    
    output->PushBackInterleaved(input, fs_mult_120 * num_channels_);
    
    AudioMultiVector temp_vector(num_channels_);
    temp_vector.PushBackInterleaved(&input[fs_mult_120 * num_channels_],
                                    peak_index * num_channels_);
    
    output->CrossFade(temp_vector, peak_index);
    
    output->PushBackInterleaved(
        &input[(fs_mult_120 + peak_index) * num_channels_],
        input_length - (fs_mult_120 + peak_index) * num_channels_);

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

Accelerate* AccelerateFactory::Create(
    int sample_rate_hz,
    size_t num_channels,
    const BackgroundNoise& background_noise) const {
  return new Accelerate(sample_rate_hz, num_channels, background_noise);
}

}  

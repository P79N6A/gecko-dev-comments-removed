









#include "webrtc/modules/audio_coding/neteq4/time_stretch.h"

#include <algorithm>  

#include "webrtc/common_audio/signal_processing/include/signal_processing_library.h"
#include "webrtc/modules/audio_coding/neteq4/background_noise.h"
#include "webrtc/modules/audio_coding/neteq4/dsp_helper.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

TimeStretch::ReturnCodes TimeStretch::Process(
    const int16_t* input,
    size_t input_len,
    AudioMultiVector* output,
    int16_t* length_change_samples) {

  
  int fs_mult_120 = fs_mult_ * 120;  

  const int16_t* signal;
  scoped_array<int16_t> signal_array;
  size_t signal_len;
  if (num_channels_ == 1) {
    signal = input;
    signal_len = input_len;
  } else {
    
    
    
    signal_len = input_len / num_channels_;
    signal_array.reset(new int16_t[signal_len]);
    signal = signal_array.get();
    size_t j = master_channel_;
    for (size_t i = 0; i < signal_len; ++i) {
      signal_array[i] = input[j];
      j += num_channels_;
    }
  }

  
  max_input_value_ = WebRtcSpl_MaxAbsValueW16(signal,
                                              static_cast<int>(signal_len));

  
  DspHelper::DownsampleTo4kHz(signal, signal_len, kDownsampledLen,
                              sample_rate_hz_, true ,
                              downsampled_input_);
  AutoCorrelation();

  
  static const int kNumPeaks = 1;
  int peak_index;
  int16_t peak_value;
  DspHelper::PeakDetection(auto_correlation_, kCorrelationLen, kNumPeaks,
                           fs_mult_, &peak_index, &peak_value);
  
  assert(peak_index >= 0);
  assert(peak_index <= (2 * kCorrelationLen - 1) * fs_mult_);

  
  
  
  
  peak_index += kMinLag * fs_mult_ * 2;
  
  assert(peak_index >= 20 * fs_mult_);
  assert(peak_index <= 20 * fs_mult_ + (2 * kCorrelationLen - 1) * fs_mult_);

  
  
  int scaling = 31 - WebRtcSpl_NormW32(max_input_value_ * max_input_value_) -
      WebRtcSpl_NormW32(peak_index);
  scaling = std::max(0, scaling);

  
  const int16_t* vec1 = &signal[fs_mult_120 - peak_index];
  
  const int16_t* vec2 = &signal[fs_mult_120];
  
  
  int32_t vec1_energy =
      WebRtcSpl_DotProductWithScale(vec1, vec1, peak_index, scaling);
  int32_t vec2_energy =
      WebRtcSpl_DotProductWithScale(vec2, vec2, peak_index, scaling);

  
  int32_t cross_corr =
      WebRtcSpl_DotProductWithScale(vec1, vec2, peak_index, scaling);

  
  bool active_speech = SpeechDetection(vec1_energy, vec2_energy, peak_index,
                                       scaling);

  int16_t best_correlation;
  if (!active_speech) {
    SetParametersForPassiveSpeech(signal_len, &best_correlation, &peak_index);
  } else {
    
    

    
    int energy1_scale = std::max(0, 16 - WebRtcSpl_NormW32(vec1_energy));
    int energy2_scale = std::max(0, 16 - WebRtcSpl_NormW32(vec2_energy));

    
    if ((energy1_scale + energy2_scale) & 1) {
      
      energy1_scale += 1;
    }

    
    int16_t vec1_energy_int16 =
        static_cast<int16_t>(vec1_energy >> energy1_scale);
    int16_t vec2_energy_int16 =
        static_cast<int16_t>(vec2_energy >> energy2_scale);

    
    int16_t sqrt_energy_prod = WebRtcSpl_SqrtFloor(vec1_energy_int16 *
                                                   vec2_energy_int16);

    
    int temp_scale = 14 - (energy1_scale + energy2_scale) / 2;
    cross_corr = WEBRTC_SPL_SHIFT_W32(cross_corr, temp_scale);
    cross_corr = std::max(0, cross_corr);  
    best_correlation = WebRtcSpl_DivW32W16(cross_corr, sqrt_energy_prod);
    
    best_correlation = std::min(static_cast<int16_t>(16384), best_correlation);
  }


  
  ReturnCodes return_value = CheckCriteriaAndStretch(
      input, input_len, peak_index, best_correlation, active_speech, output);
  switch (return_value) {
    case kSuccess:
      *length_change_samples = peak_index;
      break;
    case kSuccessLowEnergy:
      *length_change_samples = peak_index;
      break;
    case kNoStretch:
    case kError:
      *length_change_samples = 0;
      break;
  }
  return return_value;
}

void TimeStretch::AutoCorrelation() {
  
  int scaling = kLogCorrelationLen - WebRtcSpl_NormW32(
      max_input_value_ * max_input_value_);
  scaling = std::max(0, scaling);

  
  int32_t auto_corr[kCorrelationLen];
  WebRtcSpl_CrossCorrelation(auto_corr, &downsampled_input_[kMaxLag],
                             &downsampled_input_[kMaxLag - kMinLag],
                             kCorrelationLen, kMaxLag - kMinLag, scaling, -1);

  
  int32_t max_corr = WebRtcSpl_MaxAbsValueW32(auto_corr, kCorrelationLen);
  scaling = std::max(0, 17 - WebRtcSpl_NormW32(max_corr));
  WebRtcSpl_VectorBitShiftW32ToW16(auto_correlation_, kCorrelationLen,
                                   auto_corr, scaling);
}

bool TimeStretch::SpeechDetection(int32_t vec1_energy, int32_t vec2_energy,
                                  int peak_index, int scaling) const {
  
  
  
  
  
  
  
  
  int32_t left_side = (vec1_energy + vec2_energy) / 16;
  int32_t right_side;
  if (background_noise_.initialized()) {
    right_side = background_noise_.Energy(master_channel_);
  } else {
    
    right_side = 75000;
  }
  int right_scale = 16 - WebRtcSpl_NormW32(right_side);
  right_scale = std::max(0, right_scale);
  left_side = left_side >> right_scale;
  right_side = peak_index * (right_side >> right_scale);

  
  
  
  if (WebRtcSpl_NormW32(left_side) < 2 * scaling) {
    
    int temp_scale = WebRtcSpl_NormW32(left_side);
    left_side = left_side << temp_scale;
    right_side = right_side >> (2 * scaling - temp_scale);
  } else {
    left_side = left_side << 2 * scaling;
  }
  return left_side > right_side;
}

}  

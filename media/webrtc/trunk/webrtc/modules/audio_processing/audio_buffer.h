









#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_AUDIO_BUFFER_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_AUDIO_BUFFER_H_

#include <vector>

#include "webrtc/modules/audio_processing/common.h"
#include "webrtc/modules/audio_processing/include/audio_processing.h"
#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/system_wrappers/interface/scoped_vector.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class PushSincResampler;
class IFChannelBuffer;

struct SplitFilterStates {
  SplitFilterStates() {
    memset(analysis_filter_state1, 0, sizeof(analysis_filter_state1));
    memset(analysis_filter_state2, 0, sizeof(analysis_filter_state2));
    memset(synthesis_filter_state1, 0, sizeof(synthesis_filter_state1));
    memset(synthesis_filter_state2, 0, sizeof(synthesis_filter_state2));
  }

  static const int kStateSize = 6;
  int analysis_filter_state1[kStateSize];
  int analysis_filter_state2[kStateSize];
  int synthesis_filter_state1[kStateSize];
  int synthesis_filter_state2[kStateSize];
};

class AudioBuffer {
 public:
  
  AudioBuffer(int input_samples_per_channel,
              int num_input_channels,
              int process_samples_per_channel,
              int num_process_channels,
              int output_samples_per_channel);
  virtual ~AudioBuffer();

  int num_channels() const;
  int samples_per_channel() const;
  int samples_per_split_channel() const;
  int samples_per_keyboard_channel() const;

  
  
  
  int16_t* data(int channel);
  const int16_t* data(int channel) const;
  int16_t* low_pass_split_data(int channel);
  const int16_t* low_pass_split_data(int channel) const;
  int16_t* high_pass_split_data(int channel);
  const int16_t* high_pass_split_data(int channel) const;\
  
  
  const int16_t* mixed_low_pass_data();
  const int16_t* low_pass_reference(int channel) const;

  
  
  float* data_f(int channel);
  const float* data_f(int channel) const;

  float* const* channels_f();
  const float* const* channels_f() const;

  float* low_pass_split_data_f(int channel);
  const float* low_pass_split_data_f(int channel) const;
  float* high_pass_split_data_f(int channel);
  const float* high_pass_split_data_f(int channel) const;

  float* const* low_pass_split_channels_f();
  const float* const* low_pass_split_channels_f() const;
  float* const* high_pass_split_channels_f();
  const float* const* high_pass_split_channels_f() const;

  const float* keyboard_data() const;

  SplitFilterStates* filter_states(int channel);

  void set_activity(AudioFrame::VADActivity activity);
  AudioFrame::VADActivity activity() const;

  
  void DeinterleaveFrom(AudioFrame* audioFrame);
  
  
  void InterleaveTo(AudioFrame* frame, bool data_changed) const;

  
  void CopyFrom(const float* const* data,
                int samples_per_channel,
                AudioProcessing::ChannelLayout layout);
  void CopyTo(int samples_per_channel,
              AudioProcessing::ChannelLayout layout,
              float* const* data);
  void CopyLowPassToReference();

 private:
  
  void InitForNewData();

  const int input_samples_per_channel_;
  const int num_input_channels_;
  const int proc_samples_per_channel_;
  const int num_proc_channels_;
  const int output_samples_per_channel_;
  int samples_per_split_channel_;
  bool mixed_low_pass_valid_;
  bool reference_copied_;
  AudioFrame::VADActivity activity_;

  const float* keyboard_data_;
  scoped_ptr<IFChannelBuffer> channels_;
  scoped_ptr<IFChannelBuffer> split_channels_low_;
  scoped_ptr<IFChannelBuffer> split_channels_high_;
  scoped_ptr<SplitFilterStates[]> filter_states_;
  scoped_ptr<ChannelBuffer<int16_t> > mixed_low_pass_channels_;
  scoped_ptr<ChannelBuffer<int16_t> > low_pass_reference_channels_;
  scoped_ptr<ChannelBuffer<float> > input_buffer_;
  scoped_ptr<ChannelBuffer<float> > process_buffer_;
  ScopedVector<PushSincResampler> input_resamplers_;
  ScopedVector<PushSincResampler> output_resamplers_;
};

}  

#endif  

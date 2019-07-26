









#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_MAIN_SOURCE_AUDIO_BUFFER_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_MAIN_SOURCE_AUDIO_BUFFER_H_

#include "module_common_types.h"
#include "scoped_ptr.h"
#include "typedefs.h"

namespace webrtc {

struct AudioChannel;
struct SplitAudioChannel;

class AudioBuffer {
 public:
  AudioBuffer(int max_num_channels, int samples_per_channel);
  virtual ~AudioBuffer();

  int num_channels() const;
  int samples_per_channel() const;
  int samples_per_split_channel() const;

  int16_t* data(int channel) const;
  int16_t* low_pass_split_data(int channel) const;
  int16_t* high_pass_split_data(int channel) const;
  int16_t* mixed_data(int channel) const;
  int16_t* mixed_low_pass_data(int channel) const;
  int16_t* low_pass_reference(int channel) const;

  int32_t* analysis_filter_state1(int channel) const;
  int32_t* analysis_filter_state2(int channel) const;
  int32_t* synthesis_filter_state1(int channel) const;
  int32_t* synthesis_filter_state2(int channel) const;

  void set_activity(AudioFrame::VADActivity activity);
  AudioFrame::VADActivity activity() const;

  bool is_muted() const;

  void DeinterleaveFrom(AudioFrame* audioFrame);
  void InterleaveTo(AudioFrame* audioFrame) const;
  
  
  void InterleaveTo(AudioFrame* frame, bool data_changed) const;
  void Mix(int num_mixed_channels);
  void CopyAndMix(int num_mixed_channels);
  void CopyAndMixLowPass(int num_mixed_channels);
  void CopyLowPassToReference();

 private:
  const int max_num_channels_;
  int num_channels_;
  int num_mixed_channels_;
  int num_mixed_low_pass_channels_;
  
  bool data_was_mixed_;
  const int samples_per_channel_;
  int samples_per_split_channel_;
  bool reference_copied_;
  AudioFrame::VADActivity activity_;
  bool is_muted_;

  int16_t* data_;
  scoped_array<AudioChannel> channels_;
  scoped_array<SplitAudioChannel> split_channels_;
  scoped_array<AudioChannel> mixed_channels_;
  
  scoped_array<AudioChannel> mixed_low_pass_channels_;
  scoped_array<AudioChannel> low_pass_reference_channels_;
};
}  

#endif  

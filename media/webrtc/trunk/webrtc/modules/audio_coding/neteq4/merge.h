









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ4_MERGE_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ4_MERGE_H_

#include <assert.h>

#include "webrtc/modules/audio_coding/neteq4/audio_multi_vector.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/typedefs.h"

namespace webrtc {


class Expand;
class SyncBuffer;









class Merge {
 public:
  Merge(int fs_hz, size_t num_channels, Expand* expand, SyncBuffer* sync_buffer)
      : fs_hz_(fs_hz),
        fs_mult_(fs_hz_ / 8000),
        num_channels_(num_channels),
        timestamps_per_call_(fs_hz_ / 100),
        expand_(expand),
        sync_buffer_(sync_buffer),
        expanded_(num_channels_) {
    assert(num_channels_ > 0);
  }

  
  
  
  
  
  
  
  int Process(int16_t* input, size_t input_length,
              int16_t* external_mute_factor_array,
              AudioMultiVector* output);

 private:
  static const int kMaxSampleRate = 48000;
  static const int kExpandDownsampLength = 100;
  static const int kInputDownsampLength = 40;
  static const int kMaxCorrelationLength = 60;

  
  
  
  
  
  int GetExpandedSignal(int* old_length, int* expand_period);

  
  
  int16_t SignalScaling(const int16_t* input, int input_length,
                        const int16_t* expanded_signal,
                        int16_t* expanded_max, int16_t* input_max) const;

  
  
  
  void Downsample(const int16_t* input, int input_length,
                  const int16_t* expanded_signal, int expanded_length);

  
  
  
  int16_t CorrelateAndPeakSearch(int16_t expanded_max, int16_t input_max,
                                 int start_position, int input_length,
                                 int expand_period) const;

  const int fs_hz_;
  const int fs_mult_;  
  const size_t num_channels_;
  const int timestamps_per_call_;
  Expand* expand_;
  SyncBuffer* sync_buffer_;
  int16_t expanded_downsampled_[kExpandDownsampLength];
  int16_t input_downsampled_[kInputDownsampLength];
  AudioMultiVector expanded_;

  DISALLOW_COPY_AND_ASSIGN(Merge);
};

}  
#endif  

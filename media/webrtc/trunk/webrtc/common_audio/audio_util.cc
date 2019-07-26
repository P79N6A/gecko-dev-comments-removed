









#include "webrtc/common_audio/include/audio_util.h"

#include "webrtc/typedefs.h"

namespace webrtc {

void Deinterleave(const int16_t* interleaved, int samples_per_channel,
                  int num_channels, int16_t** deinterleaved) {
  for (int i = 0; i < num_channels; i++) {
    int16_t* channel = deinterleaved[i];
    int interleaved_idx = i;
    for (int j = 0; j < samples_per_channel; j++) {
      channel[j] = interleaved[interleaved_idx];
      interleaved_idx += num_channels;
    }
  }
}

void Interleave(const int16_t* const* deinterleaved, int samples_per_channel,
                int num_channels, int16_t* interleaved) {
  for (int i = 0; i < num_channels; ++i) {
    const int16_t* channel = deinterleaved[i];
    int interleaved_idx = i;
    for (int j = 0; j < samples_per_channel; j++) {
      interleaved[interleaved_idx] = channel[j];
      interleaved_idx += num_channels;
    }
  }
}

}  

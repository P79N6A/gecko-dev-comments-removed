









#ifndef WEBRTC_INTERNAL_BEAMFORMER_BLOCKER_H_
#define WEBRTC_INTERNAL_BEAMFORMER_BLOCKER_H_

#include "webrtc/modules/audio_processing/common.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {




class BlockerCallback {
 public:
  virtual ~BlockerCallback() {}

  virtual void ProcessBlock(const float* const* input,
                            int num_frames,
                            int num_input_channels,
                            int num_output_channels,
                            float* const* output) = 0;
};




























class Blocker {
 public:
  Blocker(int chunk_size,
          int block_size,
          int num_input_channels,
          int num_output_channels,
          const float* window,
          int shift_amount,
          BlockerCallback* callback);

  void ProcessChunk(const float* const* input,
                    int num_frames,
                    int num_input_channels,
                    int num_output_channels,
                    float* const* output);

 private:
  const int chunk_size_;
  const int block_size_;
  const int num_input_channels_;
  const int num_output_channels_;

  
  
  
  const int initial_delay_;

  
  
  
  
  int frame_offset_;

  
  
  
  
  
  
  ChannelBuffer<float> input_buffer_;
  ChannelBuffer<float> output_buffer_;

  
  ChannelBuffer<float> input_block_;

  
  ChannelBuffer<float> output_block_;

  scoped_ptr<float[]> window_;

  
  
  int shift_amount_;

  BlockerCallback* callback_;
};

}  

#endif  

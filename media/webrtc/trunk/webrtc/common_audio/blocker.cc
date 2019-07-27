









#include "webrtc/common_audio/blocker.h"

#include <string.h>

#include "webrtc/base/checks.h"

namespace {


void AddFrames(const float* const* a,
               int a_start_index,
               const float* const* b,
               int b_start_index,
               int num_frames,
               int num_channels,
               float* const* result,
               int result_start_index) {
  for (int i = 0; i < num_channels; ++i) {
    for (int j = 0; j < num_frames; ++j) {
      result[i][j + result_start_index] =
          a[i][j + a_start_index] + b[i][j + b_start_index];
    }
  }
}


void CopyFrames(const float* const* src,
                int src_start_index,
                int num_frames,
                int num_channels,
                float* const* dst,
                int dst_start_index) {
  for (int i = 0; i < num_channels; ++i) {
    memcpy(&dst[i][dst_start_index],
           &src[i][src_start_index],
           num_frames * sizeof(float));
  }
}

void ZeroOut(float* const* buffer,
             int starting_idx,
             int num_frames,
             int num_channels) {
  for (int i = 0; i < num_channels; ++i) {
    memset(&buffer[i][starting_idx], 0, num_frames * sizeof(float));
  }
}



void ApplyWindow(const float* window,
                 int num_frames,
                 int num_channels,
                 float* const* frames) {
  for (int i = 0; i < num_channels; ++i) {
    for (int j = 0; j < num_frames; ++j) {
      frames[i][j] = frames[i][j] * window[j];
    }
  }
}

}  

namespace webrtc {

Blocker::Blocker(int chunk_size,
                 int block_size,
                 int num_input_channels,
                 int num_output_channels,
                 const float* window,
                 int shift_amount,
                 BlockerCallback* callback)
    : chunk_size_(chunk_size),
      block_size_(block_size),
      num_input_channels_(num_input_channels),
      num_output_channels_(num_output_channels),
      initial_delay_(block_size_),
      frame_offset_(0),
      input_buffer_(chunk_size_ + initial_delay_, num_input_channels_),
      output_buffer_(chunk_size_ + initial_delay_, num_output_channels_),
      input_block_(block_size_, num_input_channels_),
      output_block_(block_size_, num_output_channels_),
      window_(new float[block_size_]),
      shift_amount_(shift_amount),
      callback_(callback) {
  CHECK_LE(num_output_channels_, num_input_channels_);
  CHECK_GE(chunk_size_, block_size_);

  memcpy(window_.get(), window, block_size_ * sizeof(float));
  size_t buffer_size = chunk_size_ + initial_delay_;
  memset(input_buffer_.channels()[0],
         0,
         buffer_size * num_input_channels_ * sizeof(float));
  memset(output_buffer_.channels()[0],
         0,
         buffer_size * num_output_channels_ * sizeof(float));
}























void Blocker::ProcessChunk(const float* const* input,
                           int chunk_size,
                           int num_input_channels,
                           int num_output_channels,
                           float* const* output) {
  CHECK_EQ(chunk_size, chunk_size_);
  CHECK_EQ(num_input_channels, num_input_channels_);
  CHECK_EQ(num_output_channels, num_output_channels_);

  
  
  CopyFrames(input,
             0,
             chunk_size_,
             num_input_channels_,
             input_buffer_.channels(),
             initial_delay_);

  int first_frame_in_block = frame_offset_;

  
  while (first_frame_in_block < chunk_size_) {
    CopyFrames(input_buffer_.channels(),
               first_frame_in_block,
               block_size_,
               num_input_channels_,
               input_block_.channels(),
               0);

    ApplyWindow(window_.get(),
                block_size_,
                num_input_channels_,
                input_block_.channels());
    callback_->ProcessBlock(input_block_.channels(),
                            block_size_,
                            num_input_channels_,
                            num_output_channels_,
                            output_block_.channels());
    ApplyWindow(window_.get(),
                block_size_,
                num_output_channels_,
                output_block_.channels());

    AddFrames(output_buffer_.channels(),
              first_frame_in_block,
              output_block_.channels(),
              0,
              block_size_,
              num_output_channels_,
              output_buffer_.channels(),
              first_frame_in_block);

    first_frame_in_block += shift_amount_;
  }

  
  CopyFrames(output_buffer_.channels(),
             0,
             chunk_size_,
             num_output_channels_,
             output,
             0);

  
  
  CopyFrames(input_buffer_.channels(),
             chunk_size,
             initial_delay_,
             num_input_channels_,
             input_buffer_.channels(),
             0);

  
  
  CopyFrames(output_buffer_.channels(),
             chunk_size,
             initial_delay_,
             num_output_channels_,
             output_buffer_.channels(),
             0);
  ZeroOut(output_buffer_.channels(),
          initial_delay_,
          chunk_size_,
          num_output_channels_);

  
  frame_offset_ = first_frame_in_block - chunk_size_;
}

}  

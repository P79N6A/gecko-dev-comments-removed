









#include "webrtc/modules/audio_coding/neteq4/tools/audio_loop.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

namespace webrtc {
namespace test {

bool AudioLoop::Init(const std::string file_name,
                     size_t max_loop_length_samples,
                     size_t block_length_samples) {
  FILE* fp = fopen(file_name.c_str(), "rb");
  if (!fp) return false;

  audio_array_.reset(new int16_t[max_loop_length_samples +
                                 block_length_samples]);
  size_t samples_read = fread(audio_array_.get(), sizeof(int16_t),
                              max_loop_length_samples, fp);
  fclose(fp);

  
  if (block_length_samples > samples_read) return false;

  
  
  
  memcpy(&audio_array_[samples_read], audio_array_.get(),
         block_length_samples * sizeof(int16_t));

  loop_length_samples_ = samples_read;
  block_length_samples_ = block_length_samples;
  return true;
}

const int16_t* AudioLoop::GetNextBlock() {
  
  if (block_length_samples_ == 0) return NULL;

  const int16_t* output_ptr = &audio_array_[next_index_];
  next_index_ = (next_index_ + block_length_samples_) % loop_length_samples_;
  return output_ptr;
}


}  
}  

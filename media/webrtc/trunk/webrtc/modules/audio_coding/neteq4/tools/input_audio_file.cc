









#include "webrtc/modules/audio_coding/neteq4/tools/input_audio_file.h"

namespace webrtc {
namespace test {

bool InputAudioFile::Read(size_t samples, int16_t* destination) {
  if (!fp_) {
    return false;
  }
  size_t bytes_read = fread(destination, sizeof(int16_t), samples, fp_);
  if (bytes_read < samples) {
    
    rewind(fp_);
    size_t missing_samples = samples - bytes_read;
    if (fread(destination, sizeof(int16_t), missing_samples, fp_) <
        missing_samples) {
      
      return false;
    }
  }
  return true;
}

void InputAudioFile::DuplicateInterleaved(const int16_t* source, size_t samples,
                                          size_t channels,
                                          int16_t* destination) {
  for (size_t i = 0; i < samples; ++i) {
    for (size_t j = 0; j < channels; ++j) {
      destination[i * channels + j] = source[i];
    }
  }
}

}  
}  

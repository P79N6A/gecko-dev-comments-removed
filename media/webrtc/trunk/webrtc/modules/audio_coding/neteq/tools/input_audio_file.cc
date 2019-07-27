









#include "webrtc/modules/audio_coding/neteq/tools/input_audio_file.h"

namespace webrtc {
namespace test {

InputAudioFile::InputAudioFile(const std::string file_name) {
  fp_ = fopen(file_name.c_str(), "rb");
}

InputAudioFile::~InputAudioFile() { fclose(fp_); }

bool InputAudioFile::Read(size_t samples, int16_t* destination) {
  if (!fp_) {
    return false;
  }
  size_t samples_read = fread(destination, sizeof(int16_t), samples, fp_);
  if (samples_read < samples) {
    
    rewind(fp_);
    size_t missing_samples = samples - samples_read;
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
  
  
  
  for (int i = static_cast<int>(samples - 1); i >= 0; --i) {
    for (int j = static_cast<int>(channels - 1); j >= 0; --j) {
      destination[i * channels + j] = source[i];
    }
  }
}

}  
}  

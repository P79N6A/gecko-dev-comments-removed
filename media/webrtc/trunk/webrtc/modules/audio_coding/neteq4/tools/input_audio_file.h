









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ4_TOOLS_INPUT_AUDIO_FILE_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ4_TOOLS_INPUT_AUDIO_FILE_H_

#include <stdio.h>

#include <string>

#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/typedefs.h"

namespace webrtc {
namespace test {


class InputAudioFile {
 public:
  explicit InputAudioFile(const std::string file_name);

  virtual ~InputAudioFile();

  
  
  
  
  bool Read(size_t samples, int16_t* destination);

  
  
  
  
  static void DuplicateInterleaved(const int16_t* source, size_t samples,
                                   size_t channels, int16_t* destination);

 private:
  FILE* fp_;
  DISALLOW_COPY_AND_ASSIGN(InputAudioFile);
};

}  
}  
#endif  

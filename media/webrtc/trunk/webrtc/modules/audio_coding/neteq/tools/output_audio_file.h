









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ_TOOLS_OUTPUT_AUDIO_FILE_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ_TOOLS_OUTPUT_AUDIO_FILE_H_

#include <assert.h>
#include <stdio.h>
#include <string>

#include "webrtc/base/constructormagic.h"
#include "webrtc/modules/audio_coding/neteq/tools/audio_sink.h"

namespace webrtc {
namespace test {

class OutputAudioFile : public AudioSink {
 public:
  
  
  explicit OutputAudioFile(const std::string& file_name) {
    out_file_ = fopen(file_name.c_str(), "wb");
  }

  virtual ~OutputAudioFile() {
    if (out_file_)
      fclose(out_file_);
  }

  virtual bool WriteArray(const int16_t* audio, size_t num_samples) OVERRIDE {
    assert(out_file_);
    return fwrite(audio, sizeof(*audio), num_samples, out_file_) == num_samples;
  }

 private:
  FILE* out_file_;

  DISALLOW_COPY_AND_ASSIGN(OutputAudioFile);
};

}  
}  
#endif  

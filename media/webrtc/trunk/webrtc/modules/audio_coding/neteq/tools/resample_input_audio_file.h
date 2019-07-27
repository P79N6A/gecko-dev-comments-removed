









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ_TOOLS_RESAMPLE_INPUT_AUDIO_FILE_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ_TOOLS_RESAMPLE_INPUT_AUDIO_FILE_H_

#include <string>

#include "webrtc/base/constructormagic.h"
#include "webrtc/common_audio/resampler/include/resampler.h"
#include "webrtc/modules/audio_coding/neteq/tools/input_audio_file.h"
#include "webrtc/typedefs.h"

namespace webrtc {
namespace test {


class ResampleInputAudioFile : public InputAudioFile {
 public:
  ResampleInputAudioFile(const std::string file_name, int file_rate_hz)
      : InputAudioFile(file_name), file_rate_hz_(file_rate_hz) {}

  bool Read(size_t samples, int output_rate_hz, int16_t* destination);

 private:
  const int file_rate_hz_;
  Resampler resampler_;
  DISALLOW_COPY_AND_ASSIGN(ResampleInputAudioFile);
};

}  
}  
#endif  











#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ_TOOLS_AUDIO_LOOP_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ_TOOLS_AUDIO_LOOP_H_

#include <string>

#include "webrtc/base/constructormagic.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/typedefs.h"

namespace webrtc {
namespace test {



class AudioLoop {
 public:
  AudioLoop()
      : next_index_(0),
        loop_length_samples_(0),
        block_length_samples_(0) {
  }

  virtual ~AudioLoop() {}

  
  
  
  
  
  bool Init(const std::string file_name, size_t max_loop_length_samples,
            size_t block_length_samples);

  
  
  
  const int16_t* GetNextBlock();

 private:
  size_t next_index_;
  size_t loop_length_samples_;
  size_t block_length_samples_;
  scoped_ptr<int16_t[]> audio_array_;

  DISALLOW_COPY_AND_ASSIGN(AudioLoop);
};

}  
}  
#endif  

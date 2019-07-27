









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ_TOOLS_AUDIO_CHECKSUM_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ_TOOLS_AUDIO_CHECKSUM_H_

#include <string>

#include "webrtc/base/constructormagic.h"
#include "webrtc/base/md5digest.h"
#include "webrtc/base/stringencode.h"
#include "webrtc/modules/audio_coding/neteq/tools/audio_sink.h"
#include "webrtc/system_wrappers/interface/compile_assert.h"
#include "webrtc/typedefs.h"

namespace webrtc {
namespace test {

class AudioChecksum : public AudioSink {
 public:
  AudioChecksum() : finished_(false) {}

  virtual bool WriteArray(const int16_t* audio, size_t num_samples) OVERRIDE {
    if (finished_)
      return false;

#ifndef WEBRTC_ARCH_LITTLE_ENDIAN
#error "Big-endian gives a different checksum"
#endif
    checksum_.Update(audio, num_samples * sizeof(*audio));
    return true;
  }

  
  std::string Finish() {
    if (!finished_) {
      finished_ = true;
      checksum_.Finish(checksum_result_, rtc::Md5Digest::kSize);
    }
    return rtc::hex_encode(checksum_result_, rtc::Md5Digest::kSize);
  }

 private:
  rtc::Md5Digest checksum_;
  char checksum_result_[rtc::Md5Digest::kSize];
  bool finished_;

  DISALLOW_COPY_AND_ASSIGN(AudioChecksum);
};

}  
}  
#endif  

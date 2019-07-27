









#ifndef WEBRTC_COMMON_AUDIO_VAD_MOCK_MOCK_VAD_H_
#define WEBRTC_COMMON_AUDIO_VAD_MOCK_MOCK_VAD_H_

#include "webrtc/common_audio/vad/include/vad.h"

#include "testing/gmock/include/gmock/gmock.h"

namespace webrtc {

class MockVad : public Vad {
 public:
  explicit MockVad(enum Aggressiveness mode) {}
  virtual ~MockVad() { Die(); }
  MOCK_METHOD0(Die, void());

  MOCK_METHOD3(VoiceActivity,
               enum Activity(const int16_t* audio,
                             size_t num_samples,
                             int sample_rate_hz));
};

}  

#endif  











#ifndef WEBRTC_VOICE_ENGINE_MOCK_VOE_OBSERVER_H_
#define WEBRTC_VOICE_ENGINE_MOCK_VOE_OBSERVER_H_

#include "testing/gmock/include/gmock/gmock.h"
#include "webrtc/voice_engine/include/voe_base.h"

namespace webrtc {

class MockVoEObserver: public VoiceEngineObserver {
 public:
  MockVoEObserver() {}
  virtual ~MockVoEObserver() {}

  MOCK_METHOD2(CallbackOnError, void(int channel, int error_code));
};

}

#endif  

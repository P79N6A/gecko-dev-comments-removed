









#ifndef MOCK_VOE_CONNECTION_OBSERVER_H_
#define MOCK_VOE_CONNECTION_OBSERVER_H_

#include "webrtc/voice_engine/include/voe_network.h"

#include "testing/gmock/include/gmock/gmock.h"

namespace webrtc {

class MockVoeConnectionObserver : public VoEConnectionObserver {
 public:
  MOCK_METHOD2(OnPeriodicDeadOrAlive, void(int channel,
                                           bool alive));
};

}

#endif  

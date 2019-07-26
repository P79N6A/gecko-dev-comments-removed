









#ifndef MOCK_VOE_CONNECTION_OBSERVER_H_
#define MOCK_VOE_CONNECTION_OBSERVER_H_

#include "voice_engine/include/voe_network.h"

namespace webrtc {

class MockVoeConnectionObserver : public VoEConnectionObserver {
 public:
  MOCK_METHOD2(OnPeriodicDeadOrAlive, void(const int channel,
                                           const bool alive));
};

}

#endif  

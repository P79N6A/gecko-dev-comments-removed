









#ifndef WEBRTC_TEST_MOCK_TRANSPORT_H_
#define WEBRTC_TEST_MOCK_TRANSPORT_H_

#include "testing/gmock/include/gmock/gmock.h"
#include "webrtc/transport.h"

namespace webrtc {

class MockTransport : public webrtc::Transport {
 public:
  MOCK_METHOD3(SendPacket,
      int(int channel, const void* data, int len));
  MOCK_METHOD3(SendRTCPPacket,
      int(int channel, const void* data, int len));
};
}  
#endif  

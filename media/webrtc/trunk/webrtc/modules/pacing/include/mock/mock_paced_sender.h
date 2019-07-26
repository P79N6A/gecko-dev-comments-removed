









#ifndef WEBRTC_MODULES_PACING_INCLUDE_MOCK_MOCK_PACED_SENDER_H_
#define WEBRTC_MODULES_PACING_INCLUDE_MOCK_MOCK_PACED_SENDER_H_

#include "testing/gmock/include/gmock/gmock.h"

#include <vector>

#include "webrtc/modules/pacing/include/paced_sender.h"

namespace webrtc {

class MockPacedSender : public PacedSender {
 public:
  MockPacedSender() : PacedSender(NULL, 0, 0) {}
  MOCK_METHOD6(SendPacket, bool(Priority priority,
                                uint32_t ssrc,
                                uint16_t sequence_number,
                                int64_t capture_time_ms,
                                int bytes,
                                bool retransmission));
  MOCK_CONST_METHOD0(QueueInMs, int());
  MOCK_CONST_METHOD0(QueueInPackets, int());
};

}  

#endif  

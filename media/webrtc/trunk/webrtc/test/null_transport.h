








#ifndef WEBRTC_VIDEO_ENGINE_TEST_COMMON_NULL_TRANSPORT_H_
#define WEBRTC_VIDEO_ENGINE_TEST_COMMON_NULL_TRANSPORT_H_

#include "webrtc/transport.h"

namespace webrtc {

class PacketReceiver;

namespace test {
class NullTransport : public newapi::Transport {
 public:
  virtual bool SendRtp(const uint8_t* packet, size_t length) OVERRIDE;
  virtual bool SendRtcp(const uint8_t* packet, size_t length) OVERRIDE;
};
}  
}  

#endif  

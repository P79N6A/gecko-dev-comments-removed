








#ifndef WEBRTC_VIDEO_ENGINE_TEST_COMMON_NULL_TRANSPORT_H_
#define WEBRTC_VIDEO_ENGINE_TEST_COMMON_NULL_TRANSPORT_H_

#include "webrtc/video_engine/new_include/transport.h"

namespace webrtc {

class PacketReceiver;

namespace test {
class NullTransport : public newapi::Transport {
 public:
  virtual bool SendRTP(const uint8_t* packet, size_t length) OVERRIDE;
  virtual bool SendRTCP(const uint8_t* packet, size_t length) OVERRIDE;
};
}  
}  

#endif  

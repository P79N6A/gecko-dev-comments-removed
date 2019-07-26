








#include "webrtc/video_engine/test/common/null_transport.h"

namespace webrtc {
namespace test {

bool NullTransport::SendRTP(const uint8_t* packet, size_t length) {
  return true;
}

bool NullTransport::SendRTCP(const uint8_t* packet, size_t length) {
  return true;
}

}  
}  

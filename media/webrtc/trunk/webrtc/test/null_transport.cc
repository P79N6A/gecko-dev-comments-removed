








#include "webrtc/test/null_transport.h"

namespace webrtc {
namespace test {

bool NullTransport::SendRtp(const uint8_t* packet, size_t length) {
  return true;
}

bool NullTransport::SendRtcp(const uint8_t* packet, size_t length) {
  return true;
}

}  
}  

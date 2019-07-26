









#include "webrtc/video/transport_adapter.h"

namespace webrtc {
namespace internal {

TransportAdapter::TransportAdapter(newapi::Transport* transport)
    : transport_(transport), enabled_(0) {}

int TransportAdapter::SendPacket(int ,
                                 const void* packet,
                                 int length) {
  if (enabled_.Value() == 0)
    return false;

  bool success = transport_->SendRtp(static_cast<const uint8_t*>(packet),
                                     static_cast<size_t>(length));
  return success ? length : -1;
}

int TransportAdapter::SendRTCPPacket(int ,
                                     const void* packet,
                                     int length) {
  if (enabled_.Value() == 0)
    return false;

  bool success = transport_->SendRtcp(static_cast<const uint8_t*>(packet),
                                      static_cast<size_t>(length));
  return success ? length : -1;
}

void TransportAdapter::Enable() {
  
  
  enabled_.CompareExchange(1, 0);
}

void TransportAdapter::Disable() { enabled_.CompareExchange(0, 1); }

}  
}  

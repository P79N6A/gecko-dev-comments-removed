









#include "webrtc/video_engine/internal/transport_adapter.h"

namespace webrtc {
namespace internal {

TransportAdapter::TransportAdapter(newapi::Transport* transport)
    : transport_(transport) {}

int TransportAdapter::SendPacket(int ,
                                 const void* packet,
                                 int length) {
  bool success = transport_->SendRTP(static_cast<const uint8_t*>(packet),
                                     static_cast<size_t>(length));
  return success ? length : -1;
}

int TransportAdapter::SendRTCPPacket(int ,
                                     const void* packet,
                                     int length) {
  bool success = transport_->SendRTCP(static_cast<const uint8_t*>(packet),
                                      static_cast<size_t>(length));
  return success ? length : -1;
}

}  
}  










#ifndef WEBRTC_VIDEO_ENGINE_INTERNAL_TRANSPORT_ADAPTER_H_
#define WEBRTC_VIDEO_ENGINE_INTERNAL_TRANSPORT_ADAPTER_H_

#include "webrtc/common_types.h"
#include "webrtc/system_wrappers/interface/atomic32.h"
#include "webrtc/transport.h"

namespace webrtc {
namespace internal {

class TransportAdapter : public webrtc::Transport {
 public:
  explicit TransportAdapter(newapi::Transport* transport);

  virtual int SendPacket(int , const void* packet, int length)
      OVERRIDE;
  virtual int SendRTCPPacket(int , const void* packet, int length)
      OVERRIDE;

  void Enable();
  void Disable();

 private:
  newapi::Transport *transport_;
  Atomic32 enabled_;
};
}  
}  

#endif  

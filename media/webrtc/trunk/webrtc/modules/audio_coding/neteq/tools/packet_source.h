









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ_TOOLS_PACKET_SOURCE_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ_TOOLS_PACKET_SOURCE_H_

#include <bitset>

#include "webrtc/base/constructormagic.h"
#include "webrtc/typedefs.h"

namespace webrtc {
namespace test {

class Packet;


class PacketSource {
 public:
  PacketSource() : use_ssrc_filter_(false), ssrc_(0) {}
  virtual ~PacketSource() {}

  
  
  virtual Packet* NextPacket() = 0;

  virtual void FilterOutPayloadType(uint8_t payload_type) {
    filter_.set(payload_type, true);
  }

  virtual void SelectSsrc(uint32_t ssrc) {
    use_ssrc_filter_ = true;
    ssrc_ = ssrc;
  }

 protected:
  std::bitset<128> filter_;  
  
  bool use_ssrc_filter_;  
  uint32_t ssrc_;  

 private:
  DISALLOW_COPY_AND_ASSIGN(PacketSource);
};

}  
}  
#endif  

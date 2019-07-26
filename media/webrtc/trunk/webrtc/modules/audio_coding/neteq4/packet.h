









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ4_PACKET_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ4_PACKET_H_

#include <list>

#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/typedefs.h"

namespace webrtc {


struct Packet {
  RTPHeader header;
  uint8_t* payload;  
  int payload_length;
  bool primary;  
  int waiting_time;

  
  Packet()
      : payload(NULL),
        payload_length(0),
        primary(true),
        waiting_time(0) {
  }

  
  
  
  
  
  bool operator==(const Packet& rhs) const {
    return (this->header.timestamp == rhs.header.timestamp &&
        this->header.sequenceNumber == rhs.header.sequenceNumber &&
        this->primary == rhs.primary);
  }
  bool operator!=(const Packet& rhs) const { return !operator==(rhs); }
  bool operator<(const Packet& rhs) const {
    if (this->header.timestamp == rhs.header.timestamp) {
      if (this->header.sequenceNumber == rhs.header.sequenceNumber) {
        
        
        
        return (this->primary && !rhs.primary);
      }
      return (static_cast<uint16_t>(rhs.header.sequenceNumber
          - this->header.sequenceNumber) < 0xFFFF / 2);
    }
    return (static_cast<uint32_t>(rhs.header.timestamp
        - this->header.timestamp) < 0xFFFFFFFF / 2);
  }
  bool operator>(const Packet& rhs) const { return rhs.operator<(*this); }
  bool operator<=(const Packet& rhs) const { return !operator>(rhs); }
  bool operator>=(const Packet& rhs) const { return !operator<(rhs); }
};


typedef std::list<Packet*> PacketList;

}  
#endif  











#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ_TOOLS_PACKET_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ_TOOLS_PACKET_H_

#include <list>

#include "webrtc/base/constructormagic.h"
#include "webrtc/common_types.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class RtpHeaderParser;
struct WebRtcRTPHeader;

namespace test {


class Packet {
 public:
  
  
  
  
  
  
  Packet(uint8_t* packet_memory,
         size_t allocated_bytes,
         double time_ms,
         const RtpHeaderParser& parser);

  
  
  
  
  
  
  Packet(uint8_t* packet_memory,
         size_t allocated_bytes,
         size_t virtual_packet_length_bytes,
         double time_ms,
         const RtpHeaderParser& parser);

  
  
  
  
  Packet(uint8_t* packet_memory, size_t allocated_bytes, double time_ms);

  Packet(uint8_t* packet_memory,
         size_t allocated_bytes,
         size_t virtual_packet_length_bytes,
         double time_ms);

  virtual ~Packet() {}

  
  
  
  
  bool ExtractRedHeaders(std::list<RTPHeader*>* headers) const;

  
  
  static void DeleteRedHeaders(std::list<RTPHeader*>* headers);

  const uint8_t* payload() const { return payload_; }

  size_t packet_length_bytes() const { return packet_length_bytes_; }

  size_t payload_length_bytes() const { return payload_length_bytes_; }

  size_t virtual_packet_length_bytes() const {
    return virtual_packet_length_bytes_;
  }

  size_t virtual_payload_length_bytes() const {
    return virtual_payload_length_bytes_;
  }

  const RTPHeader& header() const { return header_; }

  
  
  void ConvertHeader(WebRtcRTPHeader* copy_to) const;

  void set_time_ms(double time) { time_ms_ = time; }
  double time_ms() const { return time_ms_; }
  bool valid_header() const { return valid_header_; }

 private:
  bool ParseHeader(const RtpHeaderParser& parser);
  void CopyToHeader(RTPHeader* destination) const;

  RTPHeader header_;
  scoped_ptr<uint8_t[]> payload_memory_;
  const uint8_t* payload_;            
  const size_t packet_length_bytes_;  
  size_t payload_length_bytes_;  
                                 
  
  const size_t virtual_packet_length_bytes_;
  size_t virtual_payload_length_bytes_;
  double time_ms_;     
  bool valid_header_;  

  DISALLOW_COPY_AND_ASSIGN(Packet);
};

}  
}  
#endif  











#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ4_RTCP_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ4_RTCP_H_

#include "webrtc/modules/audio_coding/neteq4/interface/neteq.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/typedefs.h"

namespace webrtc {


struct RTPHeader;

class Rtcp {
 public:
  Rtcp() {
    Init(0);
  }

  ~Rtcp() {}

  
  void Init(uint16_t start_sequence_number);

  
  void Update(const RTPHeader& rtp_header, uint32_t receive_timestamp);

  
  
  void GetStatistics(bool no_reset, RtcpStatistics* stats);

 private:
  uint16_t cycles_;  
  uint16_t max_seq_no_;  
                         
  uint16_t base_seq_no_;  
  uint32_t received_packets_;  
  uint32_t received_packets_prior_;  
                                     
  uint32_t expected_prior_;  
                             
  uint32_t jitter_;  
  int32_t transit_;  

  DISALLOW_COPY_AND_ASSIGN(Rtcp);
};

}  
#endif  

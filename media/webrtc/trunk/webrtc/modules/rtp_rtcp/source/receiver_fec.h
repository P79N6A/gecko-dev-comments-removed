









#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_RECEIVER_FEC_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_RECEIVER_FEC_H_



#include "webrtc/modules/rtp_rtcp/interface/rtp_rtcp_defines.h"
#include "webrtc/modules/rtp_rtcp/source/forward_error_correction.h"
#include "webrtc/typedefs.h"

namespace webrtc {
class RTPReceiverVideo;

class ReceiverFEC {
 public:
  ReceiverFEC(const int32_t id, RTPReceiverVideo* owner);
  virtual ~ReceiverFEC();

  int32_t AddReceivedFECPacket(const WebRtcRTPHeader* rtp_header,
                               const uint8_t* incoming_rtp_packet,
                               const uint16_t payload_data_length,
                               bool& FECpacket);

  int32_t ProcessReceivedFEC();

  void SetPayloadTypeFEC(const int8_t payload_type);

 private:
  int ParseAndReceivePacket(const ForwardErrorCorrection::Packet* packet);

  int id_;
  RTPReceiverVideo* owner_;
  ForwardErrorCorrection* fec_;
  
  
  
  ForwardErrorCorrection::ReceivedPacketList received_packet_list_;
  ForwardErrorCorrection::RecoveredPacketList recovered_packet_list_;
  int8_t payload_type_fec_;
};
}  

#endif  

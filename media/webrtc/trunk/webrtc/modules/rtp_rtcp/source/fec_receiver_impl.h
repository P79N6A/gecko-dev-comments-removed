









#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_FEC_RECEIVER_IMPL_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_FEC_RECEIVER_IMPL_H_



#include "webrtc/modules/rtp_rtcp/interface/fec_receiver.h"
#include "webrtc/modules/rtp_rtcp/interface/rtp_rtcp_defines.h"
#include "webrtc/modules/rtp_rtcp/source/forward_error_correction.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class CriticalSectionWrapper;

class FecReceiverImpl : public FecReceiver {
 public:
  FecReceiverImpl(const int32_t id, RtpData* callback);
  virtual ~FecReceiverImpl();

  virtual int32_t AddReceivedRedPacket(const RTPHeader& rtp_header,
                                       const uint8_t* incoming_rtp_packet,
                                       int packet_length,
                                       uint8_t ulpfec_payload_type) OVERRIDE;

  virtual int32_t ProcessReceivedFec() OVERRIDE;

 private:
  int id_;
  scoped_ptr<CriticalSectionWrapper> crit_sect_;
  RtpData* recovered_packet_callback_;
  ForwardErrorCorrection* fec_;
  
  
  
  ForwardErrorCorrection::ReceivedPacketList received_packet_list_;
  ForwardErrorCorrection::RecoveredPacketList recovered_packet_list_;
};
}  

#endif  

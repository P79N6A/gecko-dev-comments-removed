









#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_FEC_TEST_HELPER_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_FEC_TEST_HELPER_H_

#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/modules/rtp_rtcp/source/forward_error_correction.h"

namespace webrtc {

enum {
  kFecPayloadType = 96
};
enum {
  kRedPayloadType = 97
};
enum {
  kVp8PayloadType = 120
};

typedef ForwardErrorCorrection::Packet Packet;

struct RtpPacket : public Packet {
  WebRtcRTPHeader header;
};

class FrameGenerator {
 public:
  FrameGenerator();

  void NewFrame(int num_packets);

  uint16_t NextSeqNum();

  RtpPacket* NextPacket(int offset, size_t length);

  
  RtpPacket* BuildMediaRedPacket(const RtpPacket* packet);

  
  
  
  RtpPacket* BuildFecRedPacket(const Packet* packet);

  void SetRedHeader(Packet* red_packet, uint8_t payload_type,
                    int header_length) const;

 private:
  static void BuildRtpHeader(uint8_t* data, const RTPHeader* header);

  int num_packets_;
  uint16_t seq_num_;
  uint32_t timestamp_;
};
}

#endif  

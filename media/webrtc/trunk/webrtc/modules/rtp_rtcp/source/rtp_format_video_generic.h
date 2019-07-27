








#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_FORMAT_VIDEO_GENERIC_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_FORMAT_VIDEO_GENERIC_H_

#include <string>

#include "webrtc/common_types.h"
#include "webrtc/modules/rtp_rtcp/source/rtp_format.h"
#include "webrtc/typedefs.h"

namespace webrtc {
namespace RtpFormatVideoGeneric {
static const uint8_t kKeyFrameBit = 0x01;
static const uint8_t kFirstPacketBit = 0x02;
}  

class RtpPacketizerGeneric : public RtpPacketizer {
 public:
  
  
  RtpPacketizerGeneric(FrameType frametype, size_t max_payload_len);

  virtual ~RtpPacketizerGeneric();

  virtual void SetPayloadData(
      const uint8_t* payload_data,
      size_t payload_size,
      const RTPFragmentationHeader* fragmentation) OVERRIDE;

  
  
  
  
  
  
  
  virtual bool NextPacket(uint8_t* buffer,
                          size_t* bytes_to_send,
                          bool* last_packet) OVERRIDE;

  virtual ProtectionType GetProtectionType() OVERRIDE;

  virtual StorageType GetStorageType(uint32_t retransmission_settings) OVERRIDE;

  virtual std::string ToString() OVERRIDE;

 private:
  const uint8_t* payload_data_;
  size_t payload_size_;
  const size_t max_payload_len_;
  FrameType frame_type_;
  uint32_t payload_length_;
  uint8_t generic_header_;

  DISALLOW_COPY_AND_ASSIGN(RtpPacketizerGeneric);
};


class RtpDepacketizerGeneric : public RtpDepacketizer {
 public:
  virtual ~RtpDepacketizerGeneric() {}

  virtual bool Parse(ParsedPayload* parsed_payload,
                     const uint8_t* payload_data,
                     size_t payload_data_length) OVERRIDE;
};
}  
#endif  

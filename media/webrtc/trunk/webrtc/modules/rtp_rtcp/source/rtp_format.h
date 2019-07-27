









#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_FORMAT_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_FORMAT_H_

#include <string>

#include "webrtc/base/constructormagic.h"
#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/modules/rtp_rtcp/interface/rtp_rtcp_defines.h"

namespace webrtc {

class RtpPacketizer {
 public:
  static RtpPacketizer* Create(RtpVideoCodecTypes type,
                               size_t max_payload_len,
                               const RTPVideoTypeHeader* rtp_type_header,
                               FrameType frame_type);

  virtual ~RtpPacketizer() {}

  virtual void SetPayloadData(const uint8_t* payload_data,
                              size_t payload_size,
                              const RTPFragmentationHeader* fragmentation) = 0;

  
  
  
  
  
  
  
  virtual bool NextPacket(uint8_t* buffer,
                          size_t* bytes_to_send,
                          bool* last_packet) = 0;

  virtual ProtectionType GetProtectionType() = 0;

  virtual StorageType GetStorageType(uint32_t retransmission_settings) = 0;

  virtual std::string ToString() = 0;
};

class RtpDepacketizer {
 public:
  struct ParsedPayload {
    const uint8_t* payload;
    size_t payload_length;
    FrameType frame_type;
    RTPTypeHeader type;
  };

  static RtpDepacketizer* Create(RtpVideoCodecTypes type);

  virtual ~RtpDepacketizer() {}

  
  virtual bool Parse(ParsedPayload* parsed_payload,
                     const uint8_t* payload_data,
                     size_t payload_data_length) = 0;
};
}  
#endif  

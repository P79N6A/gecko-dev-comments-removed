








#ifndef WEBRTC_MODULES_RTP_RTCP_INTERFACE_RTP_HEADER_PARSER_H_
#define WEBRTC_MODULES_RTP_RTCP_INTERFACE_RTP_HEADER_PARSER_H_

#include "webrtc/modules/rtp_rtcp/interface/rtp_rtcp_defines.h"
#include "webrtc/typedefs.h"

namespace webrtc {

struct RTPHeader;

class RtpHeaderParser {
 public:
  static RtpHeaderParser* Create();
  virtual ~RtpHeaderParser() {}

  
  static bool IsRtcp(const uint8_t* packet, int length);

  
  
  
  
  virtual bool Parse(const uint8_t* packet, int length,
                     RTPHeader* header) const = 0;

  
  virtual bool RegisterRtpHeaderExtension(RTPExtensionType type,
                                          uint8_t id) = 0;

  
  virtual bool DeregisterRtpHeaderExtension(RTPExtensionType type) = 0;
};
}  
#endif  

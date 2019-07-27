









#include "webrtc/modules/rtp_rtcp/source/rtp_format.h"

#include "webrtc/modules/rtp_rtcp/source/rtp_format_h264.h"
#include "webrtc/modules/rtp_rtcp/source/rtp_format_video_generic.h"
#include "webrtc/modules/rtp_rtcp/source/rtp_format_vp8.h"

namespace webrtc {
RtpPacketizer* RtpPacketizer::Create(RtpVideoCodecTypes type,
                                     size_t max_payload_len,
                                     const RTPVideoTypeHeader* rtp_type_header,
                                     FrameType frame_type) {
  switch (type) {
    case kRtpVideoH264:
      return new RtpPacketizerH264(frame_type, max_payload_len);
    case kRtpVideoVp8:
      assert(rtp_type_header != NULL);
      return new RtpPacketizerVp8(rtp_type_header->VP8, max_payload_len);
    case kRtpVideoVp9:
    case kRtpVideoGeneric:
      return new RtpPacketizerGeneric(frame_type, max_payload_len);
    case kRtpVideoNone:
      assert(false);
  }
  return NULL;
}

RtpDepacketizer* RtpDepacketizer::Create(RtpVideoCodecTypes type) {
  switch (type) {
    case kRtpVideoH264:
      return new RtpDepacketizerH264();
    case kRtpVideoVp8:
      return new RtpDepacketizerVp8();
    case kRtpVideoVp9: 
    case kRtpVideoGeneric:
      return new RtpDepacketizerGeneric();
    case kRtpVideoNone:
      assert(false);
  }
  return NULL;
}
}  

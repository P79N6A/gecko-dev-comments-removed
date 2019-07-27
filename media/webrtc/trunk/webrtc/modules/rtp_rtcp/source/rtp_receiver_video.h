









#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_RECEIVER_VIDEO_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_RECEIVER_VIDEO_H_

#include "webrtc/modules/rtp_rtcp/interface/rtp_rtcp_defines.h"
#include "webrtc/modules/rtp_rtcp/source/bitrate.h"
#include "webrtc/modules/rtp_rtcp/source/rtp_receiver_strategy.h"
#include "webrtc/modules/rtp_rtcp/source/rtp_utility.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class RTPReceiverVideo : public RTPReceiverStrategy {
 public:
  explicit RTPReceiverVideo(RtpData* data_callback);

  virtual ~RTPReceiverVideo();

  virtual int32_t ParseRtpPacket(WebRtcRTPHeader* rtp_header,
                                 const PayloadUnion& specific_payload,
                                 bool is_red,
                                 const uint8_t* packet,
                                 uint16_t packet_length,
                                 int64_t timestamp,
                                 bool is_first_packet) OVERRIDE;

  TelephoneEventHandler* GetTelephoneEventHandler() { return NULL; }

  int GetPayloadTypeFrequency() const OVERRIDE;

  virtual RTPAliveType ProcessDeadOrAlive(
      uint16_t last_payload_length) const OVERRIDE;

  virtual bool ShouldReportCsrcChanges(uint8_t payload_type) const OVERRIDE;

  virtual int32_t OnNewPayloadTypeCreated(
      const char payload_name[RTP_PAYLOAD_NAME_SIZE],
      int8_t payload_type,
      uint32_t frequency) OVERRIDE;

  virtual int32_t InvokeOnInitializeDecoder(
      RtpFeedback* callback,
      int32_t id,
      int8_t payload_type,
      const char payload_name[RTP_PAYLOAD_NAME_SIZE],
      const PayloadUnion& specific_payload) const OVERRIDE;

  void SetPacketOverHead(uint16_t packet_over_head);

 private:
  int32_t BuildRTPheader(const WebRtcRTPHeader* rtp_header,
                         uint8_t* data_buffer) const;
};
}  

#endif  

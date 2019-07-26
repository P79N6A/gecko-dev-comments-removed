









#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_RECEIVER_STRATEGY_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_RECEIVER_STRATEGY_H_

#include "webrtc/modules/rtp_rtcp/interface/rtp_rtcp.h"
#include "webrtc/modules/rtp_rtcp/interface/rtp_rtcp_defines.h"
#include "webrtc/modules/rtp_rtcp/source/rtp_utility.h"
#include "webrtc/typedefs.h"

namespace webrtc {



class RTPReceiverStrategy {
 public:
  
  
  
  
  
  
  
  
  RTPReceiverStrategy(RtpData* data_callback);
  virtual ~RTPReceiverStrategy() {}

  
  
  
  
  
  
  virtual int32_t ParseRtpPacket(
    WebRtcRTPHeader* rtp_header,
    const ModuleRTPUtility::PayloadUnion& specific_payload,
    const bool is_red,
    const uint8_t* packet,
    const uint16_t packet_length,
    const int64_t timestamp_ms,
    const bool is_first_packet) = 0;

  
  virtual int32_t GetFrequencyHz() const = 0;

  
  virtual RTPAliveType ProcessDeadOrAlive(
    uint16_t last_payload_length) const = 0;

  
  
  virtual bool ShouldReportCsrcChanges(uint8_t payload_type) const = 0;

  
  
  virtual int32_t OnNewPayloadTypeCreated(
      const char payloadName[RTP_PAYLOAD_NAME_SIZE],
      const int8_t payloadType,
      const uint32_t frequency) = 0;

  
  virtual int32_t InvokeOnInitializeDecoder(
    RtpFeedback* callback,
    const int32_t id,
    const int8_t payload_type,
    const char payload_name[RTP_PAYLOAD_NAME_SIZE],
    const ModuleRTPUtility::PayloadUnion& specific_payload) const = 0;

  
  
  virtual void CheckPayloadChanged(
    const int8_t payload_type,
    ModuleRTPUtility::PayloadUnion* specific_payload,
    bool* should_reset_statistics,
    bool* should_discard_changes) {
    
    *should_discard_changes = false;
    *should_reset_statistics = false;
  }

  
  void GetLastMediaSpecificPayload(
    ModuleRTPUtility::PayloadUnion* payload) const;
  void SetLastMediaSpecificPayload(
    const ModuleRTPUtility::PayloadUnion& payload);

 protected:
  ModuleRTPUtility::PayloadUnion last_payload_;
  RtpData* data_callback_;
};

}  

#endif  











#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_RECEIVER_STRATEGY_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_RECEIVER_STRATEGY_H_

#include "webrtc/modules/rtp_rtcp/interface/rtp_rtcp.h"
#include "webrtc/modules/rtp_rtcp/interface/rtp_rtcp_defines.h"
#include "webrtc/modules/rtp_rtcp/source/rtp_utility.h"
#include "webrtc/typedefs.h"

namespace webrtc {



class RTPReceiverStrategy {
 public:
  RTPReceiverStrategy();
  virtual ~RTPReceiverStrategy() {}

  
  
  
  
  
  
  virtual WebRtc_Word32 ParseRtpPacket(
    WebRtcRTPHeader* rtp_header,
    const ModuleRTPUtility::PayloadUnion& specific_payload,
    const bool is_red,
    const WebRtc_UWord8* packet,
    const WebRtc_UWord16 packet_length,
    const WebRtc_Word64 timestamp_ms) = 0;

  
  virtual WebRtc_Word32 GetFrequencyHz() const = 0;

  
  virtual RTPAliveType ProcessDeadOrAlive(
    WebRtc_UWord16 last_payload_length) const = 0;

  
  
  virtual bool PayloadIsCompatible(
    const ModuleRTPUtility::Payload& payload,
    const WebRtc_UWord32 frequency,
    const WebRtc_UWord8 channels,
    const WebRtc_UWord32 rate) const = 0;

  
  virtual void UpdatePayloadRate(
    ModuleRTPUtility::Payload* payload,
    const WebRtc_UWord32 rate) const = 0;

  
  virtual ModuleRTPUtility::Payload* CreatePayloadType(
    const char payload_name[RTP_PAYLOAD_NAME_SIZE],
    const WebRtc_Word8 payload_type,
    const WebRtc_UWord32 frequency,
    const WebRtc_UWord8 channels,
    const WebRtc_UWord32 rate) = 0;

  
  virtual WebRtc_Word32 InvokeOnInitializeDecoder(
    RtpFeedback* callback,
    const WebRtc_Word32 id,
    const WebRtc_Word8 payload_type,
    const char payload_name[RTP_PAYLOAD_NAME_SIZE],
    const ModuleRTPUtility::PayloadUnion& specific_payload) const = 0;

  
  
  
  virtual void PossiblyRemoveExistingPayloadType(
    ModuleRTPUtility::PayloadTypeMap* payload_type_map,
    const char payload_name[RTP_PAYLOAD_NAME_SIZE],
    const size_t payload_name_length,
    const WebRtc_UWord32 frequency,
    const WebRtc_UWord8 channels,
    const WebRtc_UWord32 rate) const {
    
  }

  
  
  virtual void CheckPayloadChanged(
    const WebRtc_Word8 payload_type,
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
};

}  

#endif  

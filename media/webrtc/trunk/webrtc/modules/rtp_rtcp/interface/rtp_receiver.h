









#ifndef WEBRTC_MODULES_RTP_RTCP_INTERFACE_RTP_RECEIVER_H_
#define WEBRTC_MODULES_RTP_RTCP_INTERFACE_RTP_RECEIVER_H_

#include "webrtc/modules/rtp_rtcp/interface/rtp_rtcp_defines.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class RTPPayloadRegistry;

class TelephoneEventHandler {
 public:
  virtual ~TelephoneEventHandler() {}

  
  
  virtual void SetTelephoneEventForwardToDecoder(bool forward_to_decoder) = 0;

  
  virtual bool TelephoneEventForwardToDecoder() const = 0;

  
  virtual bool TelephoneEventPayloadType(const int8_t payload_type) const = 0;
};

class RtpReceiver {
 public:
  
  static RtpReceiver* CreateVideoReceiver(
      int id, Clock* clock,
      RtpData* incoming_payload_callback,
      RtpFeedback* incoming_messages_callback,
      RTPPayloadRegistry* rtp_payload_registry);

  
  static RtpReceiver* CreateAudioReceiver(
      int id, Clock* clock,
      RtpAudioFeedback* incoming_audio_feedback,
      RtpData* incoming_payload_callback,
      RtpFeedback* incoming_messages_callback,
      RTPPayloadRegistry* rtp_payload_registry);

  virtual ~RtpReceiver() {}

  
  virtual TelephoneEventHandler* GetTelephoneEventHandler() = 0;

  
  
  virtual int32_t RegisterReceivePayload(
      const char payload_name[RTP_PAYLOAD_NAME_SIZE],
      const int8_t payload_type,
      const uint32_t frequency,
      const uint8_t channels,
      const uint32_t rate) = 0;

  
  virtual int32_t DeRegisterReceivePayload(const int8_t payload_type) = 0;

  
  
  
  virtual bool IncomingRtpPacket(const RTPHeader& rtp_header,
                                 const uint8_t* payload,
                                 int payload_length,
                                 PayloadUnion payload_specific,
                                 bool in_order) = 0;

  
  virtual NACKMethod NACK() const = 0;

  
  virtual void SetNACKStatus(const NACKMethod method) = 0;

  
  virtual uint32_t Timestamp() const = 0;
  
  virtual int32_t LastReceivedTimeMs() const = 0;

  
  virtual uint32_t SSRC() const = 0;

  
  virtual int32_t CSRCs(uint32_t array_of_csrc[kRtpCsrcSize]) const = 0;

  
  virtual int32_t Energy(uint8_t array_of_energy[kRtpCsrcSize]) const = 0;
};
}  

#endif  

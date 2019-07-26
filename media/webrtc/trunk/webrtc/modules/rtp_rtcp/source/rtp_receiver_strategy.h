









#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_RECEIVER_STRATEGY_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_RECEIVER_STRATEGY_H_

#include "webrtc/modules/rtp_rtcp/interface/rtp_rtcp.h"
#include "webrtc/modules/rtp_rtcp/interface/rtp_rtcp_defines.h"
#include "webrtc/modules/rtp_rtcp/source/rtp_utility.h"
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class TelephoneEventHandler;



class RTPReceiverStrategy {
 public:
  static RTPReceiverStrategy* CreateVideoStrategy(int32_t id,
                                                  RtpData* data_callback);
  static RTPReceiverStrategy* CreateAudioStrategy(
      int32_t id, RtpData* data_callback,
      RtpAudioFeedback* incoming_messages_callback);

  virtual ~RTPReceiverStrategy() {}

  
  
  
  
  
  
  virtual int32_t ParseRtpPacket(WebRtcRTPHeader* rtp_header,
                                 const PayloadUnion& specific_payload,
                                 bool is_red,
                                 const uint8_t* payload,
                                 uint16_t payload_length,
                                 int64_t timestamp_ms,
                                 bool is_first_packet) = 0;

  virtual TelephoneEventHandler* GetTelephoneEventHandler() = 0;

  
  virtual int GetPayloadTypeFrequency() const = 0;

  
  virtual RTPAliveType ProcessDeadOrAlive(
      uint16_t last_payload_length) const = 0;

  
  
  virtual bool ShouldReportCsrcChanges(uint8_t payload_type) const = 0;

  
  
  virtual int32_t OnNewPayloadTypeCreated(
      const char payloadName[RTP_PAYLOAD_NAME_SIZE],
      int8_t payloadType,
      uint32_t frequency) = 0;

  
  virtual int32_t InvokeOnInitializeDecoder(
      RtpFeedback* callback,
      int32_t id,
      int8_t payload_type,
      const char payload_name[RTP_PAYLOAD_NAME_SIZE],
      const PayloadUnion& specific_payload) const = 0;

  
  
  virtual void CheckPayloadChanged(int8_t payload_type,
                                   PayloadUnion* specific_payload,
                                   bool* should_reset_statistics,
                                   bool* should_discard_changes);

  virtual int Energy(uint8_t array_of_energy[kRtpCsrcSize]) const;

  
  void GetLastMediaSpecificPayload(PayloadUnion* payload) const;
  void SetLastMediaSpecificPayload(const PayloadUnion& payload);

 protected:
  
  
  
  
  
  
  
  
  RTPReceiverStrategy(RtpData* data_callback);

  scoped_ptr<CriticalSectionWrapper> crit_sect_;
  PayloadUnion last_payload_;
  RtpData* data_callback_;
};
}  

#endif  

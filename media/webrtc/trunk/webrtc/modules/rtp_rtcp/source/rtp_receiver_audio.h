









#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_RECEIVER_AUDIO_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_RECEIVER_AUDIO_H_

#include <set>

#include "webrtc/modules/rtp_rtcp/interface/rtp_receiver.h"
#include "webrtc/modules/rtp_rtcp/interface/rtp_rtcp_defines.h"
#include "webrtc/modules/rtp_rtcp/source/rtp_receiver_strategy.h"
#include "webrtc/modules/rtp_rtcp/source/rtp_utility.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class CriticalSectionWrapper;


class RTPReceiverAudio : public RTPReceiverStrategy,
                         public TelephoneEventHandler {
 public:
  RTPReceiverAudio(const int32_t id,
                   RtpData* data_callback,
                   RtpAudioFeedback* incoming_messages_callback);
  virtual ~RTPReceiverAudio() {}

  
  
  void SetTelephoneEventForwardToDecoder(bool forward_to_decoder);

  
  bool TelephoneEventForwardToDecoder() const;

  
  bool TelephoneEventPayloadType(const int8_t payload_type) const;

  TelephoneEventHandler* GetTelephoneEventHandler() {
    return this;
  }

  
  
  bool CNGPayloadType(const int8_t payload_type,
                      uint32_t* frequency,
                      bool* cng_payload_type_has_changed);

  int32_t ParseRtpPacket(WebRtcRTPHeader* rtp_header,
                         const PayloadUnion& specific_payload,
                         bool is_red,
                         const uint8_t* packet,
                         uint16_t packet_length,
                         int64_t timestamp_ms,
                         bool is_first_packet);

  int GetPayloadTypeFrequency() const OVERRIDE;

  virtual RTPAliveType ProcessDeadOrAlive(uint16_t last_payload_length) const
      OVERRIDE;

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

  
  
  void PossiblyRemoveExistingPayloadType(
      RtpUtility::PayloadTypeMap* payload_type_map,
      const char payload_name[RTP_PAYLOAD_NAME_SIZE],
      size_t payload_name_length,
      uint32_t frequency,
      uint8_t channels,
      uint32_t rate) const;

  
  
  void CheckPayloadChanged(int8_t payload_type,
                           PayloadUnion* specific_payload,
                           bool* should_reset_statistics,
                           bool* should_discard_changes) OVERRIDE;

  int Energy(uint8_t array_of_energy[kRtpCsrcSize]) const OVERRIDE;

 private:

  int32_t ParseAudioCodecSpecific(
      WebRtcRTPHeader* rtp_header,
      const uint8_t* payload_data,
      uint16_t payload_length,
      const AudioPayload& audio_specific,
      bool is_red);

  int32_t id_;

  uint32_t last_received_frequency_;

  bool telephone_event_forward_to_decoder_;
  int8_t telephone_event_payload_type_;
  std::set<uint8_t> telephone_event_reported_;

  int8_t cng_nb_payload_type_;
  int8_t cng_wb_payload_type_;
  int8_t cng_swb_payload_type_;
  int8_t cng_fb_payload_type_;
  int8_t cng_payload_type_;

  
  
  int8_t g722_payload_type_;
  bool last_received_g722_;

  uint8_t num_energy_;
  uint8_t current_remote_energy_[kRtpCsrcSize];

  RtpAudioFeedback* cb_audio_feedback_;
};
}  

#endif  











#include "webrtc/modules/rtp_rtcp/source/rtp_receiver_strategy.h"

#include <stdlib.h>

#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"

namespace webrtc {

RTPReceiverStrategy::RTPReceiverStrategy(RtpData* data_callback)
    : crit_sect_(CriticalSectionWrapper::CreateCriticalSection()),
      data_callback_(data_callback) {
  memset(&last_payload_, 0, sizeof(last_payload_));
}

void RTPReceiverStrategy::GetLastMediaSpecificPayload(
    PayloadUnion* payload) const {
  CriticalSectionScoped cs(crit_sect_.get());
  memcpy(payload, &last_payload_, sizeof(*payload));
}

void RTPReceiverStrategy::SetLastMediaSpecificPayload(
    const PayloadUnion& payload) {
  CriticalSectionScoped cs(crit_sect_.get());
  memcpy(&last_payload_, &payload, sizeof(last_payload_));
}

void RTPReceiverStrategy::CheckPayloadChanged(int8_t payload_type,
                                              PayloadUnion* specific_payload,
                                              bool* should_reset_statistics,
                                              bool* should_discard_changes) {
  
  *should_discard_changes = false;
  *should_reset_statistics = false;
}

int RTPReceiverStrategy::Energy(uint8_t array_of_energy[kRtpCsrcSize]) const {
  return -1;
}

}  











#include "webrtc/modules/rtp_rtcp/source/rtp_receiver_strategy.h"

#include <cstdlib>

namespace webrtc {

RTPReceiverStrategy::RTPReceiverStrategy() {
  memset(&last_payload_, 0, sizeof(last_payload_));
}

void RTPReceiverStrategy::GetLastMediaSpecificPayload(
  ModuleRTPUtility::PayloadUnion* payload) const {
  memcpy(payload, &last_payload_, sizeof(*payload));
}

void RTPReceiverStrategy::SetLastMediaSpecificPayload(
  const ModuleRTPUtility::PayloadUnion& payload) {
  memcpy(&last_payload_, &payload, sizeof(last_payload_));
}

}  











#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_MOCK_MOCK_RTP_PAYLOAD_REGISTRY_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_MOCK_MOCK_RTP_PAYLOAD_REGISTRY_H_

#include "testing/gmock/include/gmock/gmock.h"
#include "webrtc/modules/rtp_rtcp/interface/rtp_payload_registry.h"

namespace webrtc {

class MockRTPPayloadStrategy : public RTPPayloadStrategy {
 public:
  MOCK_CONST_METHOD0(CodecsMustBeUnique,
      bool());
  MOCK_CONST_METHOD4(PayloadIsCompatible,
      bool(const ModuleRTPUtility::Payload& payload,
           const uint32_t frequency,
           const uint8_t channels,
           const uint32_t rate));
  MOCK_CONST_METHOD2(UpdatePayloadRate,
      void(ModuleRTPUtility::Payload* payload, const uint32_t rate));
  MOCK_CONST_METHOD1(GetPayloadTypeFrequency, int(
      const ModuleRTPUtility::Payload& payload));
  MOCK_CONST_METHOD5(CreatePayloadType,
      ModuleRTPUtility::Payload*(
          const char payloadName[RTP_PAYLOAD_NAME_SIZE],
          const int8_t payloadType,
          const uint32_t frequency,
          const uint8_t channels,
          const uint32_t rate));
};

}  

#endif  

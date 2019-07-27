









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ_TOOLS_CONSTANT_PCM_PACKET_SOURCE_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ_TOOLS_CONSTANT_PCM_PACKET_SOURCE_H_

#include <stdio.h>
#include <string>

#include "webrtc/base/constructormagic.h"
#include "webrtc/common_types.h"
#include "webrtc/modules/audio_coding/neteq/tools/packet_source.h"

namespace webrtc {
namespace test {




class ConstantPcmPacketSource : public PacketSource {
 public:
  ConstantPcmPacketSource(size_t payload_len_samples,
                          int16_t sample_value,
                          int sample_rate_hz,
                          int payload_type);

  
  
  Packet* NextPacket() OVERRIDE;

 private:
  void WriteHeader(uint8_t* packet_memory);

  const size_t kHeaderLenBytes = 12;
  const size_t payload_len_samples_;
  const size_t packet_len_bytes_;
  int16_t encoded_sample_;
  const int samples_per_ms_;
  double next_arrival_time_ms_;
  const int payload_type_;
  uint16_t seq_number_;
  uint32_t timestamp_;
  const uint32_t payload_ssrc_;

  DISALLOW_COPY_AND_ASSIGN(ConstantPcmPacketSource);
};

}  
}  
#endif  

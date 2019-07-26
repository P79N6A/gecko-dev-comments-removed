









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ4_TOOLS_RTP_GENERATOR_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ4_TOOLS_RTP_GENERATOR_H_

#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/typedefs.h"

namespace webrtc {
namespace test {


class RtpGenerator {
 public:
  RtpGenerator(int samples_per_ms,
               uint16_t start_seq_number = 0,
               uint32_t start_timestamp = 0,
               uint32_t start_send_time_ms = 0,
               uint32_t ssrc = 0x12345678)
      : seq_number_(start_seq_number),
        timestamp_(start_timestamp),
        next_send_time_ms_(start_send_time_ms),
        ssrc_(ssrc),
        samples_per_ms_(samples_per_ms),
        drift_factor_(0.0) {
  }

  
  
  
  uint32_t GetRtpHeader(uint8_t payload_type, size_t payload_length_samples,
                        WebRtcRTPHeader* rtp_header);

  void set_drift_factor(double factor);

 private:
  uint16_t seq_number_;
  uint32_t timestamp_;
  uint32_t next_send_time_ms_;
  const uint32_t ssrc_;
  const int samples_per_ms_;
  double drift_factor_;
  DISALLOW_COPY_AND_ASSIGN(RtpGenerator);
};

}  
}  
#endif  

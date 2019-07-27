









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ_TOOLS_RTP_GENERATOR_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ_TOOLS_RTP_GENERATOR_H_

#include "webrtc/base/constructormagic.h"
#include "webrtc/modules/interface/module_common_types.h"
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

  virtual ~RtpGenerator() {}

  
  
  
  virtual uint32_t GetRtpHeader(uint8_t payload_type,
                                size_t payload_length_samples,
                                WebRtcRTPHeader* rtp_header);

  void set_drift_factor(double factor);

 protected:
  uint16_t seq_number_;
  uint32_t timestamp_;
  uint32_t next_send_time_ms_;
  const uint32_t ssrc_;
  const int samples_per_ms_;
  double drift_factor_;

 private:
  DISALLOW_COPY_AND_ASSIGN(RtpGenerator);
};

class TimestampJumpRtpGenerator : public RtpGenerator {
 public:
  TimestampJumpRtpGenerator(int samples_per_ms,
                            uint16_t start_seq_number,
                            uint32_t start_timestamp,
                            uint32_t jump_from_timestamp,
                            uint32_t jump_to_timestamp)
      : RtpGenerator(samples_per_ms, start_seq_number, start_timestamp),
        jump_from_timestamp_(jump_from_timestamp),
        jump_to_timestamp_(jump_to_timestamp) {}

  uint32_t GetRtpHeader(uint8_t payload_type,
                        size_t payload_length_samples,
                        WebRtcRTPHeader* rtp_header) OVERRIDE;

 private:
  uint32_t jump_from_timestamp_;
  uint32_t jump_to_timestamp_;
  DISALLOW_COPY_AND_ASSIGN(TimestampJumpRtpGenerator);
};

}  
}  
#endif  

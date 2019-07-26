









#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_BITRATE_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_BITRATE_H_

#include <stdio.h>

#include <list>

#include "webrtc/common_types.h"
#include "webrtc/modules/rtp_rtcp/source/rtp_rtcp_config.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class Clock;
class CriticalSectionWrapper;

class Bitrate {
 public:
  explicit Bitrate(Clock* clock);

  
  void Process();

  
  void Update(const int32_t bytes);

  
  uint32_t PacketRate() const;

  
  uint32_t BitrateLast() const;

  
  uint32_t BitrateNow() const;

  int64_t time_last_rate_update() const;

 protected:
  Clock* clock_;

 private:
  scoped_ptr<CriticalSectionWrapper> crit_;
  uint32_t packet_rate_;
  uint32_t bitrate_;
  uint8_t bitrate_next_idx_;
  int64_t packet_rate_array_[10];
  int64_t bitrate_array_[10];
  int64_t bitrate_diff_ms_[10];
  int64_t time_last_rate_update_;
  uint32_t bytes_count_;
  uint32_t packet_count_;
};

}  

#endif  

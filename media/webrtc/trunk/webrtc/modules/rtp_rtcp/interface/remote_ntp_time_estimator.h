









#ifndef WEBRTC_MODULES_RTP_RTCP_INTERFACE_REMOTE_NTP_TIME_ESTIMATOR_H_
#define WEBRTC_MODULES_RTP_RTCP_INTERFACE_REMOTE_NTP_TIME_ESTIMATOR_H_

#include "webrtc/system_wrappers/interface/rtp_to_ntp.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

class Clock;
class TimestampExtrapolator;





class RemoteNtpTimeEstimator {
 public:
  explicit RemoteNtpTimeEstimator(Clock* clock);

  ~RemoteNtpTimeEstimator();

  
  
  bool UpdateRtcpTimestamp(uint16_t rtt, uint32_t ntp_secs, uint32_t ntp_frac,
                           uint32_t rtp_timestamp);

  
  
  int64_t Estimate(uint32_t rtp_timestamp);

 private:
  Clock* clock_;
  scoped_ptr<TimestampExtrapolator> ts_extrapolator_;
  RtcpList rtcp_list_;
  int64_t last_timing_log_ms_;
  DISALLOW_COPY_AND_ASSIGN(RemoteNtpTimeEstimator);
};

}  

#endif  

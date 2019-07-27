









#include "webrtc/modules/rtp_rtcp/interface/remote_ntp_time_estimator.h"

#include "webrtc/system_wrappers/interface/clock.h"
#include "webrtc/system_wrappers/interface/logging.h"
#include "webrtc/system_wrappers/interface/timestamp_extrapolator.h"

namespace webrtc {

static const int kTimingLogIntervalMs = 10000;



RemoteNtpTimeEstimator::RemoteNtpTimeEstimator(Clock* clock)
    : clock_(clock),
      ts_extrapolator_(new TimestampExtrapolator(clock_->TimeInMilliseconds())),
      last_timing_log_ms_(-1) {
}

RemoteNtpTimeEstimator::~RemoteNtpTimeEstimator() {}

bool RemoteNtpTimeEstimator::UpdateRtcpTimestamp(uint16_t rtt,
                                                 uint32_t ntp_secs,
                                                 uint32_t ntp_frac,
                                                 uint32_t rtcp_timestamp) {
  bool new_rtcp_sr = false;
  if (!UpdateRtcpList(
      ntp_secs, ntp_frac, rtcp_timestamp, &rtcp_list_, &new_rtcp_sr)) {
    return false;
  }
  if (!new_rtcp_sr) {
    
    return true;
  }
  
  
  int64_t receiver_arrival_time_ms = clock_->TimeInMilliseconds();
  int64_t sender_send_time_ms = Clock::NtpToMs(ntp_secs, ntp_frac);
  int64_t sender_arrival_time_90k = (sender_send_time_ms + rtt / 2) * 90;
  ts_extrapolator_->Update(receiver_arrival_time_ms, sender_arrival_time_90k);
  return true;
}

int64_t RemoteNtpTimeEstimator::Estimate(uint32_t rtp_timestamp) {
  if (rtcp_list_.size() < 2) {
    
    return -1;
  }
  int64_t sender_capture_ntp_ms = 0;
  if (!RtpToNtpMs(rtp_timestamp, rtcp_list_, &sender_capture_ntp_ms)) {
    return -1;
  }
  uint32_t timestamp = sender_capture_ntp_ms * 90;
  int64_t receiver_capture_ms =
      ts_extrapolator_->ExtrapolateLocalTime(timestamp);
  int64_t ntp_offset =
      clock_->CurrentNtpInMilliseconds() - clock_->TimeInMilliseconds();
  int64_t receiver_capture_ntp_ms = receiver_capture_ms + ntp_offset;
  int64_t now_ms = clock_->TimeInMilliseconds();
  if (now_ms - last_timing_log_ms_ > kTimingLogIntervalMs) {
    LOG(LS_INFO) << "RTP timestamp: " << rtp_timestamp
                 << " in NTP clock: " << sender_capture_ntp_ms
                 << " estimated time in receiver clock: " << receiver_capture_ms
                 << " converted to NTP clock: " << receiver_capture_ntp_ms;
    last_timing_log_ms_ = now_ms;
  }
  return receiver_capture_ntp_ms;
}

}  











#include "webrtc/modules/remote_bitrate_estimator/include/rtp_to_ntp.h"

#include "webrtc/system_wrappers/interface/clock.h"

#include <assert.h>

namespace webrtc {

namespace synchronization {

RtcpMeasurement::RtcpMeasurement()
    : ntp_secs(0), ntp_frac(0), rtp_timestamp(0) {}

RtcpMeasurement::RtcpMeasurement(uint32_t ntp_secs, uint32_t ntp_frac,
                                 uint32_t timestamp)
    : ntp_secs(ntp_secs), ntp_frac(ntp_frac), rtp_timestamp(timestamp) {}



bool CalculateFrequency(
    int64_t rtcp_ntp_ms1,
    uint32_t rtp_timestamp1,
    int64_t rtcp_ntp_ms2,
    uint32_t rtp_timestamp2,
    double* frequency_khz) {
  if (rtcp_ntp_ms1 <= rtcp_ntp_ms2) {
    return false;
  }
  *frequency_khz = static_cast<double>(rtp_timestamp1 - rtp_timestamp2) /
      static_cast<double>(rtcp_ntp_ms1 - rtcp_ntp_ms2);
  return true;
}



bool CompensateForWrapAround(uint32_t new_timestamp,
                             uint32_t old_timestamp,
                             int64_t* compensated_timestamp) {
  assert(compensated_timestamp);
  int64_t wraps = synchronization::CheckForWrapArounds(new_timestamp,
                                                       old_timestamp);
  if (wraps < 0) {
    
    return false;
  }
  *compensated_timestamp = new_timestamp + (wraps << 32);
  return true;
}





bool RtpToNtpMs(int64_t rtp_timestamp,
                const synchronization::RtcpList& rtcp,
                int64_t* rtp_timestamp_in_ms) {
  assert(rtcp.size() == 2);
  int64_t rtcp_ntp_ms_new = Clock::NtpToMs(rtcp.front().ntp_secs,
                                           rtcp.front().ntp_frac);
  int64_t rtcp_ntp_ms_old = Clock::NtpToMs(rtcp.back().ntp_secs,
                                           rtcp.back().ntp_frac);
  int64_t rtcp_timestamp_new = rtcp.front().rtp_timestamp;
  int64_t rtcp_timestamp_old = rtcp.back().rtp_timestamp;
  if (!CompensateForWrapAround(rtcp_timestamp_new,
                               rtcp_timestamp_old,
                               &rtcp_timestamp_new)) {
    return false;
  }
  double freq_khz;
  if (!CalculateFrequency(rtcp_ntp_ms_new,
                          rtcp_timestamp_new,
                          rtcp_ntp_ms_old,
                          rtcp_timestamp_old,
                          &freq_khz)) {
    return false;
  }
  double offset = rtcp_timestamp_new - freq_khz * rtcp_ntp_ms_new;
  int64_t rtp_timestamp_unwrapped;
  if (!CompensateForWrapAround(rtp_timestamp, rtcp_timestamp_old,
                               &rtp_timestamp_unwrapped)) {
    return false;
  }
  double rtp_timestamp_ntp_ms = (static_cast<double>(rtp_timestamp_unwrapped) -
      offset) / freq_khz + 0.5f;
  if (rtp_timestamp_ntp_ms < 0) {
    return false;
  }
  *rtp_timestamp_in_ms = rtp_timestamp_ntp_ms;
  return true;
}

int CheckForWrapArounds(uint32_t new_timestamp, uint32_t old_timestamp) {
  if (new_timestamp < old_timestamp) {
    
    
    
    if (static_cast<int32_t>(new_timestamp - old_timestamp) > 0) {
      
      return 1;
    }
  } else if (static_cast<int32_t>(old_timestamp - new_timestamp) > 0) {
    
    
    return -1;
  }
  return 0;
}
}  
}  











#ifndef SYSTEM_WRAPPERS_INTERFACE_RTP_TO_NTP_H_
#define SYSTEM_WRAPPERS_INTERFACE_RTP_TO_NTP_H_

#include <list>

#include "webrtc/typedefs.h"

namespace webrtc {

struct RtcpMeasurement {
  RtcpMeasurement();
  RtcpMeasurement(uint32_t ntp_secs, uint32_t ntp_frac, uint32_t timestamp);
  uint32_t ntp_secs;
  uint32_t ntp_frac;
  uint32_t rtp_timestamp;
};

typedef std::list<RtcpMeasurement> RtcpList;




bool UpdateRtcpList(uint32_t ntp_secs,
                    uint32_t ntp_frac,
                    uint32_t rtp_timestamp,
                    RtcpList* rtcp_list,
                    bool* new_rtcp_sr);



bool RtpToNtpMs(int64_t rtp_timestamp, const RtcpList& rtcp,
                int64_t* timestamp_in_ms);



int CheckForWrapArounds(uint32_t rtp_timestamp, uint32_t rtcp_rtp_timestamp);

}  

#endif  

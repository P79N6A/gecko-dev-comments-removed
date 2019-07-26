











#ifndef WEBRTC_VIDEO_ENGINE_NEW_INCLUDE_CONFIG_H_
#define WEBRTC_VIDEO_ENGINE_NEW_INCLUDE_CONFIG_H_

#include <string>
#include <vector>

#include "webrtc/common_types.h"
#include "webrtc/typedefs.h"

namespace webrtc {

struct RtpStatistics {
  RtpStatistics()
      : ssrc(0),
        fraction_loss(0),
        cumulative_loss(0),
        extended_max_sequence_number(0) {}
  uint32_t ssrc;
  int fraction_loss;
  int cumulative_loss;
  int extended_max_sequence_number;
  std::string c_name;
};

struct StreamStats {
  StreamStats() : key_frames(0), delta_frames(0), bitrate_bps(0) {}
  uint32_t key_frames;
  uint32_t delta_frames;
  int32_t bitrate_bps;
  StreamDataCounters rtp_stats;
  RtcpStatistics rtcp_stats;
};


struct NackConfig {
  NackConfig() : rtp_history_ms(0) {}
  
  
  
  
  int rtp_history_ms;
};



struct FecConfig {
  FecConfig() : ulpfec_payload_type(-1), red_payload_type(-1) {}
  
  int ulpfec_payload_type;

  
  int red_payload_type;
};


struct RtpExtension {
  static const char* kTOffset;
  static const char* kAbsSendTime;
  RtpExtension(const char* name, int id) : name(name), id(id) {}
  
  std::string name;
  int id;
};
}  

#endif  

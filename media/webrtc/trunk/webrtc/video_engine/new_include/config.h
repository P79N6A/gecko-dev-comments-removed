









#ifndef WEBRTC_VIDEO_ENGINE_NEW_INCLUDE_CONFIG_H_
#define WEBRTC_VIDEO_ENGINE_NEW_INCLUDE_CONFIG_H_

#include <string>

namespace webrtc {
namespace newapi {

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



enum RtcpMode {
  kRtcpCompound,
  kRtcpReducedSize
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


struct RtxConfig {
  RtxConfig() : ssrc(0), rtx_payload_type(0), video_payload_type(0) {}
  
  uint32_t ssrc;

  
  int rtx_payload_type;

  
  int video_payload_type;
};


struct RtpExtension {
  RtpExtension() : id(0) {}
  
  std::string name;
  int id;
};
}  
}  

#endif  

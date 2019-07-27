











#ifndef WEBRTC_CONFIG_H_
#define WEBRTC_CONFIG_H_

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
};

struct SsrcStats {
  SsrcStats()
      : key_frames(0),
        delta_frames(0),
        total_bitrate_bps(0),
        retransmit_bitrate_bps(0),
        avg_delay_ms(0),
        max_delay_ms(0) {}
  uint32_t key_frames;
  uint32_t delta_frames;
  
  int total_bitrate_bps;
  int retransmit_bitrate_bps;
  int avg_delay_ms;
  int max_delay_ms;
  StreamDataCounters rtp_stats;
  RtcpStatistics rtcp_stats;
};


struct NackConfig {
  NackConfig() : rtp_history_ms(0) {}
  
  
  
  
  int rtp_history_ms;
};



struct FecConfig {
  FecConfig() : ulpfec_payload_type(-1), red_payload_type(-1) {}
  std::string ToString() const;
  
  int ulpfec_payload_type;

  
  int red_payload_type;
};


struct RtpExtension {
  RtpExtension(const std::string& name, int id) : name(name), id(id) {}
  std::string ToString() const;
  static bool IsSupported(const std::string& name);

  static const char* kTOffset;
  static const char* kAbsSendTime;
  std::string name;
  int id;
};

struct VideoStream {
  VideoStream()
      : width(0),
        height(0),
        max_framerate(-1),
        min_bitrate_bps(-1),
        target_bitrate_bps(-1),
        max_bitrate_bps(-1),
        max_qp(-1) {}
  std::string ToString() const;

  size_t width;
  size_t height;
  int max_framerate;

  int min_bitrate_bps;
  int target_bitrate_bps;
  int max_bitrate_bps;

  int max_qp;

  
  
  
  
  
  
  
  
  
  
  std::vector<int> temporal_layer_thresholds_bps;
};

struct VideoEncoderConfig {
  enum ContentType {
    kRealtimeVideo,
    kScreenshare,
  };

  VideoEncoderConfig()
      : content_type(kRealtimeVideo),
        encoder_specific_settings(NULL),
        min_transmit_bitrate_bps(0) {}

  std::string ToString() const;

  std::vector<VideoStream> streams;
  ContentType content_type;
  void* encoder_specific_settings;

  
  
  
  
  int min_transmit_bitrate_bps;
};

}  

#endif  

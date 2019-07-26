









#ifndef WEBRTC_VIDEO_ENGINE_NEW_INCLUDE_VIDEO_RECEIVE_STREAM_H_
#define WEBRTC_VIDEO_ENGINE_NEW_INCLUDE_VIDEO_RECEIVE_STREAM_H_

#include <string>
#include <vector>

#include "webrtc/common_types.h"
#include "webrtc/video_engine/new_include/common.h"

namespace webrtc {

class VideoDecoder;

namespace newapi {

struct ReceiveStatistics {
  RtpStatistics rtp_stats;
  int network_frame_rate;
  int decode_frame_rate;
  int render_frame_rate;
  uint32_t key_frames;
  uint32_t delta_frames;
  uint32_t video_packets;
  uint32_t retransmitted_packets;
  uint32_t fec_packets;
  uint32_t padding_packets;
  uint32_t discarded_packets;
  int32_t received_bitrate_bps;
  int receive_side_delay_ms;
};


struct RtpReceiveConfig {
  
  uint32_t ssrc;

  
  NackConfig* nack;

  
  FecConfig* fec;

  
  std::vector<RtxConfig> rtx;

  
  std::vector<RtpExtension> rtp_extensions;
};



struct ExternalVideoDecoder {
  
  VideoDecoder* decoder;

  
  
  int payload_type;

  
  bool renderer;

  
  
  
  
  int expected_delay_ms;
};

struct VideoReceiveStreamConfig {
  
  std::vector<VideoCodec> codecs;

  RtpReceiveConfig rtp;

  
  
  VideoRenderer* renderer;

  
  
  
  int render_delay_ms;

  
  
  
  int audio_channel_id;

  
  
  EncodedFrameObserver* pre_decode_callback;

  
  
  I420FrameCallback* post_decode_callback;

  
  
  std::vector<ExternalVideoDecoder> external_decoders;

  
  
  int target_delay_ms;
};

class VideoReceiveStream {
 public:
  virtual void StartReceive() = 0;
  virtual void StopReceive() = 0;

  
  virtual void GetCurrentReceiveCodec(VideoCodec* receive_codec) = 0;

  virtual void GetReceiveStatistics(ReceiveStatistics* statistics) = 0;

 protected:
  virtual ~VideoReceiveStream() {}
};

}  
}  

#endif  

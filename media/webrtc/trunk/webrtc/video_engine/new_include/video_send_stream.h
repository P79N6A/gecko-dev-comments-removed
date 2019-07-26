









#ifndef WEBRTC_VIDEO_ENGINE_NEW_INCLUDE_VIDEO_SEND_STREAM_H_
#define WEBRTC_VIDEO_ENGINE_NEW_INCLUDE_VIDEO_SEND_STREAM_H_

#include <string>
#include <vector>

#include "webrtc/common_types.h"
#include "webrtc/video_engine/new_include/common.h"

namespace webrtc {

class VideoEncoder;

namespace newapi {


struct SendStreamState;

struct SendStatistics {
  RtpStatistics rtp;
  int input_frame_rate;
  int encode_frame;
  uint32_t key_frames;
  uint32_t delta_frames;
  uint32_t video_packets;
  uint32_t retransmitted_packets;
  uint32_t fec_packets;
  uint32_t padding_packets;
  int32_t send_bitrate_bps;
  int delay_ms;
};


class VideoSendStreamInput {
 public:
  
  
  virtual void PutFrame(const I420VideoFrame& video_frame,
                        int time_since_capture_ms) = 0;

 protected:
  virtual ~VideoSendStreamInput() {}
};

struct RtpSendConfig {
  RtcpMode mode;

  std::vector<uint32_t> ssrcs;

  
  size_t max_packet_size;

  
  std::vector<RtpExtension> rtp_extensions;

  
  NackConfig* nack;

  
  FecConfig* fec;

  
  RtxConfig* rtx;

  
  std::string c_name;
};

struct VideoSendStreamConfig {
  VideoCodec codec;

  RtpSendConfig rtp;

  
  
  I420FrameCallback* pre_encode_callback;

  
  
  EncodedFrameObserver* encoded_callback;

  
  
  VideoRenderer* local_renderer;

  
  
  
  int render_delay_ms;

  
  
  
  
  VideoEncoder* encoder;
  bool internal_source;

  
  
  int target_delay_ms;

  
  SendStreamState* start_state;
};

class VideoSendStream {
 public:
  
  
  virtual VideoSendStreamInput* Input() = 0;

  virtual void StartSend() = 0;
  virtual void StopSend() = 0;

  
  virtual void GetSendStatistics(std::vector<SendStatistics>* statistics) = 0;

  
  virtual bool SetTargetBitrate(
      int min_bitrate, int max_bitrate,
      const std::vector<SimulcastStream>& streams) = 0;

  virtual void GetSendCodec(VideoCodec* send_codec) = 0;

 protected:
  virtual ~VideoSendStream() {}
};

}  
}  

#endif  











#ifndef WEBRTC_VIDEO_ENGINE_NEW_INCLUDE_VIDEO_SEND_STREAM_H_
#define WEBRTC_VIDEO_ENGINE_NEW_INCLUDE_VIDEO_SEND_STREAM_H_

#include <string>
#include <vector>

#include "webrtc/common_types.h"
#include "webrtc/video_engine/new_include/config.h"
#include "webrtc/video_engine/new_include/frame_callback.h"
#include "webrtc/video_engine/new_include/video_renderer.h"

namespace webrtc {

class VideoEncoder;

struct SendStreamState;


class VideoSendStreamInput {
 public:
  
  
  virtual void PutFrame(const I420VideoFrame& video_frame,
                        uint32_t time_since_capture_ms) = 0;

 protected:
  virtual ~VideoSendStreamInput() {}
};

class VideoSendStream {
 public:
  struct Stats {
    Stats()
        : input_frame_rate(0),
          encode_frame(0),
          key_frames(0),
          delta_frames(0),
          video_packets(0),
          retransmitted_packets(0),
          fec_packets(0),
          padding_packets(0),
          send_bitrate_bps(0),
          delay_ms(0) {}
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

  class StatsCallback {
   public:
    virtual ~StatsCallback() {}
    virtual void ReceiveStats(const std::vector<Stats>& stats) = 0;
  };

  struct Config {
    Config()
        : pre_encode_callback(NULL),
          encoded_callback(NULL),
          local_renderer(NULL),
          render_delay_ms(0),
          encoder(NULL),
          internal_source(false),
          target_delay_ms(0),
          pacing(false),
          stats_callback(NULL),
          start_state(NULL) {}
    VideoCodec codec;

    struct Rtp {
      Rtp() : mode(newapi::kRtcpReducedSize), max_packet_size(0) {}
      newapi::RtcpMode mode;

      std::vector<uint32_t> ssrcs;

      
      size_t max_packet_size;

      
      std::vector<RtpExtension> extensions;

      
      NackConfig nack;

      
      FecConfig fec;

      
      RtxConfig rtx;

      
      std::string c_name;
    } rtp;

    
    
    I420FrameCallback* pre_encode_callback;

    
    
    EncodedFrameObserver* encoded_callback;

    
    
    VideoRenderer* local_renderer;

    
    
    
    int render_delay_ms;

    
    
    
    
    VideoEncoder* encoder;
    bool internal_source;

    
    
    int target_delay_ms;

    
    
    bool pacing;

    
    StatsCallback* stats_callback;

    
    SendStreamState* start_state;
  };

  
  
  virtual VideoSendStreamInput* Input() = 0;

  virtual void StartSend() = 0;
  virtual void StopSend() = 0;

  
  virtual bool SetTargetBitrate(
      int min_bitrate, int max_bitrate,
      const std::vector<SimulcastStream>& streams) = 0;

  virtual void GetSendCodec(VideoCodec* send_codec) = 0;

 protected:
  virtual ~VideoSendStream() {}
};

}  

#endif  

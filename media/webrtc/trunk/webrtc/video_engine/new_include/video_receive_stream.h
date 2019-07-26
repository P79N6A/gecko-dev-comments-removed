









#ifndef WEBRTC_VIDEO_ENGINE_NEW_INCLUDE_VIDEO_RECEIVE_STREAM_H_
#define WEBRTC_VIDEO_ENGINE_NEW_INCLUDE_VIDEO_RECEIVE_STREAM_H_

#include <string>
#include <vector>

#include "webrtc/common_types.h"
#include "webrtc/video_engine/new_include/config.h"
#include "webrtc/video_engine/new_include/frame_callback.h"
#include "webrtc/video_engine/new_include/transport.h"
#include "webrtc/video_engine/new_include/video_renderer.h"

namespace webrtc {

class VideoDecoder;



struct ExternalVideoDecoder {
  ExternalVideoDecoder()
      : decoder(NULL), payload_type(0), renderer(false), expected_delay_ms(0) {}
  
  VideoDecoder* decoder;

  
  
  int payload_type;

  
  bool renderer;

  
  
  
  
  int expected_delay_ms;
};

class VideoReceiveStream {
 public:
  struct Stats {
    Stats()
        : network_frame_rate(0),
          decode_frame_rate(0),
          render_frame_rate(0),
          key_frames(0),
          delta_frames(0),
          video_packets(0),
          retransmitted_packets(0),
          fec_packets(0),
          padding_packets(0),
          discarded_packets(0),
          received_bitrate_bps(0),
          receive_side_delay_ms(0) {}
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

  class StatsCallback {
   public:
    virtual ~StatsCallback() {}
    virtual void ReceiveStats(const Stats& stats) = 0;
  };

  struct Config {
    Config()
        : renderer(NULL),
          render_delay_ms(0),
          audio_channel_id(0),
          pre_decode_callback(NULL),
          post_decode_callback(NULL),
          target_delay_ms(0) {}
    
    std::vector<VideoCodec> codecs;

    
    struct Rtp {
      Rtp() : ssrc(0) {}
      
      
      uint32_t ssrc;

      
      NackConfig nack;

      
      FecConfig fec;

      
      
      std::vector<RtxConfig> rtx;

      
      std::vector<RtpExtension> extensions;
    } rtp;

    
    
    VideoRenderer* renderer;

    
    
    
    int render_delay_ms;

    
    
    
    int audio_channel_id;

    
    
    
    EncodedFrameObserver* pre_decode_callback;

    
    
    
    I420FrameCallback* post_decode_callback;

    
    
    std::vector<ExternalVideoDecoder> external_decoders;

    
    
    int target_delay_ms;

    
    StatsCallback* stats_callback;
  };

  virtual void StartReceive() = 0;
  virtual void StopReceive() = 0;

  
  virtual void GetCurrentReceiveCodec(VideoCodec* receive_codec) = 0;

 protected:
  virtual ~VideoReceiveStream() {}
};

}  

#endif  











#ifndef WEBRTC_VIDEO_SEND_STREAM_H_
#define WEBRTC_VIDEO_SEND_STREAM_H_

#include <map>
#include <string>

#include "webrtc/common_types.h"
#include "webrtc/config.h"
#include "webrtc/frame_callback.h"
#include "webrtc/video_renderer.h"

namespace webrtc {

class VideoEncoder;


class VideoSendStreamInput {
 public:
  
  
  
  virtual void PutFrame(const I420VideoFrame& video_frame) = 0;
  virtual void SwapFrame(I420VideoFrame* video_frame) = 0;

 protected:
  virtual ~VideoSendStreamInput() {}
};

class VideoSendStream {
 public:
  struct Stats {
    Stats()
        : input_frame_rate(0),
          encode_frame_rate(0),
          avg_delay_ms(0),
          max_delay_ms(0) {}

    int input_frame_rate;
    int encode_frame_rate;
    int avg_delay_ms;
    int max_delay_ms;
    std::string c_name;
    std::map<uint32_t, StreamStats> substreams;
  };

  struct Config {
    Config()
        : pre_encode_callback(NULL),
          post_encode_callback(NULL),
          local_renderer(NULL),
          render_delay_ms(0),
          encoder(NULL),
          internal_source(false),
          target_delay_ms(0),
          pacing(false),
          suspend_below_min_bitrate(false) {}
    VideoCodec codec;

    static const size_t kDefaultMaxPacketSize = 1500 - 40;  
    struct Rtp {
      Rtp() : max_packet_size(kDefaultMaxPacketSize) {}

      std::vector<uint32_t> ssrcs;

      
      size_t max_packet_size;

      
      std::vector<RtpExtension> extensions;

      
      NackConfig nack;

      
      FecConfig fec;

      
      
      struct Rtx {
        Rtx() : payload_type(0) {}
        
        std::vector<uint32_t> ssrcs;

        
        int payload_type;
      } rtx;

      
      std::string c_name;
    } rtp;

    
    
    I420FrameCallback* pre_encode_callback;

    
    
    EncodedFrameObserver* post_encode_callback;

    
    
    VideoRenderer* local_renderer;

    
    
    
    int render_delay_ms;

    
    
    
    
    VideoEncoder* encoder;
    bool internal_source;

    
    
    int target_delay_ms;

    
    
    bool pacing;

    
    
    
    
    
    bool suspend_below_min_bitrate;
  };

  
  
  virtual VideoSendStreamInput* Input() = 0;

  virtual void StartSending() = 0;
  virtual void StopSending() = 0;

  virtual bool SetCodec(const VideoCodec& codec) = 0;
  virtual VideoCodec GetCodec() = 0;

  virtual Stats GetStats() const = 0;

 protected:
  virtual ~VideoSendStream() {}
};

}  

#endif  

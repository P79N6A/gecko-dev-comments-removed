









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
          media_bitrate_bps(0),
          suspended(false) {}
    int input_frame_rate;
    int encode_frame_rate;
    int media_bitrate_bps;
    bool suspended;
    std::map<uint32_t, SsrcStats> substreams;
  };

  struct Config {
    Config()
        : pre_encode_callback(NULL),
          post_encode_callback(NULL),
          local_renderer(NULL),
          render_delay_ms(0),
          target_delay_ms(0),
          suspend_below_min_bitrate(false) {}
    std::string ToString() const;

    struct EncoderSettings {
      EncoderSettings() : payload_type(-1), encoder(NULL) {}

      std::string ToString() const;

      std::string payload_name;
      int payload_type;

      
      
      webrtc::VideoEncoder* encoder;
    } encoder_settings;

    static const size_t kDefaultMaxPacketSize = 1500 - 40;  
    struct Rtp {
      Rtp() : max_packet_size(kDefaultMaxPacketSize) {}
      std::string ToString() const;

      std::vector<uint32_t> ssrcs;

      
      size_t max_packet_size;

      
      std::vector<RtpExtension> extensions;

      
      NackConfig nack;

      
      FecConfig fec;

      
      
      struct Rtx {
        Rtx() : payload_type(-1), pad_with_redundant_payloads(false) {}
        std::string ToString() const;
        
        std::vector<uint32_t> ssrcs;

        
        int payload_type;
        
        
        
        bool pad_with_redundant_payloads;
      } rtx;

      
      std::string c_name;
    } rtp;

    
    
    I420FrameCallback* pre_encode_callback;

    
    
    EncodedFrameObserver* post_encode_callback;

    
    
    VideoRenderer* local_renderer;

    
    
    
    int render_delay_ms;

    
    
    int target_delay_ms;

    
    
    
    bool suspend_below_min_bitrate;
  };

  
  
  virtual VideoSendStreamInput* Input() = 0;

  virtual void Start() = 0;
  virtual void Stop() = 0;

  
  
  
  virtual bool ReconfigureVideoEncoder(const VideoEncoderConfig& config) = 0;

  virtual Stats GetStats() const = 0;

 protected:
  virtual ~VideoSendStream() {}
};

}  

#endif  

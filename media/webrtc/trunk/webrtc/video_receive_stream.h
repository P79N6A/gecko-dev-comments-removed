









#ifndef WEBRTC_VIDEO_RECEIVE_STREAM_H_
#define WEBRTC_VIDEO_RECEIVE_STREAM_H_

#include <map>
#include <string>
#include <vector>

#include "webrtc/common_types.h"
#include "webrtc/config.h"
#include "webrtc/frame_callback.h"
#include "webrtc/transport.h"
#include "webrtc/video_renderer.h"

namespace webrtc {

namespace newapi {


enum RtcpMode { kRtcpCompound, kRtcpReducedSize };
}  

class VideoDecoder;

class VideoReceiveStream {
 public:
  
  
  struct Decoder {
    Decoder()
        : decoder(NULL),
          payload_type(0),
          renderer(false),
          expected_delay_ms(0) {}

    
    VideoDecoder* decoder;

    
    
    int payload_type;

    
    
    std::string payload_name;

    
    bool renderer;

    
    
    
    
    int expected_delay_ms;
  };

  struct Stats : public SsrcStats {
    Stats()
        : network_frame_rate(0),
          decode_frame_rate(0),
          render_frame_rate(0),
          avg_delay_ms(0),
          discarded_packets(0),
          ssrc(0) {}

    int network_frame_rate;
    int decode_frame_rate;
    int render_frame_rate;
    int avg_delay_ms;
    int discarded_packets;
    uint32_t ssrc;
    std::string c_name;
  };

  struct Config {
    Config()
        : renderer(NULL),
          render_delay_ms(0),
          audio_channel_id(-1),
          pre_decode_callback(NULL),
          pre_render_callback(NULL),
          target_delay_ms(0) {}

    
    std::vector<Decoder> decoders;

    
    struct Rtp {
      Rtp()
          : remote_ssrc(0),
            local_ssrc(0),
            rtcp_mode(newapi::kRtcpReducedSize),
            remb(true) {}

      
      uint32_t remote_ssrc;
      
      uint32_t local_ssrc;

      
      newapi::RtcpMode rtcp_mode;

      
      struct RtcpXr {
        RtcpXr() : receiver_reference_time_report(false) {}

        
        
        bool receiver_reference_time_report;
      } rtcp_xr;

      
      bool remb;

      
      NackConfig nack;

      
      FecConfig fec;

      
      
      struct Rtx {
        Rtx() : ssrc(0), payload_type(0) {}

        
        uint32_t ssrc;

        
        int payload_type;
      };

      
      typedef std::map<int, Rtx> RtxMap;
      RtxMap rtx;

      
      std::vector<RtpExtension> extensions;
    } rtp;

    
    
    VideoRenderer* renderer;

    
    
    
    int render_delay_ms;

    
    
    
    int audio_channel_id;

    
    
    
    EncodedFrameObserver* pre_decode_callback;

    
    
    
    I420FrameCallback* pre_render_callback;

    
    
    int target_delay_ms;
  };

  virtual void Start() = 0;
  virtual void Stop() = 0;

  
  virtual Stats GetStats() const = 0;

 protected:
  virtual ~VideoReceiveStream() {}
};

}  

#endif  

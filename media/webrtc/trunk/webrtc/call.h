








#ifndef WEBRTC_CALL_H_
#define WEBRTC_CALL_H_

#include <string>
#include <vector>

#include "webrtc/common_types.h"
#include "webrtc/video_receive_stream.h"
#include "webrtc/video_send_stream.h"

namespace webrtc {

class VoiceEngine;

const char* Version();

class PacketReceiver {
 public:
  virtual bool DeliverPacket(const uint8_t* packet, size_t length) = 0;

 protected:
  virtual ~PacketReceiver() {}
};



class OveruseCallback {
 public:
  
  virtual void OnOveruse() = 0;
  
  virtual void OnNormalUse() = 0;

 protected:
  virtual ~OveruseCallback() {}
};




class Call {
 public:
  struct Config {
    explicit Config(newapi::Transport* send_transport)
        : webrtc_config(NULL),
          send_transport(send_transport),
          voice_engine(NULL),
          trace_callback(NULL),
          trace_filter(kTraceDefault),
          overuse_callback(NULL) {}

    webrtc::Config* webrtc_config;

    newapi::Transport* send_transport;

    
    VoiceEngine* voice_engine;

    TraceCallback* trace_callback;
    uint32_t trace_filter;

    
    
    OveruseCallback* overuse_callback;
  };

  static Call* Create(const Call::Config& config);

  static Call* Create(const Call::Config& config,
                      const webrtc::Config& webrtc_config);

  virtual std::vector<VideoCodec> GetVideoCodecs() = 0;

  virtual VideoSendStream::Config GetDefaultSendConfig() = 0;

  virtual VideoSendStream* CreateVideoSendStream(
      const VideoSendStream::Config& config) = 0;

  virtual void DestroyVideoSendStream(VideoSendStream* send_stream) = 0;

  virtual VideoReceiveStream::Config GetDefaultReceiveConfig() = 0;

  virtual VideoReceiveStream* CreateVideoReceiveStream(
      const VideoReceiveStream::Config& config) = 0;
  virtual void DestroyVideoReceiveStream(
      VideoReceiveStream* receive_stream) = 0;

  
  
  
  virtual PacketReceiver* Receiver() = 0;

  
  
  virtual uint32_t SendBitrateEstimate() = 0;

  
  
  virtual uint32_t ReceiveBitrateEstimate() = 0;

  virtual ~Call() {}
};
}  

#endif  










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
  enum DeliveryStatus {
    DELIVERY_OK,
    DELIVERY_UNKNOWN_SSRC,
    DELIVERY_PACKET_ERROR,
  };

  virtual DeliveryStatus DeliverPacket(const uint8_t* packet,
                                       size_t length) = 0;

 protected:
  virtual ~PacketReceiver() {}
};


class LoadObserver {
 public:
  enum Load { kOveruse, kUnderuse };

  
  
  virtual void OnLoadUpdate(Load load) = 0;

 protected:
  virtual ~LoadObserver() {}
};




class Call {
 public:
  enum NetworkState {
    kNetworkUp,
    kNetworkDown,
  };
  struct Config {
    explicit Config(newapi::Transport* send_transport)
        : webrtc_config(NULL),
          send_transport(send_transport),
          voice_engine(NULL),
          overuse_callback(NULL),
          stream_start_bitrate_bps(kDefaultStartBitrateBps) {}

    static const int kDefaultStartBitrateBps;

    webrtc::Config* webrtc_config;

    newapi::Transport* send_transport;

    
    VoiceEngine* voice_engine;

    
    
    LoadObserver* overuse_callback;

    
    
    
    
    int stream_start_bitrate_bps;
  };

  struct Stats {
    Stats() : send_bandwidth_bps(0), recv_bandwidth_bps(0), pacer_delay_ms(0) {}

    int send_bandwidth_bps;
    int recv_bandwidth_bps;
    int pacer_delay_ms;
  };

  static Call* Create(const Call::Config& config);

  static Call* Create(const Call::Config& config,
                      const webrtc::Config& webrtc_config);

  virtual VideoSendStream* CreateVideoSendStream(
      const VideoSendStream::Config& config,
      const VideoEncoderConfig& encoder_config) = 0;

  virtual void DestroyVideoSendStream(VideoSendStream* send_stream) = 0;

  virtual VideoReceiveStream* CreateVideoReceiveStream(
      const VideoReceiveStream::Config& config) = 0;
  virtual void DestroyVideoReceiveStream(
      VideoReceiveStream* receive_stream) = 0;

  
  
  
  virtual PacketReceiver* Receiver() = 0;

  
  
  virtual Stats GetStats() const = 0;

  virtual void SignalNetworkState(NetworkState state) = 0;

  virtual ~Call() {}
};
}  

#endif

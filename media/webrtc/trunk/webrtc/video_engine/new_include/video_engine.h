









#ifndef WEBRTC_VIDEO_ENGINE_NEW_INCLUDE_VIDEO_ENGINE_H_
#define WEBRTC_VIDEO_ENGINE_NEW_INCLUDE_VIDEO_ENGINE_H_

#include <string>
#include <vector>

#include "webrtc/common_types.h"
#include "webrtc/video_engine/new_include/video_receive_stream.h"
#include "webrtc/video_engine/new_include/video_send_stream.h"

namespace webrtc {
namespace newapi {

class VoiceEngine;

const char* Version();

class PacketReceiver {
 public:
  virtual bool DeliverPacket(const void* packet, size_t length) = 0;

 protected:
  virtual ~PacketReceiver() {}
};

struct VideoEngineConfig {
  VideoEngineConfig()
      : voice_engine(NULL), trace_callback(NULL), trace_filter(kTraceNone) {}

  
  VoiceEngine* voice_engine;

  TraceCallback* trace_callback;
  uint32_t trace_filter;
};




class VideoCall {
 public:
  virtual std::vector<VideoCodec> GetVideoCodecs() = 0;

  virtual VideoSendStream::Config GetDefaultSendConfig() = 0;

  virtual VideoSendStream* CreateSendStream(
      const VideoSendStream::Config& config) = 0;

  
  
  
  virtual SendStreamState* DestroySendStream(VideoSendStream* send_stream) = 0;

  virtual VideoReceiveStream::Config GetDefaultReceiveConfig() = 0;

  virtual VideoReceiveStream* CreateReceiveStream(
      const VideoReceiveStream::Config& config) = 0;
  virtual void DestroyReceiveStream(VideoReceiveStream* receive_stream) = 0;

  
  
  
  virtual PacketReceiver* Receiver() = 0;

  
  
  virtual uint32_t SendBitrateEstimate() = 0;

  
  
  virtual uint32_t ReceiveBitrateEstimate() = 0;

  virtual ~VideoCall() {}
};



class VideoEngine {
 public:
  static VideoEngine* Create(const VideoEngineConfig& engine_config);
  virtual ~VideoEngine() {}

  virtual VideoCall* CreateCall(Transport* send_transport) = 0;
};

}  
}  

#endif  

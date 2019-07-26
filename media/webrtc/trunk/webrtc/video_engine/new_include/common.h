









#ifndef WEBRTC_VIDEO_ENGINE_NEW_INCLUDE_COMMON_H_
#define WEBRTC_VIDEO_ENGINE_NEW_INCLUDE_COMMON_H_

#include <string>

#include "webrtc/common_types.h"

namespace webrtc {

class I420VideoFrame;

namespace newapi {

struct EncodedFrame;

class I420FrameCallback {
 public:
  
  
  virtual void FrameCallback(I420VideoFrame* video_frame) = 0;

 protected:
  virtual ~I420FrameCallback() {}
};

class EncodedFrameObserver {
 public:
  virtual void EncodedFrameCallback(const EncodedFrame& encoded_frame) = 0;

 protected:
  virtual ~EncodedFrameObserver() {}
};

class VideoRenderer {
 public:
  
  
  
  virtual void RenderFrame(const I420VideoFrame& video_frame,
                           int time_to_render_ms) = 0;

 protected:
  virtual ~VideoRenderer() {}
};

class Transport {
 public:
  virtual bool SendRTP(const void* packet, size_t length) = 0;
  virtual bool SendRTCP(const void* packet, size_t length) = 0;

 protected:
  virtual ~Transport() {}
};

struct RtpStatistics {
  uint32_t ssrc;
  int fraction_loss;
  int cumulative_loss;
  int extended_max_sequence_number;
  std::string c_name;
};



enum RtcpMode {
  kRtcpCompound,
  kRtcpReducedSize
};


struct NackConfig {
  
  
  
  
  int rtp_history_ms;
};



struct FecConfig {
  
  int ulpfec_payload_type;

  
  int red_payload_type;
};


struct RtxConfig {
  
  uint32_t ssrc;

  
  int rtx_payload_type;

  
  int video_payload_type;
};


struct RtpExtension {
  
  std::string name;
  int id;
};

}  
}  

#endif  

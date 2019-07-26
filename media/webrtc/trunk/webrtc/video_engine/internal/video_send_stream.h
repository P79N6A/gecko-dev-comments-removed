









#ifndef WEBRTC_VIDEO_ENGINE_VIDEO_SEND_STREAM_IMPL_H_
#define WEBRTC_VIDEO_ENGINE_VIDEO_SEND_STREAM_IMPL_H_

#include <vector>

#include "webrtc/common_video/libyuv/include/webrtc_libyuv.h"
#include "webrtc/video_engine/new_include/video_receive_stream.h"
#include "webrtc/video_engine/new_include/video_send_stream.h"

namespace webrtc {

class VideoEngine;
class ViEBase;
class ViECapture;
class ViECodec;
class ViEExternalCapture;
class ViENetwork;
class ViERTP_RTCP;

namespace internal {
class VideoSendStream : public newapi::VideoSendStream,
                        public newapi::VideoSendStreamInput,
                        public webrtc::Transport {
 public:
  VideoSendStream(newapi::Transport* transport,
                  webrtc::VideoEngine* video_engine,
                  const newapi::VideoSendStream::Config& config);

  virtual ~VideoSendStream();

  virtual void PutFrame(const I420VideoFrame& frame,
                        uint32_t time_since_capture_ms) OVERRIDE;

  virtual newapi::VideoSendStreamInput* Input() OVERRIDE;

  virtual void StartSend() OVERRIDE;

  virtual void StopSend() OVERRIDE;

  virtual bool SetTargetBitrate(int min_bitrate, int max_bitrate,
                                const std::vector<SimulcastStream>& streams)
      OVERRIDE;

  virtual void GetSendCodec(VideoCodec* send_codec) OVERRIDE;

  virtual int SendPacket(int , const void* packet, int length)
      OVERRIDE;

  virtual int SendRTCPPacket(int , const void* packet, int length)
      OVERRIDE;

 private:
  newapi::Transport* transport_;
  newapi::VideoSendStream::Config config_;

  ViEBase* video_engine_base_;
  ViECapture* capture_;
  ViECodec* codec_;
  ViEExternalCapture* external_capture_;
  ViENetwork* network_;
  ViERTP_RTCP* rtp_rtcp_;

  int channel_;
  int capture_id_;
};
}  
}  

#endif  

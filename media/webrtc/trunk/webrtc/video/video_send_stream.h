









#ifndef WEBRTC_VIDEO_VIDEO_SEND_STREAM_H_
#define WEBRTC_VIDEO_VIDEO_SEND_STREAM_H_

#include <map>
#include <vector>

#include "webrtc/call.h"
#include "webrtc/common_video/libyuv/include/webrtc_libyuv.h"
#include "webrtc/modules/rtp_rtcp/interface/rtp_rtcp_defines.h"
#include "webrtc/video/encoded_frame_callback_adapter.h"
#include "webrtc/video/send_statistics_proxy.h"
#include "webrtc/video/transport_adapter.h"
#include "webrtc/video_receive_stream.h"
#include "webrtc/video_send_stream.h"
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"

namespace webrtc {

class CpuOveruseObserver;
class VideoEngine;
class ViEBase;
class ViECapture;
class ViECodec;
class ViEExternalCapture;
class ViEExternalCodec;
class ViEImageProcess;
class ViENetwork;
class ViERTP_RTCP;

namespace internal {

class VideoSendStream : public webrtc::VideoSendStream,
                        public VideoSendStreamInput {
 public:
  VideoSendStream(newapi::Transport* transport,
                  CpuOveruseObserver* overuse_observer,
                  webrtc::VideoEngine* video_engine,
                  const VideoSendStream::Config& config,
                  const VideoEncoderConfig& encoder_config,
                  const std::map<uint32_t, RtpState>& suspended_ssrcs,
                  int base_channel,
                  int start_bitrate);

  virtual ~VideoSendStream();

  virtual void Start() OVERRIDE;
  virtual void Stop() OVERRIDE;

  virtual bool ReconfigureVideoEncoder(
      const VideoEncoderConfig& config) OVERRIDE;

  virtual Stats GetStats() const OVERRIDE;

  bool DeliverRtcp(const uint8_t* packet, size_t length);

  
  virtual void SwapFrame(I420VideoFrame* frame) OVERRIDE;

  
  virtual VideoSendStreamInput* Input() OVERRIDE;

  typedef std::map<uint32_t, RtpState> RtpStateMap;
  RtpStateMap GetRtpStates() const;

  void SignalNetworkState(Call::NetworkState state);

  int GetPacerQueuingDelayMs() const;

 private:
  void ConfigureSsrcs();
  TransportAdapter transport_adapter_;
  EncodedFrameCallbackAdapter encoded_frame_proxy_;
  const VideoSendStream::Config config_;
  const int start_bitrate_bps_;
  std::map<uint32_t, RtpState> suspended_ssrcs_;

  ViEBase* video_engine_base_;
  ViECapture* capture_;
  ViECodec* codec_;
  ViEExternalCapture* external_capture_;
  ViEExternalCodec* external_codec_;
  ViENetwork* network_;
  ViERTP_RTCP* rtp_rtcp_;
  ViEImageProcess* image_process_;

  int channel_;
  int capture_id_;

  
  
  
  bool use_default_bitrate_;

  SendStatisticsProxy stats_proxy_;
};
}  
}  

#endif  

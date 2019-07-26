















#ifndef WEBRTC_VIDEO_ENGINE_INCLUDE_VIE_CODEC_H_
#define WEBRTC_VIDEO_ENGINE_INCLUDE_VIE_CODEC_H_

#include "common_types.h"

namespace webrtc {

class VideoEngine;
struct VideoCodec;





class WEBRTC_DLLEXPORT ViEEncoderObserver {
 public:
  
  
  virtual void OutgoingRate(const int video_channel,
                            const unsigned int framerate,
                            const unsigned int bitrate) = 0;
 protected:
  virtual ~ViEEncoderObserver() {}
};





class WEBRTC_DLLEXPORT ViEDecoderObserver {
 public:
  
  
  virtual void IncomingCodecChanged(const int video_channel,
                                    const VideoCodec& video_codec) = 0;

  
  
  virtual void IncomingRate(const int video_channel,
                            const unsigned int framerate,
                            const unsigned int bitrate) = 0;

  
  
  virtual void RequestNewKeyFrame(const int video_channel) = 0;

 protected:
  virtual ~ViEDecoderObserver() {}
};

class WEBRTC_DLLEXPORT ViECodec {
 public:
  
  
  
  static ViECodec* GetInterface(VideoEngine* video_engine);

  
  
  
  
  virtual int Release() = 0;

  
  virtual int NumberOfCodecs() const = 0;

  
  
  virtual int GetCodec(const unsigned char list_number,
                       VideoCodec& video_codec) const = 0;

  
  virtual int SetSendCodec(const int video_channel,
                           const VideoCodec& video_codec) = 0;

  
  virtual int GetSendCodec(const int video_channel,
                           VideoCodec& video_codec) const = 0;

  
  
  virtual int SetReceiveCodec(const int video_channel,
                              const VideoCodec& video_codec) = 0;

  
  virtual int GetReceiveCodec(const int video_channel,
                              VideoCodec& video_codec) const = 0;

  
  
  virtual int GetCodecConfigParameters(
      const int video_channel,
      unsigned char config_parameters[kConfigParameterSize],
      unsigned char& config_parameters_size) const = 0;

  
  
  virtual int SetImageScaleStatus(const int video_channel,
                                  const bool enable) = 0;

  
  virtual int GetSendCodecStastistics(const int video_channel,
                                      unsigned int& key_frames,
                                      unsigned int& delta_frames) const = 0;

  
  virtual int GetReceiveCodecStastistics(const int video_channel,
                                         unsigned int& key_frames,
                                         unsigned int& delta_frames) const = 0;

  
  
  virtual int GetReceiveSideDelay(const int video_channel,
                                  int* delay_ms) const = 0;

  
  virtual int GetCodecTargetBitrate(const int video_channel,
                                    unsigned int* bitrate) const = 0;

  
  
  virtual unsigned int GetDiscardedPackets(const int video_channel) const = 0;

  
  virtual int SetKeyFrameRequestCallbackStatus(const int video_channel,
                                               const bool enable) = 0;

  
  virtual int SetSignalKeyPacketLossStatus(
      const int video_channel,
      const bool enable,
      const bool only_key_frames = false) = 0;

  
  virtual int RegisterEncoderObserver(const int video_channel,
                                      ViEEncoderObserver& observer) = 0;

  
  virtual int DeregisterEncoderObserver(const int video_channel) = 0;

  
  virtual int RegisterDecoderObserver(const int video_channel,
                                      ViEDecoderObserver& observer) = 0;

  
  virtual int DeregisterDecoderObserver(const int video_channel) = 0;

  
  
  
  virtual int SendKeyFrame(const int video_channel) = 0;

  
  
  virtual int WaitForFirstKeyFrame(const int video_channel,
                                   const bool wait) = 0;

 protected:
  ViECodec() {}
  virtual ~ViECodec() {}
};

}  

#endif  

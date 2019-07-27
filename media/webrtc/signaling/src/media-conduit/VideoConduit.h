



#ifndef VIDEO_SESSION_H_
#define VIDEO_SESSION_H_

#include "nsAutoPtr.h"
#include "mozilla/Attributes.h"

#include "MediaConduitInterface.h"
#include "MediaEngineWrapper.h"
#include "CodecStatistics.h"
#include "LoadManagerFactory.h"
#include "LoadManager.h"


#undef FF

#include "webrtc/common_types.h"
#ifdef FF
#undef FF // Avoid name collision between scoped_ptr.h and nsCRTGlue.h.
#endif
#include "webrtc/modules/video_coding/codecs/interface/video_codec_interface.h"
#include "webrtc/video_engine/include/vie_base.h"
#include "webrtc/video_engine/include/vie_capture.h"
#include "webrtc/video_engine/include/vie_codec.h"
#include "webrtc/video_engine/include/vie_external_codec.h"
#include "webrtc/video_engine/include/vie_render.h"
#include "webrtc/video_engine/include/vie_network.h"
#include "webrtc/video_engine/include/vie_rtp_rtcp.h"





 using  webrtc::ViEBase;
 using  webrtc::ViENetwork;
 using  webrtc::ViECodec;
 using  webrtc::ViECapture;
 using  webrtc::ViERender;
 using  webrtc::ViEExternalCapture;
 using  webrtc::ViEExternalCodec;

namespace mozilla {

class WebrtcAudioConduit;


class WebrtcVideoEncoder:public VideoEncoder
                         ,public webrtc::VideoEncoder
{};


class WebrtcVideoDecoder:public VideoDecoder
                         ,public webrtc::VideoDecoder
{};





class WebrtcVideoConduit:public VideoSessionConduit
                         ,public webrtc::Transport
                         ,public webrtc::ExternalRenderer
{
public:
  
  static const unsigned int CODEC_PLNAME_SIZE;

  


  void SyncTo(WebrtcAudioConduit *aConduit);

  





  virtual MediaConduitErrorCode AttachRenderer(mozilla::RefPtr<VideoRenderer> aVideoRenderer) MOZ_OVERRIDE;
  virtual void DetachRenderer() MOZ_OVERRIDE;

  



  virtual MediaConduitErrorCode ReceivedRTPPacket(const void *data, int len) MOZ_OVERRIDE;

  



  virtual MediaConduitErrorCode ReceivedRTCPPacket(const void *data, int len) MOZ_OVERRIDE;

  virtual MediaConduitErrorCode StopTransmitting() MOZ_OVERRIDE;
  virtual MediaConduitErrorCode StartTransmitting() MOZ_OVERRIDE;
  virtual MediaConduitErrorCode StopReceiving() MOZ_OVERRIDE;
  virtual MediaConduitErrorCode StartReceiving() MOZ_OVERRIDE;

   







  virtual MediaConduitErrorCode ConfigureSendMediaCodec(const VideoCodecConfig* codecInfo) MOZ_OVERRIDE;

  








   virtual MediaConduitErrorCode ConfigureRecvMediaCodecs(
                               const std::vector<VideoCodecConfig* >& codecConfigList) MOZ_OVERRIDE;

  



  virtual MediaConduitErrorCode SetTransmitterTransport(mozilla::RefPtr<TransportInterface> aTransport) MOZ_OVERRIDE;

  virtual MediaConduitErrorCode SetReceiverTransport(mozilla::RefPtr<TransportInterface> aTransport) MOZ_OVERRIDE;

  




  bool SelectSendResolution(unsigned short width,
                            unsigned short height);

  




  bool SelectSendFrameRate(unsigned int framerate);

  










  virtual MediaConduitErrorCode SendVideoFrame(unsigned char* video_frame,
                                                unsigned int video_frame_length,
                                                unsigned short width,
                                                unsigned short height,
                                                VideoType video_type,
                                                uint64_t capture_time) MOZ_OVERRIDE;

  



  virtual MediaConduitErrorCode SetExternalSendCodec(VideoCodecConfig* config,
                                                     VideoEncoder* encoder) MOZ_OVERRIDE;

  



  virtual MediaConduitErrorCode SetExternalRecvCodec(VideoCodecConfig* config,
                                                     VideoDecoder* decoder) MOZ_OVERRIDE;


  



  virtual int SendPacket(int channel, const void *data, int len) MOZ_OVERRIDE;

  



  virtual int SendRTCPPacket(int channel, const void *data, int len) MOZ_OVERRIDE;


  



  virtual int FrameSizeChange(unsigned int, unsigned int, unsigned int) MOZ_OVERRIDE;

  virtual int DeliverFrame(unsigned char*, int, uint32_t , int64_t,
                           int64_t, void *handle) MOZ_OVERRIDE;

  





  virtual bool IsTextureSupported() MOZ_OVERRIDE {
#ifdef WEBRTC_GONK
    return true;
#else
    return false;
#endif
  }

  virtual uint64_t CodecPluginID() MOZ_OVERRIDE;

  unsigned short SendingWidth() MOZ_OVERRIDE {
    return mSendingWidth;
  }

  unsigned short SendingHeight() MOZ_OVERRIDE {
    return mSendingHeight;
  }

  unsigned int SendingMaxFs() MOZ_OVERRIDE {
    if(mCurSendCodecConfig) {
      return mCurSendCodecConfig->mMaxFrameSize;
    }
    return 0;
  }

  unsigned int SendingMaxFr() MOZ_OVERRIDE {
    if(mCurSendCodecConfig) {
      return mCurSendCodecConfig->mMaxFrameRate;
    }
    return 0;
  }

  WebrtcVideoConduit();
  virtual ~WebrtcVideoConduit();

  MediaConduitErrorCode Init();

  int GetChannel() { return mChannel; }
  webrtc::VideoEngine* GetVideoEngine() { return mVideoEngine; }
  bool GetLocalSSRC(unsigned int* ssrc) MOZ_OVERRIDE;
  bool SetLocalSSRC(unsigned int ssrc) MOZ_OVERRIDE;
  bool GetRemoteSSRC(unsigned int* ssrc) MOZ_OVERRIDE;
  bool SetLocalCNAME(const char* cname) MOZ_OVERRIDE;
  bool GetVideoEncoderStats(double* framerateMean,
                            double* framerateStdDev,
                            double* bitrateMean,
                            double* bitrateStdDev,
                            uint32_t* droppedFrames) MOZ_OVERRIDE;
  bool GetVideoDecoderStats(double* framerateMean,
                            double* framerateStdDev,
                            double* bitrateMean,
                            double* bitrateStdDev,
                            uint32_t* discardedPackets) MOZ_OVERRIDE;
  bool GetAVStats(int32_t* jitterBufferDelayMs,
                  int32_t* playoutBufferDelayMs,
                  int32_t* avSyncOffsetMs) MOZ_OVERRIDE;
  bool GetRTPStats(unsigned int* jitterMs, unsigned int* cumulativeLost) MOZ_OVERRIDE;
  bool GetRTCPReceiverReport(DOMHighResTimeStamp* timestamp,
                             uint32_t* jitterMs,
                             uint32_t* packetsReceived,
                             uint64_t* bytesReceived,
                             uint32_t* cumulativeLost,
                             int32_t* rttMs) MOZ_OVERRIDE;
  bool GetRTCPSenderReport(DOMHighResTimeStamp* timestamp,
                           unsigned int* packetsSent,
                           uint64_t* bytesSent) MOZ_OVERRIDE;
  uint64_t MozVideoLatencyAvg();

private:

  WebrtcVideoConduit(const WebrtcVideoConduit& other) = delete;
  void operator=(const WebrtcVideoConduit& other) = delete;

  
  typedef std::vector<VideoCodecConfig* > RecvCodecList;

  
  void CodecConfigToWebRTCCodec(const VideoCodecConfig* codecInfo,
                                webrtc::VideoCodec& cinst);

  
  bool CopyCodecToDB(const VideoCodecConfig* codecInfo);

  
  
  bool CheckCodecForMatch(const VideoCodecConfig* codecInfo) const;
  bool CheckCodecsForMatch(const VideoCodecConfig* curCodecConfig,
                           const VideoCodecConfig* codecInfo) const;

  
  MediaConduitErrorCode ValidateCodecConfig(const VideoCodecConfig* codecInfo, bool send) const;

  
  void DumpCodecDB() const;

  
  void VideoLatencyUpdate(uint64_t new_sample);

  webrtc::VideoEngine* mVideoEngine;
  mozilla::ReentrantMonitor mTransportMonitor;
  mozilla::RefPtr<TransportInterface> mTransmitterTransport;
  mozilla::RefPtr<TransportInterface> mReceiverTransport;
  mozilla::RefPtr<VideoRenderer> mRenderer;

  ScopedCustomReleasePtr<webrtc::ViEBase> mPtrViEBase;
  ScopedCustomReleasePtr<webrtc::ViECapture> mPtrViECapture;
  ScopedCustomReleasePtr<webrtc::ViECodec> mPtrViECodec;
  ScopedCustomReleasePtr<webrtc::ViENetwork> mPtrViENetwork;
  ScopedCustomReleasePtr<webrtc::ViERender> mPtrViERender;
  ScopedCustomReleasePtr<webrtc::ViERTP_RTCP> mPtrRTP;
  ScopedCustomReleasePtr<webrtc::ViEExternalCodec> mPtrExtCodec;

  webrtc::ViEExternalCapture* mPtrExtCapture;

  
  mozilla::Atomic<bool> mEngineTransmitting; 
  mozilla::Atomic<bool> mEngineReceiving;    

  int mChannel; 
  int mCapId;   
  RecvCodecList    mRecvCodecList;
  VideoCodecConfig* mCurSendCodecConfig;
  unsigned short mSendingWidth;
  unsigned short mSendingHeight;
  unsigned short mReceivingWidth;
  unsigned short mReceivingHeight;
  unsigned int   mSendingFramerate;
  unsigned short mNumReceivingStreams;
  bool mVideoLatencyTestEnable;
  uint64_t mVideoLatencyAvg;
  uint32_t mMinBitrate;
  uint32_t mStartBitrate;
  uint32_t mMaxBitrate;

  static const unsigned int sAlphaNum = 7;
  static const unsigned int sAlphaDen = 8;
  static const unsigned int sRoundingPadding = 1024;

  mozilla::RefPtr<WebrtcAudioConduit> mSyncedTo;

  nsAutoPtr<VideoCodecConfig> mExternalSendCodec;
  nsAutoPtr<VideoCodecConfig> mExternalRecvCodec;
  nsAutoPtr<VideoEncoder> mExternalSendCodecHandle;
  nsAutoPtr<VideoDecoder> mExternalRecvCodecHandle;

  
  nsAutoPtr<VideoCodecStatistics> mVideoCodecStat;

  nsAutoPtr<LoadManager> mLoadManager;
};

} 

#endif

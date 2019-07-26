



#ifndef VIDEO_SESSION_H_
#define VIDEO_SESSION_H_

#include "mozilla/Attributes.h"

#include "MediaConduitInterface.h"
#include "MediaEngineWrapper.h"


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

  





  virtual MediaConduitErrorCode AttachRenderer(mozilla::RefPtr<VideoRenderer> aVideoRenderer);
  virtual void DetachRenderer();

  



  virtual MediaConduitErrorCode ReceivedRTPPacket(const void *data, int len);

  



  virtual MediaConduitErrorCode ReceivedRTCPPacket(const void *data, int len);

   







  virtual MediaConduitErrorCode ConfigureSendMediaCodec(const VideoCodecConfig* codecInfo);

  








   virtual MediaConduitErrorCode ConfigureRecvMediaCodecs(
                               const std::vector<VideoCodecConfig* >& codecConfigList);

  



  virtual MediaConduitErrorCode AttachTransport(mozilla::RefPtr<TransportInterface> aTransport);

  




  virtual bool SelectSendResolution(unsigned short width,
                                    unsigned short height);

  










  virtual MediaConduitErrorCode SendVideoFrame(unsigned char* video_frame,
                                                unsigned int video_frame_length,
                                                unsigned short width,
                                                unsigned short height,
                                                VideoType video_type,
                                                uint64_t capture_time);

  



  virtual MediaConduitErrorCode SetExternalSendCodec(int pltype,
                                                     VideoEncoder* encoder);

  



  virtual MediaConduitErrorCode SetExternalRecvCodec(int pltype,
                                                     VideoDecoder* decoder);


  



  virtual int SendPacket(int channel, const void *data, int len) ;

  



  virtual int SendRTCPPacket(int channel, const void *data, int len) ;


  



  virtual int FrameSizeChange(unsigned int, unsigned int, unsigned int);

  virtual int DeliverFrame(unsigned char*,int, uint32_t , int64_t,
                           void *handle);

  





  virtual bool IsTextureSupported() {
#ifdef WEBRTC_GONK
    return true;
#else
    return false;
#endif
  }

  unsigned short SendingWidth() {
    return mSendingWidth;
  }

  unsigned short SendingHeight() {
    return mSendingHeight;
  }

  unsigned int SendingMaxFs() {
    if(mCurSendCodecConfig) {
      return mCurSendCodecConfig->mMaxFrameSize;
    }
    return 0;
  }

  unsigned int SendingMaxFr() {
    if(mCurSendCodecConfig) {
      return mCurSendCodecConfig->mMaxFrameRate;
    }
    return 0;
  }

  WebrtcVideoConduit():
                      mOtherDirection(nullptr),
                      mShutDown(false),
                      mVideoEngine(nullptr),
                      mTransport(nullptr),
                      mRenderer(nullptr),
                      mPtrExtCapture(nullptr),
                      mEngineTransmitting(false),
                      mEngineReceiving(false),
                      mChannel(-1),
                      mCapId(-1),
                      mCurSendCodecConfig(nullptr),
                      mSendingWidth(0),
                      mSendingHeight(0),
                      mReceivingWidth(640),
                      mReceivingHeight(480),
                      mVideoLatencyTestEnable(false),
                      mVideoLatencyAvg(0),
                      mMinBitrate(200),
                      mStartBitrate(300),
                      mMaxBitrate(2000)
  {
  }

  virtual ~WebrtcVideoConduit() ;

  MediaConduitErrorCode Init(WebrtcVideoConduit *other);

  int GetChannel() { return mChannel; }
  webrtc::VideoEngine* GetVideoEngine() { return mVideoEngine; }
  bool GetLocalSSRC(unsigned int* ssrc);
  bool GetRemoteSSRC(unsigned int* ssrc);
  bool GetAVStats(int32_t* jitterBufferDelayMs,
                  int32_t* playoutBufferDelayMs,
                  int32_t* avSyncOffsetMs);
  bool GetRTPStats(unsigned int* jitterMs, unsigned int* cumulativeLost);
  bool GetRTCPReceiverReport(DOMHighResTimeStamp* timestamp,
                             uint32_t* jitterMs,
                             uint32_t* packetsReceived,
                             uint64_t* bytesReceived,
                             uint32_t* cumulativeLost,
                             int32_t* rttMs);
  bool GetRTCPSenderReport(DOMHighResTimeStamp* timestamp,
                           unsigned int* packetsSent,
                           uint64_t* bytesSent);
  uint64_t MozVideoLatencyAvg();

private:

  WebrtcVideoConduit(const WebrtcVideoConduit& other) MOZ_DELETE;
  void operator=(const WebrtcVideoConduit& other) MOZ_DELETE;

  
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

  
  
  
  WebrtcVideoConduit*  mOtherDirection;
  
  bool mShutDown;

  
  
  webrtc::VideoEngine* mVideoEngine;          
  mozilla::RefPtr<TransportInterface> mTransport;
  mozilla::RefPtr<VideoRenderer> mRenderer;

  ScopedCustomReleasePtr<webrtc::ViEBase> mPtrViEBase;
  ScopedCustomReleasePtr<webrtc::ViECapture> mPtrViECapture;
  ScopedCustomReleasePtr<webrtc::ViECodec> mPtrViECodec;
  ScopedCustomReleasePtr<webrtc::ViENetwork> mPtrViENetwork;
  ScopedCustomReleasePtr<webrtc::ViERender> mPtrViERender;
  ScopedCustomReleasePtr<webrtc::ViERTP_RTCP> mPtrRTP;
  ScopedCustomReleasePtr<webrtc::ViEExternalCodec> mPtrExtCodec;

  webrtc::ViEExternalCapture* mPtrExtCapture; 

  
  bool mEngineTransmitting; 
  bool mEngineReceiving;    

  int mChannel; 
  int mCapId;   
  RecvCodecList    mRecvCodecList;
  VideoCodecConfig* mCurSendCodecConfig;
  unsigned short mSendingWidth;
  unsigned short mSendingHeight;
  unsigned short mReceivingWidth;
  unsigned short mReceivingHeight;
  bool mVideoLatencyTestEnable;
  uint64_t mVideoLatencyAvg;
  uint32_t mMinBitrate;
  uint32_t mStartBitrate;
  uint32_t mMaxBitrate;

  static const unsigned int sAlphaNum = 7;
  static const unsigned int sAlphaDen = 8;
  static const unsigned int sRoundingPadding = 1024;

  mozilla::RefPtr<WebrtcAudioConduit> mSyncedTo;
};

} 

#endif

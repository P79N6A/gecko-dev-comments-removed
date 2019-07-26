



#ifndef VIDEO_SESSION_H_
#define VIDEO_SESSION_H_

#include "mozilla/Attributes.h"

#include "MediaConduitInterface.h"


#include "webrtc/common_types.h"
#include "webrtc/video_engine/include/vie_base.h"
#include "webrtc/video_engine/include/vie_capture.h"
#include "webrtc/video_engine/include/vie_codec.h"
#include "webrtc/video_engine/include/vie_render.h"
#include "webrtc/video_engine/include/vie_network.h"
#include "webrtc/video_engine/include/vie_rtp_rtcp.h"





 using  webrtc::ViEBase;
 using  webrtc::ViENetwork;
 using  webrtc::ViECodec;
 using  webrtc::ViECapture;
 using  webrtc::ViERender;
 using  webrtc::ViEExternalCapture;


namespace mozilla {

class WebrtcAudioConduit;





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



  



  virtual int SendPacket(int channel, const void *data, int len) ;

  



  virtual int SendRTCPPacket(int channel, const void *data, int len) ;


  



  virtual int FrameSizeChange(unsigned int, unsigned int, unsigned int);

  virtual int DeliverFrame(unsigned char*,int, uint32_t , int64_t,
                           void *handle);

  




  virtual bool IsTextureSupported() { return false; }

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
                      mPtrViEBase(nullptr),
                      mPtrViECapture(nullptr),
                      mPtrViECodec(nullptr),
                      mPtrViENetwork(nullptr),
                      mPtrViERender(nullptr),
                      mPtrExtCapture(nullptr),
                      mPtrRTP(nullptr),
                      mEngineTransmitting(false),
                      mEngineReceiving(false),
                      mChannel(-1),
                      mCapId(-1),
                      mCurSendCodecConfig(nullptr),
                      mSendingWidth(0),
                      mSendingHeight(0)
  {
  }

  virtual ~WebrtcVideoConduit() ;

  MediaConduitErrorCode Init(WebrtcVideoConduit *other);

  int GetChannel() { return mChannel; }
  webrtc::VideoEngine* GetVideoEngine() { return mVideoEngine; }
  bool GetLocalSSRC(unsigned int* ssrc);
  bool GetRemoteSSRC(unsigned int* ssrc);
  bool GetRTPJitter(unsigned int* jitterMs);
  bool GetRTCPReceiverReport(DOMHighResTimeStamp* timestamp,
                             unsigned int* jitterMs,
                             unsigned int* packetsReceived,
                             uint64_t* bytesReceived);
  bool GetRTCPSenderReport(DOMHighResTimeStamp* timestamp,
                           unsigned int* packetsSent,
                           uint64_t* bytesSent);

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

  
  
  
  WebrtcVideoConduit*  mOtherDirection;
  
  bool mShutDown;

  
  
  webrtc::VideoEngine* mVideoEngine;          
  mozilla::RefPtr<TransportInterface> mTransport;
  mozilla::RefPtr<VideoRenderer> mRenderer;

  webrtc::ViEBase* mPtrViEBase;
  webrtc::ViECapture* mPtrViECapture;
  webrtc::ViECodec* mPtrViECodec;
  webrtc::ViENetwork* mPtrViENetwork;
  webrtc::ViERender* mPtrViERender;
  webrtc::ViEExternalCapture*  mPtrExtCapture; 
  webrtc::ViERTP_RTCP* mPtrRTP;

  
  bool mEngineTransmitting; 
  bool mEngineReceiving;    

  int mChannel; 
  int mCapId;   
  RecvCodecList    mRecvCodecList;
  VideoCodecConfig* mCurSendCodecConfig;
  unsigned short mSendingWidth;
  unsigned short mSendingHeight;

  mozilla::RefPtr<WebrtcAudioConduit> mSyncedTo;
};

} 

#endif

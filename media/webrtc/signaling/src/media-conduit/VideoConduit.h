



#ifndef VIDEO_SESSION_H_
#define VIDEO_SESSION_H_

#include "mozilla/Attributes.h"

#include "MediaConduitInterface.h"


#include "common_types.h"
#include "video_engine/include/vie_base.h"
#include "video_engine/include/vie_capture.h"
#include "video_engine/include/vie_codec.h"
#include "video_engine/include/vie_render.h"
#include "video_engine/include/vie_network.h"
#include "video_engine/include/vie_file.h"
#include "video_engine/include/vie_rtp_rtcp.h"





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

  





   MediaConduitErrorCode AttachRenderer(mozilla::RefPtr<VideoRenderer> aVideoRenderer);

  



  virtual MediaConduitErrorCode ReceivedRTPPacket(const void *data, int len);

  



  virtual MediaConduitErrorCode ReceivedRTCPPacket(const void *data, int len);

   







  virtual MediaConduitErrorCode ConfigureSendMediaCodec(const VideoCodecConfig* codecInfo);

  








   virtual MediaConduitErrorCode ConfigureRecvMediaCodecs(
                               const std::vector<VideoCodecConfig* >& codecConfigList);

  



  virtual MediaConduitErrorCode AttachTransport(mozilla::RefPtr<TransportInterface> aTransport);

  










  virtual MediaConduitErrorCode SendVideoFrame(unsigned char* video_frame,
                                                unsigned int video_frame_length,
                                                unsigned short width,
                                                unsigned short height,
                                                VideoType video_type,
                                                uint64_t capture_time);



  



  virtual int SendPacket(int channel, const void *data, int len) ;

  



  virtual int SendRTCPPacket(int channel, const void *data, int len) ;


  



  virtual int FrameSizeChange(unsigned int, unsigned int, unsigned int);

  virtual int DeliverFrame(unsigned char*,int, uint32_t , int64_t);


  WebrtcVideoConduit():
                      mVideoEngine(NULL),
                      mTransport(NULL),
                      mRenderer(NULL),
                      mEngineTransmitting(false),
                      mEngineReceiving(false),
                      mEngineRendererStarted(false),
                      mChannel(-1),
                      mCapId(-1),
                      mCurSendCodecConfig(NULL)

  {
  }


  virtual ~WebrtcVideoConduit() ;



  MediaConduitErrorCode Init();

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
  bool mEngineRendererStarted; 

  int mChannel; 
  int mCapId;   
  RecvCodecList    mRecvCodecList;
  VideoCodecConfig* mCurSendCodecConfig;

  mozilla::RefPtr<WebrtcAudioConduit> mSyncedTo;
};



} 

#endif

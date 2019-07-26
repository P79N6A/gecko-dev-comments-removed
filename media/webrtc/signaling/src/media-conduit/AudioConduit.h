




#ifndef AUDIO_SESSION_H_
#define AUDIO_SESSION_H_

#include "mozilla/Attributes.h"
#include "mozilla/TimeStamp.h"
#include "nsTArray.h"

#include "MediaConduitInterface.h"
#include "MediaEngineWrapper.h"


#include "webrtc/common_types.h"
#include "webrtc/voice_engine/include/voe_base.h"
#include "webrtc/voice_engine/include/voe_volume_control.h"
#include "webrtc/voice_engine/include/voe_codec.h"
#include "webrtc/voice_engine/include/voe_file.h"
#include "webrtc/voice_engine/include/voe_network.h"
#include "webrtc/voice_engine/include/voe_external_media.h"
#include "webrtc/voice_engine/include/voe_audio_processing.h"
#include "webrtc/voice_engine/include/voe_video_sync.h"
#include "webrtc/voice_engine/include/voe_rtp_rtcp.h"

 using webrtc::VoEBase;
 using webrtc::VoENetwork;
 using webrtc::VoECodec;
 using webrtc::VoEExternalMedia;
 using webrtc::VoEAudioProcessing;
 using webrtc::VoEVideoSync;
 using webrtc::VoERTP_RTCP;



namespace mozilla {


DOMHighResTimeStamp
NTPtoDOMHighResTimeStamp(uint32_t ntpHigh, uint32_t ntpLow);





class WebrtcAudioConduit:public AudioSessionConduit
	      		            ,public webrtc::Transport
{
public:
  
  static const unsigned int CODEC_PLNAME_SIZE;

  



  virtual MediaConduitErrorCode ReceivedRTPPacket(const void *data, int len);

  



  virtual MediaConduitErrorCode ReceivedRTCPPacket(const void *data, int len);

  







  virtual MediaConduitErrorCode ConfigureSendMediaCodec(const AudioCodecConfig* codecConfig);
  








  virtual MediaConduitErrorCode ConfigureRecvMediaCodecs(
    const std::vector<AudioCodecConfig* >& codecConfigList);
  



  virtual MediaConduitErrorCode EnableAudioLevelExtension(bool enabled, uint8_t id);

  



  virtual MediaConduitErrorCode AttachTransport(mozilla::RefPtr<TransportInterface> aTransport);
  















  virtual MediaConduitErrorCode SendAudioFrame(const int16_t speechData[],
                                               int32_t lengthSamples,
                                               int32_t samplingFreqHz,
                                               int32_t capture_time);

  















   virtual MediaConduitErrorCode GetAudioFrame(int16_t speechData[],
                                              int32_t samplingFreqHz,
                                              int32_t capture_delay,
                                              int& lengthSamples);


  



  virtual int SendPacket(int channel, const void *data, int len) ;

  



  virtual int SendRTCPPacket(int channel, const void *data, int len) ;



  WebrtcAudioConduit():
                      mOtherDirection(nullptr),
                      mShutDown(false),
                      mVoiceEngine(nullptr),
                      mTransport(nullptr),
                      mEngineTransmitting(false),
                      mEngineReceiving(false),
                      mChannel(-1),
                      mCurSendCodecConfig(nullptr),
                      mCaptureDelay(150),
                      mEchoOn(true),
                      mEchoCancel(webrtc::kEcAec),
#ifdef MOZILLA_INTERNAL_API
                      mLastTimestamp(0),
#endif 
                      mSamples(0),
                      mLastSyncLog(0)
  {
  }

  virtual ~WebrtcAudioConduit();

  MediaConduitErrorCode Init(WebrtcAudioConduit *other);

  int GetChannel() { return mChannel; }
  webrtc::VoiceEngine* GetVoiceEngine() { return mVoiceEngine; }
  bool GetLocalSSRC(unsigned int* ssrc);
  bool GetRemoteSSRC(unsigned int* ssrc);
  bool GetAVStats(int32_t* jitterBufferDelayMs,
                  int32_t* playoutBufferDelayMs,
                  int32_t* avSyncOffsetMs);
  bool GetRTPStats(unsigned int* jitterMs, unsigned int* cumulativeLost);
  bool GetRTCPReceiverReport(DOMHighResTimeStamp* timestamp,
                             unsigned int* jitterMs,
                             unsigned int* packetsReceived,
                             uint64_t* bytesReceived,
                             unsigned int *cumulativeLost);
  bool GetRTCPSenderReport(DOMHighResTimeStamp* timestamp,
                           unsigned int* packetsSent,
                           uint64_t* bytesSent);

private:
  WebrtcAudioConduit(const WebrtcAudioConduit& other) MOZ_DELETE;
  void operator=(const WebrtcAudioConduit& other) MOZ_DELETE;

  
  typedef std::vector<AudioCodecConfig* > RecvCodecList;

  
  bool CodecConfigToWebRTCCodec(const AudioCodecConfig* codecInfo,
                                webrtc::CodecInst& cinst);

  
  bool IsSamplingFreqSupported(int freq) const;

  
  unsigned int GetNum10msSamplesForFrequency(int samplingFreqHz) const;

  
  bool CopyCodecToDB(const AudioCodecConfig* codecInfo);

  
  
  bool CheckCodecForMatch(const AudioCodecConfig* codecInfo) const;
  bool CheckCodecsForMatch(const AudioCodecConfig* curCodecConfig,
                           const AudioCodecConfig* codecInfo) const;
  
  MediaConduitErrorCode ValidateCodecConfig(const AudioCodecConfig* codecInfo, bool send) const;

  
  void DumpCodecDB() const;

  
  
  
  WebrtcAudioConduit*  mOtherDirection;
  
  bool mShutDown;

  
  
  webrtc::VoiceEngine* mVoiceEngine;
  mozilla::RefPtr<TransportInterface> mTransport;
  ScopedCustomReleasePtr<webrtc::VoENetwork>   mPtrVoENetwork;
  ScopedCustomReleasePtr<webrtc::VoEBase>      mPtrVoEBase;
  ScopedCustomReleasePtr<webrtc::VoECodec>     mPtrVoECodec;
  ScopedCustomReleasePtr<webrtc::VoEExternalMedia> mPtrVoEXmedia;
  ScopedCustomReleasePtr<webrtc::VoEAudioProcessing> mPtrVoEProcessing;
  ScopedCustomReleasePtr<webrtc::VoEVideoSync> mPtrVoEVideoSync;
  ScopedCustomReleasePtr<webrtc::VoERTP_RTCP>  mPtrVoERTP_RTCP;
  ScopedCustomReleasePtr<webrtc::VoERTP_RTCP>  mPtrRTP;
  
  bool mEngineTransmitting; 
  bool mEngineReceiving;    
                            
  
  
  
  struct Processing {
    TimeStamp mTimeStamp;
    uint32_t mRTPTimeStamp; 
  };
  nsAutoTArray<Processing,8> mProcessing;

  int mChannel;
  RecvCodecList    mRecvCodecList;
  AudioCodecConfig* mCurSendCodecConfig;

  
  int32_t mCaptureDelay;

  bool mEchoOn;
  webrtc::EcModes  mEchoCancel;

#ifdef MOZILLA_INTERNAL_API
  uint32_t mLastTimestamp;
#endif 

  uint32_t mSamples;
  uint32_t mLastSyncLog;
};

} 

#endif

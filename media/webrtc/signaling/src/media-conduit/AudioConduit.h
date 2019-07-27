




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

  



  virtual MediaConduitErrorCode ReceivedRTPPacket(const void *data, int len) override;

  



  virtual MediaConduitErrorCode ReceivedRTCPPacket(const void *data, int len) override;

  virtual MediaConduitErrorCode StopTransmitting() override;
  virtual MediaConduitErrorCode StartTransmitting() override;
  virtual MediaConduitErrorCode StopReceiving() override;
  virtual MediaConduitErrorCode StartReceiving() override;

  







  virtual MediaConduitErrorCode ConfigureSendMediaCodec(const AudioCodecConfig* codecConfig) override;
  








  virtual MediaConduitErrorCode ConfigureRecvMediaCodecs(
    const std::vector<AudioCodecConfig* >& codecConfigList) override;
  



  virtual MediaConduitErrorCode EnableAudioLevelExtension(bool enabled, uint8_t id) override;

  



  virtual MediaConduitErrorCode SetTransmitterTransport(mozilla::RefPtr<TransportInterface> aTransport) override;

  virtual MediaConduitErrorCode SetReceiverTransport(mozilla::RefPtr<TransportInterface> aTransport) override;

  















  virtual MediaConduitErrorCode SendAudioFrame(const int16_t speechData[],
                                               int32_t lengthSamples,
                                               int32_t samplingFreqHz,
                                               int32_t capture_time) override;

  















   virtual MediaConduitErrorCode GetAudioFrame(int16_t speechData[],
                                              int32_t samplingFreqHz,
                                              int32_t capture_delay,
                                              int& lengthSamples) override;


  



  virtual int SendPacket(int channel, const void *data, int len) override;

  



  virtual int SendRTCPPacket(int channel, const void *data, int len) override;


  virtual uint64_t CodecPluginID() override { return 0; }

  WebrtcAudioConduit():
                      mVoiceEngine(nullptr),
                      mTransportMonitor("WebrtcAudioConduit"),
                      mTransmitterTransport(nullptr),
                      mReceiverTransport(nullptr),
                      mEngineTransmitting(false),
                      mEngineReceiving(false),
                      mChannel(-1),
                      mCurSendCodecConfig(nullptr),
                      mCaptureDelay(150),
#if !defined(MOZILLA_EXTERNAL_LINKAGE)
                      mLastTimestamp(0),
#endif 
                      mSamples(0),
                      mLastSyncLog(0)
  {
  }

  virtual ~WebrtcAudioConduit();

  MediaConduitErrorCode Init();

  int GetChannel() { return mChannel; }
  webrtc::VoiceEngine* GetVoiceEngine() { return mVoiceEngine; }
  bool SetLocalSSRC(unsigned int ssrc) override;
  bool GetLocalSSRC(unsigned int* ssrc) override;
  bool GetRemoteSSRC(unsigned int* ssrc) override;
  bool SetLocalCNAME(const char* cname) override;
  bool GetVideoEncoderStats(double* framerateMean,
                            double* framerateStdDev,
                            double* bitrateMean,
                            double* bitrateStdDev,
                            uint32_t* droppedFrames) override
  {
    return false;
  }
  bool GetVideoDecoderStats(double* framerateMean,
                            double* framerateStdDev,
                            double* bitrateMean,
                            double* bitrateStdDev,
                            uint32_t* discardedPackets) override
  {
    return false;
  }
  bool GetAVStats(int32_t* jitterBufferDelayMs,
                  int32_t* playoutBufferDelayMs,
                  int32_t* avSyncOffsetMs) override;
  bool GetRTPStats(unsigned int* jitterMs, unsigned int* cumulativeLost) override;
  bool GetRTCPReceiverReport(DOMHighResTimeStamp* timestamp,
                             uint32_t* jitterMs,
                             uint32_t* packetsReceived,
                             uint64_t* bytesReceived,
                             uint32_t *cumulativeLost,
                             int32_t* rttMs) override;
  bool GetRTCPSenderReport(DOMHighResTimeStamp* timestamp,
                           unsigned int* packetsSent,
                           uint64_t* bytesSent) override;

private:
  WebrtcAudioConduit(const WebrtcAudioConduit& other) = delete;
  void operator=(const WebrtcAudioConduit& other) = delete;

  
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

  webrtc::VoiceEngine* mVoiceEngine;
  mozilla::ReentrantMonitor mTransportMonitor;
  mozilla::RefPtr<TransportInterface> mTransmitterTransport;
  mozilla::RefPtr<TransportInterface> mReceiverTransport;
  ScopedCustomReleasePtr<webrtc::VoENetwork>   mPtrVoENetwork;
  ScopedCustomReleasePtr<webrtc::VoEBase>      mPtrVoEBase;
  ScopedCustomReleasePtr<webrtc::VoECodec>     mPtrVoECodec;
  ScopedCustomReleasePtr<webrtc::VoEExternalMedia> mPtrVoEXmedia;
  ScopedCustomReleasePtr<webrtc::VoEAudioProcessing> mPtrVoEProcessing;
  ScopedCustomReleasePtr<webrtc::VoEVideoSync> mPtrVoEVideoSync;
  ScopedCustomReleasePtr<webrtc::VoERTP_RTCP>  mPtrVoERTP_RTCP;
  ScopedCustomReleasePtr<webrtc::VoERTP_RTCP>  mPtrRTP;
  
  mozilla::Atomic<bool> mEngineTransmitting; 
  mozilla::Atomic<bool> mEngineReceiving;    
                            
  
  
  
  struct Processing {
    TimeStamp mTimeStamp;
    uint32_t mRTPTimeStamp; 
  };
  nsAutoTArray<Processing,8> mProcessing;

  int mChannel;
  RecvCodecList    mRecvCodecList;
  AudioCodecConfig* mCurSendCodecConfig;

  
  int32_t mCaptureDelay;

#if !defined(MOZILLA_EXTERNAL_LINKAGE)
  uint32_t mLastTimestamp;
#endif 

  uint32_t mSamples;
  uint32_t mLastSyncLog;
};

} 

#endif

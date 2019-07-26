




#ifndef AUDIO_SESSION_H_
#define AUDIO_SESSION_H_

#include "mozilla/Attributes.h"

#include "MediaConduitInterface.h"


#include "common_types.h"
#include "voice_engine/include/voe_base.h"
#include "voice_engine/include/voe_volume_control.h"
#include "voice_engine/include/voe_codec.h"
#include "voice_engine/include/voe_file.h"
#include "voice_engine/include/voe_network.h"
#include "voice_engine/include/voe_external_media.h"


 using webrtc::VoEBase;
 using webrtc::VoENetwork;
 using webrtc::VoECodec;
 using webrtc::VoEExternalMedia;





namespace mozilla {





class WebrtcAudioConduit:public AudioSessionConduit
	      		            ,public webrtc::Transport
{
public:
  
  static const unsigned int CODEC_PLNAME_SIZE;

  



 virtual MediaConduitErrorCode ReceivedRTPPacket(const void *data, int len);

  



 virtual MediaConduitErrorCode ReceivedRTCPPacket(const void *data, int len);

  







 virtual MediaConduitErrorCode ConfigureSendMediaCodec(
                               const AudioCodecConfig* codecConfig);
  








 virtual MediaConduitErrorCode ConfigureRecvMediaCodecs(
                               const std::vector<AudioCodecConfig* >& codecConfigList);

  



 virtual MediaConduitErrorCode AttachTransport(
                               mozilla::RefPtr<TransportInterface> aTransport);

  















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
                      mVoiceEngine(NULL),
                      mTransport(NULL),
                      mEngineTransmitting(false),
                      mEngineReceiving(false),
                      mChannel(-1),
                      mCurSendCodecConfig(NULL)
  {
  }

  virtual ~WebrtcAudioConduit();

  MediaConduitErrorCode Init();

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

  webrtc::VoiceEngine* mVoiceEngine;
  mozilla::RefPtr<TransportInterface> mTransport;
  webrtc::VoENetwork*  mPtrVoENetwork;
  webrtc::VoEBase*     mPtrVoEBase;
  webrtc::VoECodec*    mPtrVoECodec;
  webrtc::VoEExternalMedia* mPtrVoEXmedia;

  
  bool mEngineTransmitting; 
  bool mEngineReceiving;    
                            

  int mChannel;
  RecvCodecList    mRecvCodecList;
  AudioCodecConfig* mCurSendCodecConfig;
};

} 

#endif

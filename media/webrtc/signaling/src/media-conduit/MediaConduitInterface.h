



#ifndef MEDIA_CONDUIT_ABSTRACTION_
#define MEDIA_CONDUIT_ABSTRACTION_

#include "nsISupportsImpl.h"
#include "nsXPCOM.h"
#include "nsDOMNavigationTiming.h"
#include "mozilla/RefPtr.h"
#include "CodecConfig.h"
#include "VideoTypes.h"
#include "MediaConduitErrors.h"

#include <vector>

namespace mozilla {





class TransportInterface
{
public:
  virtual ~TransportInterface() {}

  





  virtual nsresult SendRtpPacket(const void* data, int len) = 0;

  





  virtual nsresult SendRtcpPacket(const void* data, int len) = 0;
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(TransportInterface)
};










class VideoRenderer
{
 public:
  virtual ~VideoRenderer() {}

  





  virtual void FrameSizeChange(unsigned int width,
                               unsigned int height,
                               unsigned int number_of_streams) = 0;

  












  virtual void RenderVideoFrame(const unsigned char* buffer,
                                unsigned int buffer_size,
                                uint32_t time_stamp,
                                int64_t render_time) = 0;

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(VideoRenderer)
};











class MediaSessionConduit
{
public:
  enum Type { AUDIO, VIDEO } ;

  virtual ~MediaSessionConduit() {}

  virtual Type type() const = 0;

  







  virtual MediaConduitErrorCode ReceivedRTPPacket(const void *data, int len) = 0;

  







  virtual MediaConduitErrorCode ReceivedRTCPPacket(const void *data, int len) = 0;


  





  virtual MediaConduitErrorCode AttachTransport(RefPtr<TransportInterface> aTransport) = 0;

  virtual bool GetLocalSSRC(unsigned int* ssrc) = 0;
  virtual bool GetRemoteSSRC(unsigned int* ssrc) = 0;

  


  virtual bool GetRTPStats(unsigned int* jitterMs,
                           unsigned int* cumulativeLost) = 0;
  virtual bool GetRTCPReceiverReport(DOMHighResTimeStamp* timestamp,
                                     unsigned int* jitterMs,
                                     unsigned int* packetsReceived,
                                     uint64_t* bytesReceived,
                                     unsigned int* cumulativeLost) = 0;
  virtual bool GetRTCPSenderReport(DOMHighResTimeStamp* timestamp,
                                   unsigned int* packetsSent,
                                   uint64_t* bytesSent) = 0;

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MediaSessionConduit)

};







class VideoSessionConduit : public MediaSessionConduit
{
public:
  




  static RefPtr<VideoSessionConduit> Create(VideoSessionConduit *aOther);

  enum FrameRequestType
  {
    FrameRequestNone,
    FrameRequestFir,
    FrameRequestPli,
    FrameRequestUnknown
  };

  VideoSessionConduit() : mFrameRequestMethod(FrameRequestNone),
                          mUsingNackBasic(false) {}

  virtual ~VideoSessionConduit() {}

  virtual Type type() const { return VIDEO; }

  





  virtual MediaConduitErrorCode AttachRenderer(RefPtr<VideoRenderer> aRenderer) = 0;
  virtual void DetachRenderer() = 0;

  










  virtual MediaConduitErrorCode SendVideoFrame(unsigned char* video_frame,
                                               unsigned int video_frame_length,
                                               unsigned short width,
                                               unsigned short height,
                                               VideoType video_type,
                                               uint64_t capture_time) = 0;

  








  virtual MediaConduitErrorCode ConfigureSendMediaCodec(const VideoCodecConfig* sendSessionConfig) = 0;

  






  virtual MediaConduitErrorCode ConfigureRecvMediaCodecs(
                                const std::vector<VideoCodecConfig* >& recvCodecConfigList) = 0;


  



  virtual unsigned short SendingWidth() = 0;

  virtual unsigned short SendingHeight() = 0;

  virtual unsigned int SendingMaxFs() = 0;

  virtual unsigned int SendingMaxFr() = 0;

  



    FrameRequestType FrameRequestMethod() const {
      return mFrameRequestMethod;
    }

    bool UsingNackBasic() const {
      return mUsingNackBasic;
    }

   protected:
     
     FrameRequestType mFrameRequestMethod;
     bool mUsingNackBasic;
};






class AudioSessionConduit : public MediaSessionConduit
{
public:

   




  static mozilla::RefPtr<AudioSessionConduit> Create(AudioSessionConduit *aOther);

  virtual ~AudioSessionConduit() {}

  virtual Type type() const { return AUDIO; }


  















  virtual MediaConduitErrorCode SendAudioFrame(const int16_t audioData[],
                                                int32_t lengthSamples,
                                                int32_t samplingFreqHz,
                                                int32_t capture_delay) = 0;

  















  virtual MediaConduitErrorCode GetAudioFrame(int16_t speechData[],
                                              int32_t samplingFreqHz,
                                              int32_t capture_delay,
                                              int& lengthSamples) = 0;

   





  virtual MediaConduitErrorCode ConfigureSendMediaCodec(const AudioCodecConfig* sendCodecConfig) = 0;

   




  virtual MediaConduitErrorCode ConfigureRecvMediaCodecs(
                                const std::vector<AudioCodecConfig* >& recvCodecConfigList) = 0;
   





  virtual MediaConduitErrorCode EnableAudioLevelExtension(bool enabled, uint8_t id) = 0;

};
}
#endif









#ifndef MEDIA_CONDUIT_ABSTRACTION_
#define MEDIA_CONDUIT_ABSTRACTION_

#include "nsISupportsImpl.h"
#include "nsXPCOM.h"
#include "nsDOMNavigationTiming.h"
#include "mozilla/RefPtr.h"
#include "CodecConfig.h"
#include "VideoTypes.h"
#include "MediaConduitErrors.h"

#include "ImageContainer.h"

#include "webrtc/common_types.h"

#include <vector>

namespace mozilla {





class TransportInterface
{
protected:
  virtual ~TransportInterface() {}

public:
  





  virtual nsresult SendRtpPacket(const void* data, int len) = 0;

  





  virtual nsresult SendRtcpPacket(const void* data, int len) = 0;
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(TransportInterface)
};





class ImageHandle
{
public:
  explicit ImageHandle(layers::Image* image) : mImage(image) {}

  const RefPtr<layers::Image>& GetImage() const { return mImage; }

private:
  RefPtr<layers::Image> mImage;
};









class VideoRenderer
{
protected:
  virtual ~VideoRenderer() {}

public:
  





  virtual void FrameSizeChange(unsigned int width,
                               unsigned int height,
                               unsigned int number_of_streams) = 0;

  
















  virtual void RenderVideoFrame(const unsigned char* buffer,
                                unsigned int buffer_size,
                                uint32_t time_stamp,
                                int64_t render_time,
                                const ImageHandle& handle) = 0;

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(VideoRenderer)
};











class MediaSessionConduit
{
protected:
  virtual ~MediaSessionConduit() {}

public:
  enum Type { AUDIO, VIDEO } ;

  virtual Type type() const = 0;

  







  virtual MediaConduitErrorCode ReceivedRTPPacket(const void *data, int len) = 0;

  







  virtual MediaConduitErrorCode ReceivedRTCPPacket(const void *data, int len) = 0;

  virtual MediaConduitErrorCode StopTransmitting() = 0;
  virtual MediaConduitErrorCode StartTransmitting() = 0;
  virtual MediaConduitErrorCode StopReceiving() = 0;
  virtual MediaConduitErrorCode StartReceiving() = 0;


  









  virtual MediaConduitErrorCode SetTransmitterTransport(RefPtr<TransportInterface> aTransport) = 0;

  








  virtual MediaConduitErrorCode SetReceiverTransport(RefPtr<TransportInterface> aTransport) = 0;

  virtual bool SetLocalSSRC(unsigned int ssrc) = 0;
  virtual bool GetLocalSSRC(unsigned int* ssrc) = 0;
  virtual bool GetRemoteSSRC(unsigned int* ssrc) = 0;
  virtual bool SetLocalCNAME(const char* cname) = 0;

  


  virtual bool GetVideoEncoderStats(double* framerateMean,
                                    double* framerateStdDev,
                                    double* bitrateMean,
                                    double* bitrateStdDev,
                                    uint32_t* droppedFrames) = 0;
  virtual bool GetVideoDecoderStats(double* framerateMean,
                                    double* framerateStdDev,
                                    double* bitrateMean,
                                    double* bitrateStdDev,
                                    uint32_t* discardedPackets) = 0;
  virtual bool GetAVStats(int32_t* jitterBufferDelayMs,
                          int32_t* playoutBufferDelayMs,
                          int32_t* avSyncOffsetMs) = 0;
  virtual bool GetRTPStats(unsigned int* jitterMs,
                           unsigned int* cumulativeLost) = 0;
  virtual bool GetRTCPReceiverReport(DOMHighResTimeStamp* timestamp,
                                     uint32_t* jitterMs,
                                     uint32_t* packetsReceived,
                                     uint64_t* bytesReceived,
                                     uint32_t* cumulativeLost,
                                     int32_t* rttMs) = 0;
  virtual bool GetRTCPSenderReport(DOMHighResTimeStamp* timestamp,
                                   unsigned int* packetsSent,
                                   uint64_t* bytesSent) = 0;

  virtual uint64_t CodecPluginID() = 0;

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MediaSessionConduit)

};


class CodecPluginID
{
public:
  virtual ~CodecPluginID() {}

  virtual const uint64_t PluginID() = 0;
};

class VideoEncoder : public CodecPluginID
{
public:
  virtual ~VideoEncoder() {}
};

class VideoDecoder : public CodecPluginID
{
public:
  virtual ~VideoDecoder() {}
};






class VideoSessionConduit : public MediaSessionConduit
{
public:
  




  static RefPtr<VideoSessionConduit> Create();

  enum FrameRequestType
  {
    FrameRequestNone,
    FrameRequestFir,
    FrameRequestPli,
    FrameRequestUnknown
  };

  VideoSessionConduit() : mFrameRequestMethod(FrameRequestNone),
                          mUsingNackBasic(false),
                          mUsingTmmbr(false) {}

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

  virtual MediaConduitErrorCode ConfigureCodecMode(webrtc::VideoCodecMode) = 0;
  








  virtual MediaConduitErrorCode ConfigureSendMediaCodec(const VideoCodecConfig* sendSessionConfig) = 0;

  






  virtual MediaConduitErrorCode ConfigureRecvMediaCodecs(
                                const std::vector<VideoCodecConfig* >& recvCodecConfigList) = 0;

  




  virtual MediaConduitErrorCode SetExternalSendCodec(VideoCodecConfig* config,
                                                     VideoEncoder* encoder) = 0;

  




  virtual MediaConduitErrorCode SetExternalRecvCodec(VideoCodecConfig* config,
                                                     VideoDecoder* decoder) = 0;

  



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

    bool UsingTmmbr() const {
      return mUsingTmmbr;
    }
   protected:
     
     FrameRequestType mFrameRequestMethod;
     bool mUsingNackBasic;
     bool mUsingTmmbr;
};






class AudioSessionConduit : public MediaSessionConduit
{
public:

   




  static mozilla::RefPtr<AudioSessionConduit> Create();

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







#ifndef EMEVideoDecoder_h_
#define EMEVideoDecoder_h_

#include "GMPVideoDecoder.h"
#include "PlatformDecoderModule.h"

namespace mozilla {

class CDMProxy;
class MediaTaskQueue;

class EMEVideoCallbackAdapter : public VideoCallbackAdapter {
public:
  EMEVideoCallbackAdapter(MediaDataDecoderCallbackProxy* aCallback,
                          VideoInfo aVideoInfo,
                          layers::ImageContainer* aImageContainer)
   : VideoCallbackAdapter(aCallback, aVideoInfo, aImageContainer)
  {}

  virtual void Error(GMPErr aErr) override;
};

class EMEVideoDecoder : public GMPVideoDecoder {
public:
  EMEVideoDecoder(CDMProxy* aProxy,
                  const mp4_demuxer::VideoDecoderConfig& aConfig,
                  layers::LayersBackend aLayersBackend,
                  layers::ImageContainer* aImageContainer,
                  MediaTaskQueue* aTaskQueue,
                  MediaDataDecoderCallbackProxy* aCallback)
   : GMPVideoDecoder(aConfig, aLayersBackend, aImageContainer, aTaskQueue, aCallback,
                     new EMEVideoCallbackAdapter(aCallback, VideoInfo(aConfig.display_width,
                                                                      aConfig.display_height), aImageContainer))
   , mProxy(aProxy)
  {
  }

private:
  virtual void InitTags(nsTArray<nsCString>& aTags) override;
  virtual nsCString GetNodeId() override;
  virtual GMPUnique<GMPVideoEncodedFrame>::Ptr CreateFrame(MediaRawData* aSample) override;

  nsRefPtr<CDMProxy> mProxy;
};

}

#endif 







#ifndef EMEAudioDecoder_h_
#define EMEAudioDecoder_h_

#include "GMPAudioDecoder.h"
#include "PlatformDecoderModule.h"

namespace mozilla {

class EMEAudioCallbackAdapter : public AudioCallbackAdapter {
public:
  explicit EMEAudioCallbackAdapter(MediaDataDecoderCallbackProxy* aCallback)
   : AudioCallbackAdapter(aCallback)
  {}

  virtual void Error(GMPErr aErr) MOZ_OVERRIDE;
};

class EMEAudioDecoder : public GMPAudioDecoder {
public:
  EMEAudioDecoder(CDMProxy* aProxy,
                  const mp4_demuxer::AudioDecoderConfig& aConfig,
                  MediaTaskQueue* aTaskQueue,
                  MediaDataDecoderCallbackProxy* aCallback)
   : GMPAudioDecoder(aConfig, aTaskQueue, aCallback, new EMEAudioCallbackAdapter(aCallback))
   , mProxy(aProxy)
  {
  }

private:
  virtual void InitTags(nsTArray<nsCString>& aTags) MOZ_OVERRIDE;
  virtual nsCString GetNodeId() MOZ_OVERRIDE;

  nsRefPtr<CDMProxy> mProxy;
};

} 

#endif

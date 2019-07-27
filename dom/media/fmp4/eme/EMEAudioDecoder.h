





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

  virtual void Error(GMPErr aErr) override;
};

class EMEAudioDecoder : public GMPAudioDecoder {
public:
  EMEAudioDecoder(CDMProxy* aProxy,
                  const AudioInfo& aConfig,
                  MediaTaskQueue* aTaskQueue,
                  MediaDataDecoderCallbackProxy* aCallback)
   : GMPAudioDecoder(aConfig, aTaskQueue, aCallback, new EMEAudioCallbackAdapter(aCallback))
   , mProxy(aProxy)
  {
  }

private:
  virtual void InitTags(nsTArray<nsCString>& aTags) override;
  virtual nsCString GetNodeId() override;

  nsRefPtr<CDMProxy> mProxy;
};

} 

#endif

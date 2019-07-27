





#if !defined(EMEDecoderModule_h_)
#define EMEDecoderModule_h_

#include "PlatformDecoderModule.h"
#include "gmp-decryption.h"

namespace mozilla {

class CDMProxy;

class EMEDecoderModule : public PlatformDecoderModule {
private:

public:
  EMEDecoderModule(CDMProxy* aProxy,
                   PlatformDecoderModule* aPDM,
                   bool aCDMDecodesAudio,
                   bool aCDMDecodesVideo);

  virtual ~EMEDecoderModule();

  
  virtual already_AddRefed<MediaDataDecoder>
  CreateVideoDecoder(const VideoInfo& aConfig,
                    layers::LayersBackend aLayersBackend,
                    layers::ImageContainer* aImageContainer,
                    FlushableMediaTaskQueue* aVideoTaskQueue,
                    MediaDataDecoderCallback* aCallback) override;

  
  virtual already_AddRefed<MediaDataDecoder>
  CreateAudioDecoder(const AudioInfo& aConfig,
                     FlushableMediaTaskQueue* aAudioTaskQueue,
                     MediaDataDecoderCallback* aCallback) override;

  virtual ConversionRequired
  DecoderNeedsConversion(const TrackInfo& aConfig) const override;

private:
  nsRefPtr<CDMProxy> mProxy;
  
  nsRefPtr<PlatformDecoderModule> mPDM;
  
  nsRefPtr<MediaTaskQueue> mTaskQueue;
  bool mCDMDecodesAudio;
  bool mCDMDecodesVideo;

};

} 

#endif 

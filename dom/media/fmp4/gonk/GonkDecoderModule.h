





#if !defined(GonkPlatformDecoderModule_h_)
#define GonkPlatformDecoderModule_h_

#include "PlatformDecoderModule.h"

namespace mozilla {

class GonkDecoderModule : public PlatformDecoderModule {
public:
  GonkDecoderModule();
  virtual ~GonkDecoderModule();

  
  virtual already_AddRefed<MediaDataDecoder>
  CreateVideoDecoder(const VideoInfo& aConfig,
                     mozilla::layers::LayersBackend aLayersBackend,
                     mozilla::layers::ImageContainer* aImageContainer,
                     FlushableMediaTaskQueue* aVideoTaskQueue,
                     MediaDataDecoderCallback* aCallback) override;

  
  virtual already_AddRefed<MediaDataDecoder>
  CreateAudioDecoder(const AudioInfo& aConfig,
                     FlushableMediaTaskQueue* aAudioTaskQueue,
                     MediaDataDecoderCallback* aCallback) override;

  static void Init();

  virtual ConversionRequired
  DecoderNeedsConversion(const TrackInfo& aConfig) const override;
};

} 

#endif

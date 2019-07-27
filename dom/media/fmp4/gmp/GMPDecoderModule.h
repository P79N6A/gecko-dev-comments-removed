





#if !defined(GMPDecoderModule_h_)
#define GMPDecoderModule_h_

#include "PlatformDecoderModule.h"

namespace mozilla {

class GMPDecoderModule : public PlatformDecoderModule {
public:
  GMPDecoderModule();

  virtual ~GMPDecoderModule();

  
  virtual already_AddRefed<MediaDataDecoder>
  CreateVideoDecoder(const mp4_demuxer::VideoDecoderConfig& aConfig,
                     layers::LayersBackend aLayersBackend,
                     layers::ImageContainer* aImageContainer,
                     FlushableMediaTaskQueue* aVideoTaskQueue,
                     MediaDataDecoderCallback* aCallback) override;

  
  virtual already_AddRefed<MediaDataDecoder>
  CreateAudioDecoder(const mp4_demuxer::AudioDecoderConfig& aConfig,
                     FlushableMediaTaskQueue* aAudioTaskQueue,
                     MediaDataDecoderCallback* aCallback) override;

  virtual ConversionRequired
  DecoderNeedsConversion(const mp4_demuxer::TrackConfig& aConfig) const override;
};

} 

#endif 

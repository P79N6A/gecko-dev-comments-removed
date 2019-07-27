





#ifndef mozilla_AppleDecoderModule_h
#define mozilla_AppleDecoderModule_h

#include "PlatformDecoderModule.h"

namespace mozilla {

class AppleDecoderModule : public PlatformDecoderModule {
public:
  AppleDecoderModule();
  virtual ~AppleDecoderModule();

  
  
  nsresult Startup();

  
  
  virtual nsresult Shutdown() MOZ_OVERRIDE;

  
  virtual MediaDataDecoder*
  CreateH264Decoder(const mp4_demuxer::VideoDecoderConfig& aConfig,
                    mozilla::layers::LayersBackend aLayersBackend,
                    mozilla::layers::ImageContainer* aImageContainer,
                    MediaTaskQueue* aVideoTaskQueue,
                    MediaDataDecoderCallback* aCallback) MOZ_OVERRIDE;

  
  virtual MediaDataDecoder* CreateAACDecoder(
    const mp4_demuxer::AudioDecoderConfig& aConfig,
    MediaTaskQueue* aAudioTaskQueue,
    MediaDataDecoderCallback* aCallback) MOZ_OVERRIDE;

  static void Init();
private:
  static bool sIsEnabled;
};

} 

#endif 

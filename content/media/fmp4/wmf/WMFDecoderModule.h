





#if !defined(WMFPlatformDecoderModule_h_)
#define WMFPlatformDecoderModule_h_

#include "PlatformDecoderModule.h"

namespace mozilla {

class WMFDecoderModule : public PlatformDecoderModule {
public:
  WMFDecoderModule();
  virtual ~WMFDecoderModule();

  
  
  nsresult Startup();

  
  
  virtual nsresult Shutdown() MOZ_OVERRIDE;

  
  virtual MediaDataDecoder*
  CreateH264Decoder(const mp4_demuxer::VideoDecoderConfig& aConfig,
                    layers::LayersBackend aLayersBackend,
                    layers::ImageContainer* aImageContainer,
                    MediaTaskQueue* aVideoTaskQueue,
                    MediaDataDecoderCallback* aCallback) MOZ_OVERRIDE;

  
  virtual MediaDataDecoder*
  CreateAACDecoder(const mp4_demuxer::AudioDecoderConfig& aConfig,
                   MediaTaskQueue* aAudioTaskQueue,
                   MediaDataDecoderCallback* aCallback) MOZ_OVERRIDE;

  static void Init();
private:
  static bool sIsWMFEnabled;
  static bool sDXVAEnabled;
};

} 

#endif

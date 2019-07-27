





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

  virtual already_AddRefed<MediaDataDecoder>
  CreateH264Decoder(const mp4_demuxer::VideoDecoderConfig& aConfig,
                    layers::LayersBackend aLayersBackend,
                    layers::ImageContainer* aImageContainer,
                    MediaTaskQueue* aVideoTaskQueue,
                    MediaDataDecoderCallback* aCallback) MOZ_OVERRIDE;

  virtual already_AddRefed<MediaDataDecoder>
  CreateAudioDecoder(const mp4_demuxer::AudioDecoderConfig& aConfig,
                     MediaTaskQueue* aAudioTaskQueue,
                     MediaDataDecoderCallback* aCallback) MOZ_OVERRIDE;

  bool SupportsAudioMimeType(const char* aMimeType) MOZ_OVERRIDE;

  
  static void Init();
private:
  static bool sIsWMFEnabled;
  static bool sDXVAEnabled;
};

} 

#endif

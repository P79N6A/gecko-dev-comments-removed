





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
                    mozilla::layers::LayersBackend aLayersBackend,
                    mozilla::layers::ImageContainer* aImageContainer) MOZ_OVERRIDE;

  
  virtual MediaDataDecoder* CreateAACDecoder(
    const mp4_demuxer::AudioDecoderConfig& aConfig) MOZ_OVERRIDE;

  
  virtual void OnDecodeThreadStart() MOZ_OVERRIDE;
  virtual void OnDecodeThreadFinish() MOZ_OVERRIDE;

  static void Init();
private:
  static bool sIsWMFEnabled;
  static bool sDXVAEnabled;
};

} 

#endif

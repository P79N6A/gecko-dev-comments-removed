





#if !defined(WMFPlatformDecoderModule_h_)
#define WMFPlatformDecoderModule_h_

#include "PlatformDecoderModule.h"

namespace mozilla {

class WMFDecoderModule : public PlatformDecoderModule {
public:
  WMFDecoderModule();
  virtual ~WMFDecoderModule();

  
  
  nsresult Init();

  
  
  virtual nsresult Shutdown() MOZ_OVERRIDE;

  
  virtual MediaDataDecoder*
  CreateVideoDecoder(mozilla::layers::LayersBackend aLayersBackend,
                     mozilla::layers::ImageContainer* aImageContainer) MOZ_OVERRIDE;

  
  virtual MediaDataDecoder* CreateAudioDecoder(uint32_t aChannelCount,
                                               uint32_t aSampleRate,
                                               uint16_t aBitsPerSample,
                                               const uint8_t* aUserData,
                                               uint32_t aUserDataLength) MOZ_OVERRIDE;

  
  virtual void OnDecodeThreadStart() MOZ_OVERRIDE;
  virtual void OnDecodeThreadFinish() MOZ_OVERRIDE;
private:
  const bool mDXVAEnabled;
};

} 

#endif
